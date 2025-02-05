#include "..\NES.h"

DWORD PpuProcessScanLine_Sprites_PrepareNextLine()
{
	BYTE bNextLineY = 0;
	BYTE bCurrSpriteHeight = 0;
	ObjectAttributeEntryStruct *pCurrSprite = NULL;

	// populate sprite list for the next line
	gSystem.Ppu.bCurrLineSpriteCount = 0;
	for(DWORD i = 0; i < MAX_SPRITE_COUNT; i++)
	{
		pCurrSprite = &gSystem.Ppu.ObjectAttributeMemory[i];
		if(pCurrSprite->bY >= 240)
		{
			// sprite is hidden
			continue;
		}

		bNextLineY = (BYTE)gSystem.Ppu.wScanLineIndex;

		bCurrSpriteHeight = 8;
		if(PpuControlFlag_Get(PPU_CONTROL_FLAG_SPRITE_SIZE_8X16) != 0)
		{
			// 8x16 mode
			bCurrSpriteHeight = 16;
		}

		if(bNextLineY >= pCurrSprite->bY)
		{
			if((bNextLineY - pCurrSprite->bY) < bCurrSpriteHeight)
			{
				// sprite visible on next line
				if(gSystem.Ppu.bCurrLineSpriteCount == MAX_SPRITES_PER_LINE)
				{
					PpuStatusFlag_Set(PPU_STATUS_FLAG_SPRITE_OVERFLOW);
					break;
				}
				else
				{
					// store current sprite in list
					memcpy(&gSystem.Ppu.CurrLineSpriteList[gSystem.Ppu.bCurrLineSpriteCount], &gSystem.Ppu.ObjectAttributeMemory[i], sizeof(ObjectAttributeEntryStruct));
					gSystem.Ppu.bCurrLineSpriteIndexList[gSystem.Ppu.bCurrLineSpriteCount] = (BYTE)i;
					gSystem.Ppu.bCurrLineSpriteCount++;
				}
			}
		}
	}

	return 0;
}

DWORD PpuProcessScanLine_Sprites_ProcessCurrentLine()
{
	BYTE bCurrX = 0;
	BYTE bCurrY = 0;
	BYTE bSpriteBehindBackground = 0;
	BYTE bVerticalFlip = 0;
	BYTE bHorizontalFlip = 0;
	BYTE bDeltaX = 0;
	BYTE bCurrSpritePositionY = 0;
	BYTE bPatternDataHigh = 0;
	BYTE bPatternDataLow = 0;
	WORD wPatternTableAddress = 0;
	BYTE bPixelShiftX = 0;
	BYTE bPaletteEntryIndex = 0;
	BYTE bPaletteIndex = 0;
	BYTE bCurrSpriteTile = 0;
	ObjectAttributeEntryStruct *pCurrSprite = NULL;

	bCurrX = gSystem.Ppu.wDotIndex - 1;
	bCurrY = gSystem.Ppu.wScanLineIndex - 1;
	for(DWORD i = 0; i < gSystem.Ppu.bCurrLineSpriteCount; i++)
	{
		// get sprite data
		pCurrSprite = &gSystem.Ppu.CurrLineSpriteList[i];

		bSpriteBehindBackground = 0;
		if(pCurrSprite->bAttributes & 0x20)
		{
			bSpriteBehindBackground = 1;
		}

		bHorizontalFlip = 0;
		if(pCurrSprite->bAttributes & 0x40)
		{
			bHorizontalFlip = 1;
		}

		bVerticalFlip = 0;
		if(pCurrSprite->bAttributes & 0x80)
		{
			bVerticalFlip = 1;
		}

		if(bCurrX >= pCurrSprite->bX)
		{
			bDeltaX = (bCurrX - pCurrSprite->bX);
			if(bDeltaX < 8)
			{
				// sprite exists within this region, check if the current pixel matches
				bCurrSpritePositionY = bCurrY - pCurrSprite->bY;

				// get tile
				bCurrSpriteTile = pCurrSprite->bTile;

				// check sprite mode
				wPatternTableAddress = 0;
				if(PpuControlFlag_Get(PPU_CONTROL_FLAG_SPRITE_SIZE_8X16) == 0)
				{
					// 8x8
					if(PpuControlFlag_Get(PPU_CONTROL_FLAG_SPRITE_PATTERN_TABLE_MODE) != 0)
					{
						wPatternTableAddress = 0x1000;
					}

					if(bVerticalFlip != 0)
					{
						// vertical flip - adjust Y position
						bCurrSpritePositionY = 7 - bCurrSpritePositionY;
					}
				}
				else
				{
					// 8x16
					if(pCurrSprite->bTile & 1)
					{
						// in 8x16 mode, bit 0 specifies the pattern table
						wPatternTableAddress = 0x1000;

						// drop bit 0
						bCurrSpriteTile--;
					}

					if(bVerticalFlip != 0)
					{
						// vertical flip - adjust Y position
						bCurrSpritePositionY = 15 - bCurrSpritePositionY;
					}

					if(bCurrSpritePositionY >= 8)
					{
						bCurrSpriteTile++;
					}
				}

				wPatternTableAddress += (bCurrSpriteTile * 16);
				wPatternTableAddress += (bCurrSpritePositionY % 8);

				PpuMemory_Read8(wPatternTableAddress, &bPatternDataLow);
				PpuMemory_Read8(wPatternTableAddress + 8, &bPatternDataHigh);

				if(bHorizontalFlip != 0)
				{
					// horizontal flip
					bPixelShiftX = bDeltaX;
				}
				else
				{
					bPixelShiftX = 7 - bDeltaX;
				}

				bPaletteEntryIndex = (GET_BIT(bPatternDataHigh, bPixelShiftX) << 1) + GET_BIT(bPatternDataLow, bPixelShiftX);
				gSystem.Ppu.pCurrPixelSprite = NULL;
				if(bPaletteEntryIndex != 0)
				{
					// get colour palette entry address
					bPaletteIndex = 4 + (pCurrSprite->bAttributes & 3);
					gSystem.Ppu.pCurrPixelSprite = GetSystemColourFromPalette(bPaletteIndex, bPaletteEntryIndex);
					if(gSystem.Ppu.pCurrPixelSprite == NULL)
					{
						return 1;
					}
				}
				else
				{
					// this sprite is not visible at the current X position, try next sprite
					continue;
				}

				// store sprite priority
				gSystem.Ppu.bCurrPixelSpriteBehindBackground = bSpriteBehindBackground;

				// store sprite index, this is required to check for sprite 0 hits later
				gSystem.Ppu.bCurrPixelSpriteIndex = gSystem.Ppu.bCurrLineSpriteIndexList[i];

				// stop here - first sprite has priority
				break;
			}
		}
	}

	return 0;
}

DWORD PpuProcessScanLine_Sprites(BYTE bPreRenderLine)
{
	if(gSystem.Ppu.wDotIndex >= 257 && gSystem.Ppu.wDotIndex <= 320)
	{
		// reset OAM address
		gSystem.Ppu.bObjectAttributeMemoryAddress = 0;
	}

	if(bPreRenderLine == 0)
	{
		// handle sprites
		if(gSystem.Ppu.wDotIndex == 257)
		{
			// populate sprite list for the next line
			if(PpuProcessScanLine_Sprites_PrepareNextLine() != 0)
			{
				return 1;
			}
		}

		// draw sprites for the current line
		if(gSystem.Ppu.wDotIndex >= 1 && gSystem.Ppu.wDotIndex <= 256)
		{
			// ignore first line - nothing to render
			if(gSystem.Ppu.wScanLineIndex != 0)
			{
				if(PpuProcessScanLine_Sprites_ProcessCurrentLine() != 0)
				{
					return 1;
				}
			}
		}
	}

	return 0;
}
