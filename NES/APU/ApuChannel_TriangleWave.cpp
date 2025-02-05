#include "..\NES.h"

DWORD ApuMemoryMappedRegisters_Write_TriangleWave(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue)
{
	AudioChannelState_TriangleWaveStruct *pTriangleWaveState = NULL;
	BYTE bControlFlag = 0;
	BYTE bLinearCounter = 0;
	BYTE bTimerHigh = 0;
	BYTE bLengthTableIndex = 0;

	pTriangleWaveState = &pAudioChannel->TriangleWave;

	// handle apu register memory write
	if(wApuRegisterAddress == APU_REGISTER_TRIANGLE_WAVE_CTRL1)
	{
		bControlFlag = (bValue >> 7) & 1;
		bLinearCounter = bValue & 0x7F;

		// set linear counter
		ApuLinearCounter_Set(&pTriangleWaveState->LinearCounter, bControlFlag, bLinearCounter);

		pTriangleWaveState->LengthCounter.bHalt = bControlFlag;
	}
	else if(wApuRegisterAddress == APU_REGISTER_TRIANGLE_WAVE_CTRL2)
	{
		// set timer low value
		*(BYTE*)&pTriangleWaveState->wFrequencyTimer = bValue;
	}
	else if(wApuRegisterAddress == APU_REGISTER_TRIANGLE_WAVE_CTRL3)
	{
		bTimerHigh = bValue & 7;
		bLengthTableIndex = (bValue >> 3) & 0x1F;

		*(BYTE*)((BYTE*)&pTriangleWaveState->wFrequencyTimer + 1) = bTimerHigh;
		if(pAudioChannel->bChannelEnabled != 0)
		{
			// set length counter
			ApuLengthCounter_Set(&pTriangleWaveState->LengthCounter, bLengthTableIndex);
		}

		// reset linear counter
		pTriangleWaveState->LinearCounter.bResetFlag = 1;

		// reset phase
		pTriangleWaveState->bResetPhase = 1;
		pTriangleWaveState->dwCurrPhaseSamplesRemaining = 0;
	}
	else
	{
		return 1;
	}

	return 0;
}

DWORD ApuQuarterFrame_TriangleWave(AudioChannelStateStruct *pAudioChannel)
{
	// process linear counter
	ApuLinearCounter_Process(&pAudioChannel->TriangleWave.LinearCounter);

	return 0;
}

DWORD ApuHalfFrame_TriangleWave(AudioChannelStateStruct *pAudioChannel)
{
	// process length counter
	ApuLengthCounter_Process(&pAudioChannel->TriangleWave.LengthCounter);

	return 0;
}

BYTE GetNextSample_TriangleWave(AudioChannelStateStruct *pAudioChannel)
{
	AudioChannelState_TriangleWaveStruct *pTriangleWaveState = NULL;
	DWORD dwOutputFrequencyHz = 0;
	DWORD dwTotalSamples = 0;
	DWORD dwSampleRemainder = 0;
	BYTE bCurrIndex = 0;
	BYTE bSample = 0;
	BYTE bReset = 0;
	BYTE bTriangleWaveSequence[32] =
	{
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	pTriangleWaveState = &pAudioChannel->TriangleWave;

	if(pTriangleWaveState->wFrequencyTimer == 0)
	{
		// not playing
		return 0;
	}

	if(pTriangleWaveState->LengthCounter.bCurrCounter == 0)
	{
		// not playing
		return 0;
	}

	if(pTriangleWaveState->LinearCounter.bCurrCounter == 0)
	{
		// not playing
		return 0;
	}

	if(pTriangleWaveState->dwCurrPhaseSamplesRemaining == 0)
	{
		if(pTriangleWaveState->bResetPhase != 0)
		{
			// reset flag set
			bReset = 1;
			pTriangleWaveState->bResetPhase = 0;
		}
		else
		{
			// increase phase index
			pTriangleWaveState->bCurrTriangleWavePhaseIndex++;
			if(pTriangleWaveState->bCurrTriangleWavePhaseIndex == 32)
			{
				bReset = 1;
			}
		}

		if(bReset != 0)
		{
			// reset phase
			pTriangleWaveState->bCurrTriangleWavePhaseIndex = 0;

			// calculate native output frequency (unlike other channels, the triangle channel outputs 2 samples per APU cycle)
			dwOutputFrequencyHz = (gSystem.RegionSpecificSettings.dwMasterClockSpeedHz / (gSystem.RegionSpecificSettings.dwApuCyclePeriod * 2)) / (8 * (pTriangleWaveState->wFrequencyTimer + 1));
			dwTotalSamples = AUDIO_SAMPLE_RATE / dwOutputFrequencyHz;

			// calculate number of samples for each phase
			pTriangleWaveState->dwCurrPhaseSamples = dwTotalSamples / sizeof(bTriangleWaveSequence);
			dwSampleRemainder = dwTotalSamples % sizeof(bTriangleWaveSequence);

			// some samples are lost due to rounding, distribute these remaining samples evenly across the phases
			memset(pTriangleWaveState->bCurrSequenceAdditionalSamples, 0, sizeof(pTriangleWaveState->bCurrSequenceAdditionalSamples));
			for(DWORD i = 0; i < dwSampleRemainder; i++)
			{
				bCurrIndex = (BYTE)((i * sizeof(pTriangleWaveState->bCurrSequenceAdditionalSamples)) / dwSampleRemainder);
				pTriangleWaveState->bCurrSequenceAdditionalSamples[bCurrIndex] = 1;
			}
		}

		// calculate number of samples for this phase
		pTriangleWaveState->dwCurrPhaseSamplesRemaining = pTriangleWaveState->dwCurrPhaseSamples;
		if(pTriangleWaveState->bCurrSequenceAdditionalSamples[pTriangleWaveState->bCurrTriangleWavePhaseIndex] != 0)
		{
			pTriangleWaveState->dwCurrPhaseSamplesRemaining++;
		}
	}

	pTriangleWaveState->dwCurrPhaseSamplesRemaining--;

	pAudioChannel->bVolume = bTriangleWaveSequence[pTriangleWaveState->bCurrTriangleWavePhaseIndex];

	bSample = pAudioChannel->bVolume;

	return bSample;
}
