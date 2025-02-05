#include "NES.h"

DWORD ProcessWaitingCycles()
{
	for(UINT64 i = 0; i < gSystem.MasterClock.qwCyclesWaiting; i++)
	{
		if(gSystem.DisplayWindow.bWindowClosed != 0)
		{
			// window closed, shutting down
			return 1;
		}

		// check CPU
		gSystem.MasterClock.bCpuCycle++;
		if(gSystem.MasterClock.bCpuCycle >= gSystem.RegionSpecificSettings.dwCpuCyclePeriod)
		{
			// cycle CPU
			if(CycleCpu() != 0)
			{
				return 1;
			}
			gSystem.MasterClock.bCpuCycle = 0;
		}

		// check PPU
		gSystem.MasterClock.bPpuCycle++;
		if(gSystem.MasterClock.bPpuCycle >= gSystem.RegionSpecificSettings.dwPpuCyclePeriod)
		{
			// cycle PPU
			if(CyclePpu() != 0)
			{
				return 1;
			}
			gSystem.MasterClock.bPpuCycle = 0;
		}

		// check APU
		gSystem.MasterClock.bApuCycle++;
		if(gSystem.MasterClock.bApuCycle >= gSystem.RegionSpecificSettings.dwApuCyclePeriod)
		{
			// cycle APU
			if(CycleApu() != 0)
			{
				return 1;
			}
			gSystem.MasterClock.bApuCycle = 0;
		}
	}

	// reset main counter
	gSystem.MasterClock.qwCyclesWaiting = 0;

	return 0;
}

VOID CALLBACK MasterClockTimer(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	BYTE bStopTimer = 0;
	LARGE_INTEGER LargeInt;
	UINT64 qwCurrCounter = 0;
	UINT64 qwClockDelta = 0;
	UINT64 qwCycleCount = 0;

	// get counter
	QueryPerformanceCounter(&LargeInt);
	qwCurrCounter = LargeInt.QuadPart;

	if(gSystem.MasterClock.bReady != 0)
	{
		// calculate clock delta (including remainder from previous batch)
		qwClockDelta = (gSystem.RegionSpecificSettings.dwMasterClockSpeedHz * (qwCurrCounter - gSystem.MasterClock.qwPrevCounter)) + gSystem.MasterClock.qwLastRemainder;

		// calculate number of full cycles
		qwCycleCount = qwClockDelta / gSystem.MasterClock.qwFrequency;

		// store remainder, carry over to next block
		gSystem.MasterClock.qwLastRemainder = qwClockDelta % gSystem.MasterClock.qwFrequency;

		// increase cycle counter
		gSystem.MasterClock.qwCyclesWaiting += qwCycleCount;

		// execute current batch
		if(ProcessWaitingCycles() != 0)
		{
			// error / shutting down
			bStopTimer = 1;
		}
	}
	else
	{
		// ready
		gSystem.MasterClock.bReady = 1;
	}

	// store current value
	gSystem.MasterClock.qwPrevCounter = qwCurrCounter;

	if(bStopTimer != 0)
	{
		// timer stopped - set event
		SetEvent((HANDLE)dwUser);

		// stop timer
		timeKillEvent(uTimerID);
	}
}

DWORD MasterClockLoop()
{
	BYTE bError = 0;
	LARGE_INTEGER PerformanceCounterFrequency;
	HANDLE hTimerStoppedEvent = NULL;

	// get frequency
	QueryPerformanceFrequency(&PerformanceCounterFrequency);
	gSystem.MasterClock.qwFrequency = PerformanceCounterFrequency.QuadPart;

	// create timer stopped event
	hTimerStoppedEvent = CreateEvent(NULL, 0, 0, NULL);
	if(hTimerStoppedEvent == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// start master clock timer (1ms interval)
	if(timeSetEvent(1, 1, MasterClockTimer, (DWORD_PTR)hTimerStoppedEvent, TIME_PERIODIC) == 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// wait for master clock timer to stop (the timer terminates itself via timeKillEvent internally)
	WaitForSingleObject(hTimerStoppedEvent, INFINITE);

	// clean up
CLEAN_UP:
	if(hTimerStoppedEvent != NULL)
	{
		CloseHandle(hTimerStoppedEvent);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}
