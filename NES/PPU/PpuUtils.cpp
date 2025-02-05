#include "..\NES.h"

DWORD PpuStatusFlag_Set(BYTE bFlag)
{
	// set flag
	gSystem.Ppu.bPpuStatus |= bFlag;

	return 0;
}

DWORD PpuStatusFlag_Clear(BYTE bFlag)
{
	// clear flag
	gSystem.Ppu.bPpuStatus &= ~bFlag;

	return 0;
}

BYTE PpuControlFlag_Get(BYTE bFlag)
{
	// get ppu control flag value
	if(gSystem.Ppu.bPpuControl & bFlag)
	{
		return 1;
	}

	return 0;
}

BYTE PpuMaskFlag_Get(BYTE bFlag)
{
	// get ppu mask flag value
	if(gSystem.Ppu.bPpuMask & bFlag)
	{
		return 1;
	}

	return 0;
}

DWORD PpuMemory_FixAddress(WORD wAddress, WORD *pwFixedAddress)
{
	WORD wFixedAddress = 0;

	// ppu memory is 16kb - drop high bits
	wFixedAddress = wAddress % sizeof(gSystem.Ppu.bInternalMemory);

	if(wFixedAddress >= 0x3F20)
	{
		// ppu palette indexes - first 32 bytes (0x3F00 -> 0x3F20) are mirrored 8 times, only write to the first block
		wFixedAddress = 0x3F00 + (wFixedAddress % 0x20);
	}

	// handle palette mirroring
	if(wFixedAddress == 0x3F10 || wFixedAddress == 0x3F14 || wFixedAddress == 0x3F18 || wFixedAddress == 0x3F1C)
	{
		wFixedAddress -= 0x10;
	}

	if(gSystem.Ppu.NameTableMirrorMode == NAME_TABLE_MIRROR_MODE_VERTICAL)
	{
		// handle vertical name table mirroring
		if((wFixedAddress >= 0x2800 && wFixedAddress < 0x2C00) || (wFixedAddress >= 0x2C00 && wFixedAddress < 0x3000))
		{
			wFixedAddress -= 0x800;
		}
	}
	else if(gSystem.Ppu.NameTableMirrorMode == NAME_TABLE_MIRROR_MODE_HORIZONTAL)
	{
		// handle horizontal name table mirroring
		if((wFixedAddress >= 0x2400 && wFixedAddress < 0x2800) || (wFixedAddress >= 0x2C00 && wFixedAddress < 0x3000))
		{
			wFixedAddress -= 0x400;
		}
	}

	// store fixed address
	*pwFixedAddress = wFixedAddress;

	return 0;
}

DWORD PpuMemory_Read8(WORD wAddress, BYTE *pbValue)
{
	WORD wFixedAddress = 0;

	// read byte
	PpuMemory_FixAddress(wAddress, &wFixedAddress);
	*pbValue = gSystem.Ppu.bInternalMemory[wFixedAddress];

	return 0;
}

DWORD PpuMemory_Write8(WORD wAddress, BYTE bValue)
{
	WORD wFixedAddress = 0;

	// write byte
	PpuMemory_FixAddress(wAddress, &wFixedAddress);
	gSystem.Ppu.bInternalMemory[wFixedAddress] = bValue;

	return 0;
}

DWORD PpuMemory_WriteRange(WORD wAddress, VOID *pData, DWORD dwLength)
{
	WORD wCurrAddress = 0;
	BYTE *pCurrPtr = NULL;

	// write data range
	wCurrAddress = wAddress;
	pCurrPtr = (BYTE*)pData;
	for(DWORD i = 0; i < dwLength; i++)
	{
		PpuMemory_Write8(wCurrAddress, *pCurrPtr);
		wCurrAddress++;
		pCurrPtr++;
	}

	return 0;
}

DWORD PpuObjectAttributeMemory_Read(BYTE *pbValue)
{
	// get byte
	*pbValue = *(BYTE*)((BYTE*)&gSystem.Ppu.ObjectAttributeMemory + gSystem.Ppu.bObjectAttributeMemoryAddress);

	return 0;
}

DWORD PpuObjectAttributeMemory_Write(BYTE bValue)
{
	// set byte (ObjectAttributeMemory is 256 bytes so this can't overflow)
	*(BYTE*)((BYTE*)&gSystem.Ppu.ObjectAttributeMemory + gSystem.Ppu.bObjectAttributeMemoryAddress) = bValue;

	return 0;
}

DWORD PpuAddressRegister_IncreaseV()
{
	// check address increase mode
	if(PpuControlFlag_Get(PPU_CONTROL_FLAG_ADDR_INCREASE_32) != 0)
	{
		// add 32 - next line
		gSystem.Ppu.Reg.wV += 32;
	}
	else
	{
		// add 1
		gSystem.Ppu.Reg.wV++;
	}

	return 0;
}

DWORD PpuAddressRegister_Set(PpuAddressRegisterTypeEnum PpuAddressRegisterType, PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE bValue)
{
	WORD *pwRegisterPtr = NULL;
	WORD wOrigValue = 0;
	WORD wNewValue = 0;

	// check address register type (V / T)
	if(PpuAddressRegisterType == PPU_ADDRESS_REGISTER_TYPE_V)
	{
		// V register
		pwRegisterPtr = &gSystem.Ppu.Reg.wV;
	}
	else if(PpuAddressRegisterType == PPU_ADDRESS_REGISTER_TYPE_T)
	{
		// T register
		pwRegisterPtr = &gSystem.Ppu.Reg.wT;
	}
	else
	{
		return 1;
	}

	// store original value
	wOrigValue = *pwRegisterPtr;

	// check field type
	if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_ADDR_LOW)
	{
		// current address low
		wNewValue = wOrigValue & ~0xFF;
		wNewValue |= bValue;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_ADDR_HIGH)
	{
		// current address high
		wNewValue = wOrigValue & ~0x7F00;
		wNewValue |= (bValue & 0x3F) << 8;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE)
	{
		// name table selection (X and Y)
		wNewValue = wOrigValue & ~0xC00;
		wNewValue |= (bValue & 0x3) << 10;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X)
	{
		// name table X
		wNewValue = wOrigValue & ~0x400;
		wNewValue |= (bValue & 0x1) << 10;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y)
	{
		// name table Y
		wNewValue = wOrigValue & ~0x800;
		wNewValue |= (bValue & 0x1) << 11;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X)
	{
		// tile X position
		wNewValue = wOrigValue & ~0x1F;
		wNewValue |= (bValue & 0x1F);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y)
	{
		// tile Y position
		wNewValue = wOrigValue & ~0x3E0;
		wNewValue |= (bValue & 0x1F) << 5;
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y)
	{
		// tile Y offset
		wNewValue = wOrigValue & ~0x7000;
		wNewValue |= (bValue & 0x7) << 12;
	}
	else
	{
		return 1;
	}

	*pwRegisterPtr = wNewValue;

	return 0;
}

DWORD PpuAddressRegister_Get(PpuAddressRegisterTypeEnum PpuAddressRegisterType, PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE *pbValue)
{
	WORD wRegisterValue = NULL;
	BYTE bValue = 0;

	// check address register type (V / T)
	if(PpuAddressRegisterType == PPU_ADDRESS_REGISTER_TYPE_V)
	{
		// V register
		wRegisterValue = gSystem.Ppu.Reg.wV;
	}
	else if(PpuAddressRegisterType == PPU_ADDRESS_REGISTER_TYPE_T)
	{
		// T register
		wRegisterValue = gSystem.Ppu.Reg.wT;
	}
	else
	{
		return 1;
	}

	if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_ADDR_LOW)
	{
		// current address low
		bValue = (BYTE)(wRegisterValue & 0xFF);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_ADDR_HIGH)
	{
		// current address high
		bValue = (BYTE)((wRegisterValue >> 8) & 0x3F);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE)
	{
		// name table selection (X and Y)
		bValue = (BYTE)((wRegisterValue >> 10) & 0x3);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X)
	{
		// name table X
		bValue = (BYTE)((wRegisterValue >> 10) & 0x1);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y)
	{
		// name table Y
		bValue = (BYTE)((wRegisterValue >> 11) & 0x1);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X)
	{
		// tile X position
		bValue = (BYTE)(wRegisterValue & 0x1F);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y)
	{
		// tile Y position
		bValue = (BYTE)((wRegisterValue >> 5) & 0x1F);
	}
	else if(PpuAddressRegisterField == PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y)
	{
		// tile Y offset
		bValue = (BYTE)((wRegisterValue >> 12) & 0x7);
	}
	else
	{
		return 1;
	}

	*pbValue = bValue;

	return 0;
}

DWORD PpuAddressRegister_V_Get(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE *pbValue)
{
	// get V register field value
	if(PpuAddressRegister_Get(PPU_ADDRESS_REGISTER_TYPE_V, PpuAddressRegisterField, pbValue) != 0)
	{
		return 1;
	}

	return 0;
}

DWORD PpuAddressRegister_V_Set(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE bValue)
{
	// set V register field value
	if(PpuAddressRegister_Set(PPU_ADDRESS_REGISTER_TYPE_V, PpuAddressRegisterField, bValue) != 0)
	{
		return 1;
	}

	return 0;
}

DWORD PpuAddressRegister_T_Get(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE *pbValue)
{
	// get T register field value
	if(PpuAddressRegister_Get(PPU_ADDRESS_REGISTER_TYPE_T, PpuAddressRegisterField, pbValue) != 0)
	{
		return 1;
	}

	return 0;
}

DWORD PpuAddressRegister_T_Set(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE bValue)
{
	// set T register field value
	if(PpuAddressRegister_Set(PPU_ADDRESS_REGISTER_TYPE_T, PpuAddressRegisterField, bValue) != 0)
	{
		return 1;
	}

	return 0;
}
