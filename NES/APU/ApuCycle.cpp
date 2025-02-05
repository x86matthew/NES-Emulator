#include "..\NES.h"

DWORD CycleApu_SingleCycle()
{
	AudioChannelStateStruct *pAudioChannel = NULL;

	// process cycle
	for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		pAudioChannel = GetAudioChannel((BYTE)(i + 1));
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		if(pAudioChannel->pCycleHandler != NULL)
		{
			// call handler for the current channel
			pAudioChannel->pCycleHandler(pAudioChannel);
		}
	}

	return 0;
}

DWORD CycleApu_QuarterFrame()
{
	AudioChannelStateStruct *pAudioChannel = NULL;

	// process quarter frame
	for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		pAudioChannel = GetAudioChannel((BYTE)(i + 1));
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		if(pAudioChannel->pQuarterFrameHandler != NULL)
		{
			// call handler for the current channel
			pAudioChannel->pQuarterFrameHandler(pAudioChannel);
		}
	}

	return 0;
}

DWORD CycleApu_HalfFrame()
{
	AudioChannelStateStruct *pAudioChannel = NULL;

	// process half frame
	for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		pAudioChannel = GetAudioChannel((BYTE)(i + 1));
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		if(pAudioChannel->pHalfFrameHandler != NULL)
		{
			// call handler for the current channel
			pAudioChannel->pHalfFrameHandler(pAudioChannel);
		}
	}

	return 0;
}

DWORD CycleApu_CheckOutputSample()
{
	DWORD dwScaledCyclesPerOutputSample = 0;

	// the NES generates an audio sample on every APU cycle (or 2 samples per APU cycle for the triangle wave channel).
	// these samples must be converted to 48000Hz format for output.
	// as the values don't divide evenly, the values are scaled up and an accumulator stores the partial remainders.
	// (there are roughly 18.4 native samples for every 48000Hz output sample on NTSC)

	// calculate number of scaled cycles per output sample
	dwScaledCyclesPerOutputSample = (((gSystem.RegionSpecificSettings.dwMasterClockSpeedHz / gSystem.RegionSpecificSettings.dwApuCyclePeriod) * APU_SCALED_SAMPLE_COUNTER_MULTIPLIER) / AUDIO_SAMPLE_RATE);

	// increase scaled counter
	gSystem.Apu.dwScaledSampleCounter += APU_SCALED_SAMPLE_COUNTER_MULTIPLIER;
	if(gSystem.Apu.dwScaledSampleCounter >= dwScaledCyclesPerOutputSample)
	{
		// generate next output sample
		if(ApuGenerateOutputSample() != 0)
		{
			return 1;
		}

		// update counter
		gSystem.Apu.dwScaledSampleCounter -= dwScaledCyclesPerOutputSample;
	}

	return 0;
}

DWORD CycleApu()
{
	DWORD dwTargetCount = 0;
	DWORD dwTotalCyclesPerFrame = 0;

	// set target cycle count
	dwTargetCount = gSystem.RegionSpecificSettings.dwApuQuarterFrameCycleCount;

	// adjust cycle count for the last quarter-frame to balance the total correctly
	if(gSystem.Apu.bQuarterFrameIndex == 3)
	{
		if(gSystem.Apu.b5StepFrameMode != 0)
		{
			// 5-step mode
			dwTotalCyclesPerFrame = gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_5Step;
		}
		else
		{
			// 4-step mode
			dwTotalCyclesPerFrame = gSystem.RegionSpecificSettings.dwApuFullFrameCycleCount_4Step;
		}

		// increase target cycle count
		dwTargetCount += dwTotalCyclesPerFrame - (gSystem.RegionSpecificSettings.dwApuQuarterFrameCycleCount * 4);
	}

	// quarter-frame
	CycleApu_SingleCycle();

	// increase cycle counter
	gSystem.Apu.dwFrameCycleCounter++;
	if(gSystem.Apu.dwFrameCycleCounter == dwTargetCount)
	{
		// quarter-frame
		CycleApu_QuarterFrame();

		// reset counter
		gSystem.Apu.dwFrameCycleCounter = 0;

		// increase quarter-frame index
		gSystem.Apu.bQuarterFrameIndex++;
		if((gSystem.Apu.bQuarterFrameIndex % 2) == 0)
		{
			// half-frame
			CycleApu_HalfFrame();

			// check if this is the end of the frame
			if(gSystem.Apu.bQuarterFrameIndex == 4)
			{
				// reset index
				gSystem.Apu.bQuarterFrameIndex = 0;
			}
		}
	}

	// check if an output sample should be generated
	CycleApu_CheckOutputSample();

	return 0;
}
