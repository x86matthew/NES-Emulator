#include "NES.h"

#pragma comment(lib, "winmm.lib")

SystemStateStruct gSystem;

DWORD InitialiseSystem(char *pRomPath, RegionTypeEnum RegionType)
{
	// clear system state
	memset(&gSystem, 0, sizeof(gSystem));
	Memory_Reset();

	printf("Loading ROM...\n");

	// load ROM file
	if(LoadROM(pRomPath) != 0)
	{
		printf("Error: Failed to load ROM (%s)\n", pRomPath);
		return 1;
	}

	// create shutdown event object (manual reset because multiple threads check this event)
	gSystem.hShutDownEvent = CreateEvent(NULL, 1, 0, NULL);
	if(gSystem.hShutDownEvent == NULL)
	{
		return 1;
	}

	// initialise region-specific settings (NTSC / PAL)
	if(InitialiseRegionSpecificSettings(RegionType) != 0)
	{
		return 1;
	}

	// initialise win32 display window
	if(InitialiseDisplayWindow() != 0)
	{
		return 1;
	}

	// initialise audio output
	if(InitialiseAudioChannels() != 0)
	{
		return 1;
	}

	// initialise input keys
	if(InitialiseInputKeyMappings() != 0)
	{
		return 1;
	}

	// initialise opcode table
	if(PopulateOpcodeLookupTable() != 0)
	{
		return 1;
	}

	// initialise cpu state
	gSystem.Cpu.pCurrInstruction = NULL;
	Memory_Read16(INTERRUPT_VECTOR_RESET, &gSystem.Cpu.Reg.wPC);
	gSystem.Cpu.Reg.bSP = INITIAL_STACK_PTR;
	StatusFlag_Set(CPU_STATUS_FLAG_INTERRUPT_DISABLE);
	StatusFlag_Set(CPU_STATUS_FLAG_ALWAYS_ON);

	return 0;
}

DWORD CloseSystem_CloseHandle(HANDLE *phHandle)
{
	if(*phHandle != NULL)
	{
		// close handle
		CloseHandle(*phHandle);
		*phHandle = NULL;
	}

	return 0;
}

DWORD CloseSystem_DeleteDC(HDC *phDC)
{
	if(*phDC != NULL)
	{
		// delete DC
		DeleteDC(*phDC);
		*phDC = NULL;
	}

	return 0;
}

DWORD CloseSystem_DestroyWindow(HWND *phWnd)
{
	if(*phWnd != NULL)
	{
		// destroy window
		DestroyWindow(*phWnd);
		*phWnd = NULL;
	}

	return 0;
}

DWORD CloseSystem_DeleteCriticalSection(CRITICAL_SECTION *pCriticalSection, BYTE *pbCriticalSectionReady)
{
	if(*pbCriticalSectionReady != 0)
	{
		// delete critical section object
		DeleteCriticalSection(pCriticalSection);
		*pbCriticalSectionReady = 0;
	}

	return 0;
}

DWORD CloseSystem_WaitForThreadExit(HANDLE hThread)
{
	if(hThread != NULL)
	{
		// wait for thread to exit
		WaitForSingleObject(hThread, INFINITE);
	}

	return 0;
}

DWORD CloseSystem()
{
	// shutting down, wait for threads
	if(gSystem.hShutDownEvent != NULL)
	{
		SetEvent(gSystem.hShutDownEvent);
	}
	CloseSystem_WaitForThreadExit(gSystem.DisplayWindow.hThread);
	CloseSystem_WaitForThreadExit(gSystem.DisplayWindow.hRedrawBitmapThread);
	CloseSystem_WaitForThreadExit(gSystem.Apu.hThread);

	// clean up all objects in gSystem
	CloseSystem_CloseHandle(&gSystem.DisplayWindow.hThread);
	CloseSystem_CloseHandle(&gSystem.DisplayWindow.hRedrawBitmapThread);
	CloseSystem_CloseHandle(&gSystem.Apu.hThread);
	CloseSystem_CloseHandle(&gSystem.Apu.hOutputBufferEvent);
	CloseSystem_CloseHandle(&gSystem.hShutDownEvent);
	CloseSystem_DeleteDC(&gSystem.DisplayWindow.hBitmapDC);
	CloseSystem_DestroyWindow(&gSystem.DisplayWindow.hWnd);
	CloseSystem_DeleteCriticalSection(&gSystem.Apu.OutputBufferCriticalSection, &gSystem.Apu.bOutputBufferCriticalSectionReady);
	CloseSystem_DeleteCriticalSection(&gSystem.DisplayWindow.CurrentFrameBufferCriticalSection, &gSystem.DisplayWindow.bCurrentFrameBufferCriticalSectionReady);

	return 0;
}

DWORD StartNesEmulator(char *pRomPath, RegionTypeEnum RegionType)
{
	BYTE bError = 0;

	// initialise system
	if(InitialiseSystem(pRomPath, RegionType) != 0)
	{
		printf("Error: Failed to initialise system\n");
		ERROR_CLEAN_UP(1);
	}

	printf("Ready\n");

	// begin master clock loop
	MasterClockLoop();

	// clean up
CLEAN_UP:
	CloseSystem();
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *pRomPathParam = NULL;
	char *pRegionParam = NULL;
	RegionTypeEnum RegionType;
	BYTE bInvalidParams = 0;

	printf("NES Emulator\n");
	printf("- x86matthew\n\n");

	if(argc == 2)
	{
		// region not specified - use default (NTSC)
		pRomPathParam = argv[1];
		RegionType = REGION_NTSC;
	}
	else if(argc == 3)
	{
		pRegionParam = argv[1];
		pRomPathParam = argv[2];

		// validate region param
		if(strcmp(pRegionParam, "-ntsc") == 0)
		{
			// NTSC
			RegionType = REGION_NTSC;
		}
		else if(strcmp(pRegionParam, "-pal") == 0)
		{
			// PAL
			RegionType = REGION_PAL;
		}
		else
		{
			// invalid
			bInvalidParams = 1;
		}
	}
	else
	{
		// invalid param count
		bInvalidParams = 1;
	}

	if(bInvalidParams != 0)
	{
		// print usage
		printf("Usage: %s [-ntsc / -pal] <rom_path>\n\n", argv[0]);
		return 1;
	}

	// start emulator
	StartNesEmulator(pRomPathParam, RegionType);

	return 0;
}
