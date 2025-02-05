#include "..\NES.h"

DWORD ApuMemoryMappedRegisters_Write_SquareWave(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue)
{
	AudioChannelState_SquareWaveStruct *pSquareWaveState = NULL;
	BYTE bDutyCycle = 0;
	BYTE bConstantVolume = 0;
	BYTE bVolume = 0;
	BYTE bLoop = 0;
	BYTE bSweepEnabled = 0;
	BYTE bSweepInterval = 0;
	BYTE bNegate = 0;
	BYTE bShiftCount = 0;
	BYTE bNegateAdditional = 0;
	BYTE bTimerHigh = 0;
	BYTE bLengthTableIndex = 0;

	pSquareWaveState = &pAudioChannel->SquareWave;

	// handle apu register memory write
	if(wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_1_CTRL1 || wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_2_CTRL1)
	{
		bDutyCycle = (bValue >> 6) & 3;
		bConstantVolume = (bValue >> 4) & 1;
		bVolume = bValue & 0xF;
		bLoop = (bValue >> 5) & 1;

		pSquareWaveState->bCurrSquareWaveSequenceMode = bDutyCycle;
		if(bConstantVolume != 0)
		{
			// constant volume
			pAudioChannel->bVolume = bVolume;
			pSquareWaveState->VolumeDecay.bEnabled = 0;
			pSquareWaveState->LengthCounter.bHalt = bLoop;
		}
		else
		{
			// set volume decay
			ApuVolumeDecay_Set(&pSquareWaveState->VolumeDecay, bVolume, bLoop);
		}
	}
	else if(wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_1_CTRL2 || wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_2_CTRL2)
	{
		bSweepEnabled = (bValue >> 7) & 1;
		bSweepInterval = (bValue >> 4) & 7;
		bNegate = (bValue >> 3) & 1;
		bShiftCount = bValue & 7;

		if(bSweepEnabled == 0)
		{
			pSquareWaveState->Sweep.bEnabled = 0;
		}
		else
		{
			// sweep enabled
			bNegateAdditional = 0;
			if(pAudioChannel->bAudioChannelIndex == 2)
			{
				bNegateAdditional = 1;
			}
			ApuSweep_Set(&pSquareWaveState->Sweep, bSweepInterval, bNegate, bNegateAdditional, bShiftCount);
		}
	}
	else if(wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_1_CTRL3 || wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_2_CTRL3)
	{
		// set timer low value
		*(BYTE*)&pSquareWaveState->wFrequencyTimer = bValue;
	}
	else if(wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_1_CTRL4 || wApuRegisterAddress == APU_REGISTER_SQUARE_WAVE_2_CTRL4)
	{
		bTimerHigh = bValue & 7;
		bLengthTableIndex = (bValue >> 3) & 0x1F;
		
		*(BYTE*)((BYTE*)&pSquareWaveState->wFrequencyTimer + 1) = bTimerHigh;
		if(pAudioChannel->bChannelEnabled != 0)
		{
			ApuLengthCounter_Set(&pSquareWaveState->LengthCounter, bLengthTableIndex);
		}
		if(pSquareWaveState->VolumeDecay.bEnabled != 0)
		{
			// reset volume envelope counter
			pSquareWaveState->VolumeDecay.bResetFlag = 1;
		}

		// reset phase
		pSquareWaveState->bResetPhase = 1;
		pSquareWaveState->dwCurrPhaseSamplesRemaining = 0;
	}
	else
	{
		return 1;
	}

	return 0;
}

DWORD ApuQuarterFrame_SquareWave(AudioChannelStateStruct *pAudioChannel)
{
	// process volume decay
	ApuVolumeDecay_Process(&pAudioChannel->SquareWave.VolumeDecay);

	return 0;
}

DWORD ApuHalfFrame_SquareWave(AudioChannelStateStruct *pAudioChannel)
{
	// process length counter
	ApuLengthCounter_Process(&pAudioChannel->SquareWave.LengthCounter);

	// process sweep
	ApuSweep_Process(&pAudioChannel->SquareWave.Sweep);

	return 0;
}

BYTE GetNextSample_SquareWave(AudioChannelStateStruct *pAudioChannel)
{
	AudioChannelState_SquareWaveStruct *pSquareWaveState = NULL;
	DWORD dwOutputFrequencyHz = 0;
	DWORD dwTotalSamples = 0;
	DWORD dwSampleRemainder = 0;
	BYTE bCurrIndex = 0;
	BYTE bSample = 0;
	BYTE bReset = 0;
	SquareWaveSequenceStruct SquareWaveSequenceModeList[4] =
	{
		// duty mode 0
		{ 0, 1, 0, 0, 0, 0, 0, 0 },
		// duty mode 1
		{ 0, 1, 1, 0, 0, 0, 0, 0 },
		// duty mode 2
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		// duty mode 3
		{ 1, 0, 0, 1, 1, 1, 1, 1 }
	};

	pSquareWaveState = &pAudioChannel->SquareWave;

	if(pSquareWaveState->wFrequencyTimer < 8)
	{
		// not playing
		return 0;
	}

	if(pSquareWaveState->LengthCounter.bCurrCounter == 0)
	{
		// not playing
		return 0;
	}

	if(pSquareWaveState->dwCurrPhaseSamplesRemaining == 0)
	{
		if(pSquareWaveState->bResetPhase != 0)
		{
			// reset flag set
			bReset = 1;
			pSquareWaveState->bResetPhase = 0;
		}
		else
		{
			// increase phase index
			pSquareWaveState->bCurrSquareWavePhaseIndex++;
			if(pSquareWaveState->bCurrSquareWavePhaseIndex == 8)
			{
				bReset = 1;
			}
		}

		if(bReset != 0)
		{
			// reset phase
			pSquareWaveState->bCurrSquareWavePhaseIndex = 0;

			// calculate native output frequency
			dwOutputFrequencyHz = (gSystem.RegionSpecificSettings.dwMasterClockSpeedHz / gSystem.RegionSpecificSettings.dwApuCyclePeriod) / (8 * (pSquareWaveState->wFrequencyTimer + 1));
			dwTotalSamples = AUDIO_SAMPLE_RATE / dwOutputFrequencyHz;

			// calculate number of samples for each phase
			pSquareWaveState->dwCurrPhaseSamples = dwTotalSamples / sizeof(SquareWaveSequenceModeList[0].bSequence);
			dwSampleRemainder = dwTotalSamples % sizeof(SquareWaveSequenceModeList[0].bSequence);

			// some samples are lost due to rounding, distribute these remaining samples evenly across the phases
			memset(pSquareWaveState->bCurrSequenceAdditionalSamples, 0, sizeof(pSquareWaveState->bCurrSequenceAdditionalSamples));
			for(DWORD i = 0; i < dwSampleRemainder; i++)
			{
				bCurrIndex = (BYTE)((i * sizeof(pSquareWaveState->bCurrSequenceAdditionalSamples)) / dwSampleRemainder);
				pSquareWaveState->bCurrSequenceAdditionalSamples[bCurrIndex] = 1;
			}
		}

		pSquareWaveState->bCurrSampleOn = SquareWaveSequenceModeList[pSquareWaveState->bCurrSquareWaveSequenceMode].bSequence[pSquareWaveState->bCurrSquareWavePhaseIndex];

		// calculate number of samples for this phase
		pSquareWaveState->dwCurrPhaseSamplesRemaining = pSquareWaveState->dwCurrPhaseSamples;
		if(pSquareWaveState->bCurrSequenceAdditionalSamples[pSquareWaveState->bCurrSquareWavePhaseIndex] != 0)
		{
			pSquareWaveState->dwCurrPhaseSamplesRemaining++;
		}
	}

	pSquareWaveState->dwCurrPhaseSamplesRemaining--;

	if(pSquareWaveState->bCurrSampleOn != 0)
	{
		bSample = pAudioChannel->bVolume;
	}
	else
	{
		bSample = 0;
	}

	return bSample;
}
