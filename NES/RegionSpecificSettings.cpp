#include "NES.h"

DWORD InitialiseRegionSpecificSettings(RegionTypeEnum RegionType)
{
	DeltaModulationOutputRateTableStruct DeltaModulationOutputRateTable_NTSC =
	{
		{ 214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27 }
	};
	DeltaModulationOutputRateTableStruct DeltaModulationOutputRateTable_PAL =
	{
		{ 199, 177, 158, 149, 138, 118, 105, 99, 88, 74, 66, 59, 49, 39, 33, 25 }
	};
	NoiseFrequencyTimerTableStruct NoiseFrequencyTimerTable_NTSC =
	{
		{ 2, 4, 8, 16, 32, 48, 64, 80, 101, 127, 190, 254, 381, 508, 1017, 2034 }
	};
	NoiseFrequencyTimerTableStruct NoiseFrequencyTimerTable_PAL =
	{
		{ 2, 4, 7, 15, 30, 44, 59, 74, 94, 118, 177, 236, 354, 472, 945, 1889 }
	};

	// check current region type
	if(RegionType == REGION_NTSC)
	{
		// NTSC
		gSystem.RegionSpecificSettings.dwMasterClockSpeedHz = MASTER_CLOCK_SPEED_HZ_NTSC;
		gSystem.RegionSpecificSettings.dwCpuCyclePeriod = MASTER_CYCLES_PER_CPU_CYCLE_NTSC;
		gSystem.RegionSpecificSettings.dwPpuCyclePeriod = MASTER_CYCLES_PER_PPU_CYCLE_NTSC;
		gSystem.RegionSpecificSettings.dwPpuTotalScanLineCount = PPU_TOTAL_SCAN_LINE_COUNT_NTSC;
		gSystem.RegionSpecificSettings.bPpuSkipFirstIdleOddFramePreRender = 1;
		gSystem.RegionSpecificSettings.dwApuCyclePeriod = MASTER_CYCLES_PER_APU_CYCLE_NTSC;
		gSystem.RegionSpecificSettings.dwApuQuarterFrameCycleCount = APU_QUARTER_FRAME_CYCLE_COUNT_NTSC;
		gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_4Step = APU_FULL_FRAME_CYCLE_COUNT_4_STEP_NTSC;
		gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_5Step = APU_FULL_FRAME_CYCLE_COUNT_5_STEP_NTSC;
		memcpy(&gSystem.RegionSpecificSettings.ApuNoiseFrequencyTimerTable, &NoiseFrequencyTimerTable_NTSC, sizeof(NoiseFrequencyTimerTable_NTSC));
		memcpy(&gSystem.RegionSpecificSettings.ApuDeltaModulationOutputRateTable, &DeltaModulationOutputRateTable_NTSC, sizeof(DeltaModulationOutputRateTable_NTSC));
	}
	else if(RegionType == REGION_PAL)
	{
		// PAL
		gSystem.RegionSpecificSettings.dwMasterClockSpeedHz = MASTER_CLOCK_SPEED_HZ_PAL;
		gSystem.RegionSpecificSettings.dwCpuCyclePeriod = MASTER_CYCLES_PER_CPU_CYCLE_PAL;
		gSystem.RegionSpecificSettings.dwPpuCyclePeriod = MASTER_CYCLES_PER_PPU_CYCLE_PAL;
		gSystem.RegionSpecificSettings.dwPpuTotalScanLineCount = PPU_TOTAL_SCAN_LINE_COUNT_PAL;
		gSystem.RegionSpecificSettings.bPpuSkipFirstIdleOddFramePreRender = 0;
		gSystem.RegionSpecificSettings.dwApuCyclePeriod = MASTER_CYCLES_PER_APU_CYCLE_PAL;
		gSystem.RegionSpecificSettings.dwApuQuarterFrameCycleCount = APU_QUARTER_FRAME_CYCLE_COUNT_PAL;
		gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_4Step = APU_FULL_FRAME_CYCLE_COUNT_4_STEP_PAL;
		gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_5Step = APU_FULL_FRAME_CYCLE_COUNT_5_STEP_PAL;
		memcpy(&gSystem.RegionSpecificSettings.ApuNoiseFrequencyTimerTable, &NoiseFrequencyTimerTable_PAL, sizeof(NoiseFrequencyTimerTable_PAL));
		memcpy(&gSystem.RegionSpecificSettings.ApuDeltaModulationOutputRateTable, &DeltaModulationOutputRateTable_PAL, sizeof(DeltaModulationOutputRateTable_PAL));
	}
	else
	{
		return 1;
	}

	return 0;
}
