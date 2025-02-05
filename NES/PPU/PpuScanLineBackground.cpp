#include "..\NES.h"

DWORD PpuProcessScanLine_Background_AddTileToQueue(BYTE bTileHigh, BYTE bTileLow, BYTE bAttributeValue)
{
	// store the next 8 pixels in the queue
	for(DWORD i = 0; i < 8; i++)
	{
		gSystem.Ppu.BackgroundPixelQueue[8 + i].bPaletteEntryIndex = (GET_BIT(bTileHigh, 7 - i) << 1) + GET_BIT(bTileLow, 7 - i);
		gSystem.Ppu.BackgroundPixelQueue[8 + i].bPaletteIndex = bAttributeValue;
	}

	return 0;
}

DWORD PpuProcessScanLine_Background_GetPixelFromQueue(BYTE *pbPaletteIndex, BYTE *pbPaletteEntryIndex)
{
	BackgroundPixelQueueStruct *pBackgroundPixelQueueEntry = NULL;

	if(gSystem.Ppu.Reg.bX >= ELEMENT_COUNT(gSystem.Ppu.BackgroundPixelQueue))
	{
		return 1;
	}
	pBackgroundPixelQueueEntry = &gSystem.Ppu.BackgroundPixelQueue[gSystem.Ppu.Reg.bX];

	// store values
	*pbPaletteIndex = pBackgroundPixelQueueEntry->bPaletteIndex;
	*pbPaletteEntryIndex = pBackgroundPixelQueueEntry->bPaletteEntryIndex;

	// adjust queue
	memmove(&gSystem.Ppu.BackgroundPixelQueue[0], &gSystem.Ppu.BackgroundPixelQueue[1], sizeof(BackgroundPixelQueueStruct) * (BACKGROUND_PIXEL_QUEUE_COUNT - 1));

	return 0;
}

DWORD PpuProcessScanLine_Background_FetchTileCycle(BYTE bCycleType)
{
	BYTE bNameTableIndex = 0;
	BYTE bTilePosX = 0;
	BYTE bTilePosY = 0;
	BYTE bTileOffsetY = 0;
	BYTE bPpuCurrTileHigh = 0;
	BYTE bNameTableX = 0;
	WORD wNameTableBase = 0;
	WORD wAttributeTableBase = 0;
	WORD wAttributeAddress = 0;
	BYTE bAttributeTableEntry = 0;
	BYTE bAttributeTableEntryShiftCount = 0;

	if(bCycleType == 0)
	{
		// get name table base address
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE, &bNameTableIndex);
		wNameTableBase = 0x2000 + (bNameTableIndex * 0x400);

		// get name table entry for current tile - reading the lowest 10 bits of V register is equivalent to: (TILE_POS_Y << 5) + TILE_POS_X
		PpuMemory_Read8(wNameTableBase + (gSystem.Ppu.Reg.wV & 0x3FF), &gSystem.Ppu.bCurrNameTableEntry);
	}
	else if(bCycleType == 1)
	{
		// get attribute table entry address
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, &bTilePosX);
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, &bTilePosY);
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE, &bNameTableIndex);
		wNameTableBase = 0x2000 + (bNameTableIndex * 0x400);
		wAttributeTableBase = wNameTableBase + 0x3C0;
		wAttributeAddress = wAttributeTableBase + ((bTilePosY / 4) * 8) + (bTilePosX / 4);

		// read attribute table entry
		PpuMemory_Read8(wAttributeAddress, &bAttributeTableEntry);

		// extract relevant bits from attribute byte
		if((bTilePosY % 4) < 2)
		{
			if((bTilePosX % 4) < 2)
			{
				// top-left
				bAttributeTableEntryShiftCount = 0;
			}
			else
			{
				// top-right
				bAttributeTableEntryShiftCount = 2;
			}
		}
		else
		{
			if((bTilePosX % 4) < 2)
			{
				// bottom-left
				bAttributeTableEntryShiftCount = 4;
			}
			else
			{
				// bottom-right
				bAttributeTableEntryShiftCount = 6;
			}
		}

		// store value
		gSystem.Ppu.bCurrAttributeValue = (bAttributeTableEntry >> bAttributeTableEntryShiftCount) & 3;
	}
	else if(bCycleType == 2)
	{
		// read tile data (low)
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, &bTileOffsetY);
		gSystem.Ppu.wCurrPatternTableAddress = (gSystem.Ppu.bCurrNameTableEntry * 16) + bTileOffsetY;
		if(PpuControlFlag_Get(PPU_CONTROL_FLAG_BACKGROUND_PATTERN_TABLE_MODE) != 0)
		{
			// use alternative pattern table
			gSystem.Ppu.wCurrPatternTableAddress += 0x1000;
		}
		PpuMemory_Read8(gSystem.Ppu.wCurrPatternTableAddress, &gSystem.Ppu.bCurrTileLow);
	}
	else if(bCycleType == 3)
	{
		// read tile data (high)
		gSystem.Ppu.wCurrPatternTableAddress += 8;
		PpuMemory_Read8(gSystem.Ppu.wCurrPatternTableAddress, &bPpuCurrTileHigh);

		// finished reading tile - store the next 8 pixels in the queue
		PpuProcessScanLine_Background_AddTileToQueue(bPpuCurrTileHigh, gSystem.Ppu.bCurrTileLow, gSystem.Ppu.bCurrAttributeValue);

		// increase tile X position
		PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, &bTilePosX);
		bTilePosX++;
		if(bTilePosX == 32)
		{
			// overflow - reset tile X position and switch name-table
			bTilePosX = 0;
			PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X, &bNameTableX);
			bNameTableX ^= 1;
			PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X, bNameTableX);
		}
		PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, bTilePosX);
	}
	else
	{
		return 1;
	}

	return 0;
}

DWORD PpuProcessScanLine_Background(BYTE bPreRenderLine)
{
	BYTE bTilePosX = 0;
	BYTE bTilePosY = 0;
	BYTE bNameTableX = 0;
	BYTE bNameTableY = 0;
	BYTE bPaletteIndex = 0;
	BYTE bPaletteEntryIndex = 0;
	BYTE bTileOffsetY = 0;
	BYTE bTileCycleIndex = 0;
	BYTE bTileCycleType = 0;

	if((gSystem.Ppu.wDotIndex >= 1 && gSystem.Ppu.wDotIndex <= 256) || (gSystem.Ppu.wDotIndex >= 321 && gSystem.Ppu.wDotIndex <= 336))
	{
		// get next pixel from queue - the queue is buffered 16 pixels (or 2 tiles) in advance
		if(PpuProcessScanLine_Background_GetPixelFromQueue(&bPaletteIndex, &bPaletteEntryIndex) != 0)
		{
			return 1;
		}

		// check if the pixel is within the visible range
		if(bPreRenderLine == 0 && gSystem.Ppu.wDotIndex <= 256)
		{
			gSystem.Ppu.pCurrPixelBackground = NULL;
			if(bPaletteEntryIndex != 0)
			{
				// get colour palette entry address
				gSystem.Ppu.pCurrPixelBackground = GetSystemColourFromPalette(bPaletteIndex, bPaletteEntryIndex);
				if(gSystem.Ppu.pCurrPixelBackground == NULL)
				{
					return 1;
				}
			}
		}

		// get cycle index for the current tile
		bTileCycleIndex = (gSystem.Ppu.wDotIndex - 1) % 8;
		if((bTileCycleIndex % 2) == 1)
		{
			// only fetch on odd cycles - the NES takes 2 cycles to read each of the 4 elements per tile
			bTileCycleType = (bTileCycleIndex - 1) / 2;
			if(PpuProcessScanLine_Background_FetchTileCycle(bTileCycleType) != 0)
			{
				return 1;
			}
		}

		// check if this is the end of the current line
		if(gSystem.Ppu.wDotIndex == 256)
		{
			// increase tile Y offset
			PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, &bTileOffsetY);
			bTileOffsetY++;
			if(bTileOffsetY == 8)
			{
				// overflow - reset tile Y offset
				bTileOffsetY = 0;

				// increase tile Y position
				PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, &bTilePosY);
				bTilePosY++;

				if(bTilePosY == 30)
				{
					// reset tile Y position
					bTilePosY = 0;

					// switch vertical name-table
					PpuAddressRegister_V_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y, &bNameTableY);
					bNameTableY ^= 1;
					PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y, bNameTableY);
				}
				else if(bTilePosY == 32)
				{
					// out of range - reset tile Y position
					bTilePosY = 0;
				}
				PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, bTilePosY);
			}
			PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, bTileOffsetY);
		}
	}
	else if(gSystem.Ppu.wDotIndex == 257)
	{
		// V.TILE_POS_X = T.TILE_POS_X
		PpuAddressRegister_T_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, &bTilePosX);
		PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, bTilePosX);

		// V.NAME_TABLE_X = T.NAME_TABLE_X
		PpuAddressRegister_T_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X, &bNameTableX);
		PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X, bNameTableX);
	}
	else if(gSystem.Ppu.wDotIndex >= 280 && gSystem.Ppu.wDotIndex <= 304)
	{
		// check if this is the pre-render line
		if(bPreRenderLine != 0)
		{
			// V.TILE_POS_Y = T.TILE_POS_Y
			PpuAddressRegister_T_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, &bTilePosY);
			PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, bTilePosY);

			// V.TILE_OFFSET_Y = T.TILE_OFFSET_Y
			PpuAddressRegister_T_Get(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, &bTileOffsetY);
			PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, bTileOffsetY);

			// V.NAME_TABLE_Y = T.NAME_TABLE_Y
			PpuAddressRegister_T_Get(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y, &bNameTableY);
			PpuAddressRegister_V_Set(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y, bNameTableY);
		}
	}

	return 0;
}
