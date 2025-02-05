#include "..\NES.h"

DWORD PpuMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue)
{
	BYTE bValue = 0;

	// handle ppu register memory read
	if(wAddress == PPU_REGISTER_PPUSTATUS)
	{
		bValue = gSystem.Ppu.bPpuStatus;

		// reset write latch
		gSystem.Ppu.Reg.bW = 0;

		// clear v-blank flag
		PpuStatusFlag_Clear(PPU_STATUS_FLAG_VBLANK);
	}
	else if(wAddress == PPU_REGISTER_OAMDATA)
	{
		// read OAM data value (without increasing address afterwards)
		PpuObjectAttributeMemory_Read(&bValue);
	}
	else if(wAddress == PPU_REGISTER_PPUDATA)
	{
		// get data from read buffer
		bValue = gSystem.Ppu.bReadBuffer;

		// read next value to buffer
		PpuMemory_Read8(gSystem.Ppu.Reg.wV, &gSystem.Ppu.bReadBuffer);

		// increase ppu V address
		PpuAddressRegister_IncreaseV();
	}
	else
	{
		return 1;
	}

	*pbValue = bValue;

	return 0;
}

DWORD PpuMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue)
{
	BYTE bToggleWriteLatch = 0;

	// handle ppu register memory write
	if(wAddress == PPU_REGISTER_PPUCTRL)
	{
		// store control value (ignore lowest 2 bits, there are stored in T register instead)
		gSystem.Ppu.bPpuControl = bValue & ~0x3;

		// store name table value
		PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE, bValue);
	}
	else if(wAddress == PPU_REGISTER_PPUMASK)
	{
		// store mask value
		gSystem.Ppu.bPpuMask = bValue;
	}
	else if(wAddress == PPU_REGISTER_OAMADDR)
	{
		// set OAM address
		gSystem.Ppu.bObjectAttributeMemoryAddress = bValue;
	}
	else if(wAddress == PPU_REGISTER_OAMDATA)
	{
		// set OAM data value
		PpuObjectAttributeMemory_Write(bValue);

		// increase address
		gSystem.Ppu.bObjectAttributeMemoryAddress++;
	}
	else if(wAddress == PPU_REGISTER_PPUSCROLL)
	{
		if(gSystem.Ppu.Reg.bW == 0)
		{
			// X scroll
			PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X, bValue >> 3);
			gSystem.Ppu.Reg.bX = bValue & 7;
		}
		else
		{
			// Y scroll
			PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y, bValue >> 3);
			PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y, bValue & 7);
		}

		bToggleWriteLatch = 1;
	}
	else if(wAddress == PPU_REGISTER_PPUADDR)
	{
		if(gSystem.Ppu.Reg.bW == 0)
		{
			// high byte
			PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_ADDR_HIGH, bValue);
		}
		else
		{
			// low byte
			PpuAddressRegister_T_Set(PPU_ADDRESS_REGISTER_FIELD_ADDR_LOW, bValue);
			gSystem.Ppu.Reg.wV = gSystem.Ppu.Reg.wT;
		}

		bToggleWriteLatch = 1;
	}
	else if(wAddress == PPU_REGISTER_PPUDATA)
	{
		// write data
		PpuMemory_Write8(gSystem.Ppu.Reg.wV, bValue);

		// increase ppu V address
		PpuAddressRegister_IncreaseV();
	}
	else if(wAddress == PPU_REGISTER_OAMDMA)
	{
		// DMA write
		gSystem.Cpu.bDmaPending = 1;
		gSystem.Cpu.wDmaBaseAddress = bValue << 8;
	}
	else
	{
		return 1;
	}

	if(bToggleWriteLatch != 0)
	{
		// toggle write latch
		gSystem.Ppu.Reg.bW ^= 1;
	}

	return 0;
}
