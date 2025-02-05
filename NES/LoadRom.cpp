#include "NES.h"

DWORD LoadFileIntoMemory(char *pPath, BYTE **pFileData, DWORD *pdwFileSize)
{
	BYTE bError = 0;
	HANDLE hFile = NULL;
	DWORD dwFileSize = 0;
	DWORD dwFileSizeHigh = 0;
	BYTE *pFileDataBuffer = NULL;
	DWORD dwBytesRead = 0;

	// open file
	hFile = CreateFileA(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		// failed - reset hFile to NULL
		hFile = NULL;
		ERROR_CLEAN_UP(1);
	}

	// calculate file size
	dwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
	if(dwFileSizeHigh != 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// allocate buffer
	pFileDataBuffer = (BYTE*)malloc(dwFileSize);
	if(pFileDataBuffer == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// read file contents
	if(ReadFile(hFile, pFileDataBuffer, dwFileSize, &dwBytesRead, NULL) == 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// verify byte count
	if(dwBytesRead != dwFileSize)
	{
		ERROR_CLEAN_UP(1);
	}

	// store values
	*pFileData = pFileDataBuffer;
	*pdwFileSize = dwFileSize;

	// clean up
CLEAN_UP:
	if(pFileDataBuffer != NULL)
	{
		// only free output data on failure
		if(bError != 0)
		{
			free(pFileDataBuffer);
		}
	}
	if(hFile != NULL)
	{
		CloseHandle(hFile);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD LoadROM(char *pFilePath)
{
	BYTE bError = 0;
	BYTE *pFileData = NULL;
	BYTE *pPrgRomFileBase = NULL;
	BYTE *pChrRomFileBase = NULL;
	DWORD dwFileSize = 0;
	DWORD dwMinimumFileSize = 0;
	DWORD dwPrgRomTotalSize = 0;
	DWORD dwChrRomTotalSize = 0;
	BYTE bMapperIndex = 0;
	BYTE bSignature[4] = { 'N', 'E', 'S', 0x1A };
	NesRomFileHeaderStruct *pNesRomFileHeader = NULL;

	// load entire rom file into memory
	if(LoadFileIntoMemory(pFilePath, &pFileData, &dwFileSize) != 0)
	{
		printf("Error: Failed to open file\n");
		ERROR_CLEAN_UP(1);
	}

	// validate file size
	if(dwFileSize < sizeof(NesRomFileHeaderStruct))
	{
		ERROR_CLEAN_UP(1);
	}

	// get rom header
	pNesRomFileHeader = (NesRomFileHeaderStruct*)pFileData;
	if(memcmp(pNesRomFileHeader->bSignature, bSignature, sizeof(bSignature)) != 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// calculate block sizes
	dwPrgRomTotalSize = pNesRomFileHeader->bPrgRomBlockCount * 0x4000;
	dwChrRomTotalSize = pNesRomFileHeader->bChrRomBlockCount * 0x2000;

	// validate file size
	dwMinimumFileSize = sizeof(NesRomFileHeaderStruct) + dwPrgRomTotalSize + dwChrRomTotalSize;
	if(dwFileSize < dwMinimumFileSize)
	{
		ERROR_CLEAN_UP(1);
	}

	// get mapper index
	bMapperIndex = (pNesRomFileHeader->bFlags[1] & 0xF0) + (pNesRomFileHeader->bFlags[0] >> 4);
	if(bMapperIndex != 0)
	{
		printf("Error: Mapper %u not supported\n", bMapperIndex);
		ERROR_CLEAN_UP(1);
	}

	// ensure trainer is not present
	if(pNesRomFileHeader->bFlags[0] & 0x4)
	{
		ERROR_CLEAN_UP(1);
	}

	// get prg-rom region
	pPrgRomFileBase = pFileData + sizeof(NesRomFileHeaderStruct);
	if(pNesRomFileHeader->bPrgRomBlockCount == 1)
	{
		// 1 prg-rom block - copy duplicate block into memory twice
		Memory_WriteRange(0x8000, pPrgRomFileBase, 0x4000);
		Memory_WriteRange(0xC000, pPrgRomFileBase, 0x4000);
	}
	else if(pNesRomFileHeader->bPrgRomBlockCount == 2)
	{
		// 2 prg-rom blocks - copy blocks into memory
		Memory_WriteRange(0x8000, pPrgRomFileBase, 0x8000);
	}
	else
	{
		// invalid prg-rom block count
		ERROR_CLEAN_UP(1);
	}

	// get chr-rom region (optional)
	pChrRomFileBase = pPrgRomFileBase + dwPrgRomTotalSize;
	if(pNesRomFileHeader->bChrRomBlockCount != 0)
	{
		// rom contains chr-rom regions, validate count
		if(pNesRomFileHeader->bChrRomBlockCount == 1)
		{
			// copy pattern table
			PpuMemory_WriteRange(0, pChrRomFileBase, 0x2000);
		}
		else
		{
			// invalid chr-rom block count
			ERROR_CLEAN_UP(1);
		}
	}

	// get name table mirror mode
	if(pNesRomFileHeader->bFlags[0] & 0x1)
	{
		// vertical mode
		gSystem.Ppu.NameTableMirrorMode = NAME_TABLE_MIRROR_MODE_VERTICAL;
	}
	else
	{
		// horizontal mode
		gSystem.Ppu.NameTableMirrorMode = NAME_TABLE_MIRROR_MODE_HORIZONTAL;
	}

	// clean up
CLEAN_UP:
	if(pFileData != NULL)
	{
		free(pFileData);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}
