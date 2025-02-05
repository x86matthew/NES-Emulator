#include "NES.h"

DWORD Memory_FixAddress(WORD wAddress, WORD *pwFixedAddress)
{
	WORD wFixedAddress = 0;

	if(wAddress < 0x2000)
	{
		// internal RAM - first 2KB (0x0 -> 0x7FF) is mirrored 4 times
		wFixedAddress = wAddress % 2048;
	}
	else if(wAddress < 0x4000)
	{
		// PPU registers - first 8 bytes (0x2000 -> 0x2007) are mirrored 1024 times, only write to the first block
		wFixedAddress = 0x2000 + (wAddress % 8);
	}
	else
	{
		// use original address
		wFixedAddress = wAddress;
	}

	*pwFixedAddress = wFixedAddress;

	return 0;
}

DWORD Memory_Read8(WORD wAddress, BYTE *pbValue)
{
	WORD wFixedAddress = 0;
	BYTE bHandled = 0;
	DWORD (*pFunctionList[])(WORD wAddress, BYTE *pbValue) =
	{
		PpuMemoryMappedRegisters_Read,
		ApuMemoryMappedRegisters_Read,
		InputMemoryMappedRegisters_Read,
	};

	// some memory addresses are shared with ppu/apu/input - allow them to handle the read first if necessary
	Memory_FixAddress(wAddress, &wFixedAddress);
	for(DWORD i = 0; i < ELEMENT_COUNT(pFunctionList); i++)
	{
		if(pFunctionList[i](wFixedAddress, pbValue) != 0)
		{
			// not handled, try next
			continue;
		}

		bHandled = 1;
		break;
	}

	if(bHandled == 0)
	{
		// read from main memory
		*pbValue = gSystem.bMemory[wFixedAddress];
	}

	return 0;
}

DWORD Memory_Write8(WORD wAddress, BYTE bValue)
{
	WORD wFixedAddress = 0;
	BYTE bHandled = 0;
	DWORD (*pFunctionList[])(WORD wAddress, BYTE bValue) =
	{
		PpuMemoryMappedRegisters_Write,
		ApuMemoryMappedRegisters_Write,
		InputMemoryMappedRegisters_Write,
	};

	// some memory addresses are shared with ppu/apu/input - allow them to handle the write first if necessary
	Memory_FixAddress(wAddress, &wFixedAddress);
	for(DWORD i = 0; i < ELEMENT_COUNT(pFunctionList); i++)
	{
		if(pFunctionList[i](wFixedAddress, bValue) != 0)
		{
			// not handled, try next
			continue;
		}

		bHandled = 1;
		break;
	}

	if(bHandled == 0)
	{
		// write to main memory
		gSystem.bMemory[wFixedAddress] = bValue;
	}

	return 0;
}

DWORD Memory_ReadRange(WORD wAddress, VOID *pData, DWORD dwLength)
{
	WORD wCurrAddress = 0;
	BYTE *pCurrPtr = NULL;

	// read memory range
	wCurrAddress = wAddress;
	pCurrPtr = (BYTE*)pData;
	for(DWORD i = 0; i < dwLength; i++)
	{
		Memory_Read8(wCurrAddress, pCurrPtr);
		wCurrAddress++;
		pCurrPtr++;
	}

	return 0;
}

DWORD Memory_WriteRange(WORD wAddress, VOID *pData, DWORD dwLength)
{
	WORD wCurrAddress = 0;
	BYTE *pCurrPtr = NULL;

	// write memory range
	wCurrAddress = wAddress;
	pCurrPtr = (BYTE*)pData;
	for(DWORD i = 0; i < dwLength; i++)
	{
		Memory_Write8(wCurrAddress, *pCurrPtr);
		wCurrAddress++;
		pCurrPtr++;
	}

	return 0;
}

DWORD Memory_Read16(WORD wAddress, WORD *pwValue)
{
	// read 16-bit value
	Memory_ReadRange(wAddress, pwValue, sizeof(WORD));

	return 0;
}

DWORD Memory_Reset()
{
	// reset memory
	memset(gSystem.bMemory, 0, sizeof(gSystem.bMemory));

	return 0;
}
