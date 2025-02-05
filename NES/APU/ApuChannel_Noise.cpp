#include "..\NES.h"

DWORD ApuMemoryMappedRegisters_Write_Noise(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue)
{
	AudioChannelState_NoiseStruct *pNoiseState = NULL;
	BYTE bConstantVolume = 0;
	BYTE bVolume = 0;
	BYTE bLoop = 0;
	BYTE bShortMode = 0;
	BYTE bTimer = 0;
	BYTE bLengthTableIndex = 0;

	// handle apu register memory write
	pNoiseState = &pAudioChannel->Noise;
	if(wApuRegisterAddress == APU_REGISTER_NOISE_CTRL1)
	{
		bConstantVolume = (bValue >> 4) & 1;
		bVolume = bValue & 0xF;
		bLoop = (bValue >> 5) & 1;

		if(bConstantVolume != 0)
		{
			// constant volume
			pAudioChannel->bVolume = bVolume;
			pNoiseState->VolumeDecay.bEnabled = 0;
			pNoiseState->LengthCounter.bHalt = bLoop;
		}
		else
		{
			// set volume decay
			ApuVolumeDecay_Set(&pNoiseState->VolumeDecay, bVolume, bLoop);
		}
	}
	else if(wApuRegisterAddress == APU_REGISTER_NOISE_CTRL2)
	{
		bShortMode = (bValue >> 7) & 1;
		bTimer = bValue & 0xF;

		if(bTimer >= ELEMENT_COUNT(gSystem.RegionSpecificSettings.ApuNoiseFrequencyTimerTable.wFrequencyTimerTable))
		{
			return 1;
		}

		// set noise channel frequency
		pNoiseState->wFrequencyTimer = gSystem.RegionSpecificSettings.ApuNoiseFrequencyTimerTable.wFrequencyTimerTable[bTimer];
		pNoiseState->bShortMode = bShortMode;
		pNoiseState->dwCurrRandomSequenceIndex = 0;
		pNoiseState->dwCurrSamplesRemaining = 0;
	}
	else if(wApuRegisterAddress == APU_REGISTER_NOISE_CTRL3)
	{
		bLengthTableIndex = (bValue >> 3) & 0x1F;
		
		if(pAudioChannel->bChannelEnabled != 0)
		{
			// set length counter
			ApuLengthCounter_Set(&pNoiseState->LengthCounter, bLengthTableIndex);
		}
		if(pNoiseState->VolumeDecay.bEnabled != 0)
		{
			// reset volume envelope counter
			pNoiseState->VolumeDecay.bResetFlag = 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

DWORD ApuQuarterFrame_Noise(AudioChannelStateStruct *pAudioChannel)
{
	// process volume decay
	ApuVolumeDecay_Process(&pAudioChannel->Noise.VolumeDecay);

	return 0;
}

DWORD ApuHalfFrame_Noise(AudioChannelStateStruct *pAudioChannel)
{
	// process length counter
	ApuLengthCounter_Process(&pAudioChannel->Noise.LengthCounter);

	return 0;
}

BYTE GetNextSample_Noise(AudioChannelStateStruct *pAudioChannel)
{
	AudioChannelState_NoiseStruct *pNoiseState = NULL;
	BYTE bSample = 0;
	DWORD dwOutputFrequencyHz = 0;
	DWORD dwTotalSamples = 0;

	pNoiseState = &pAudioChannel->Noise;

	if(pNoiseState->wFrequencyTimer == 0)
	{
		// not playing
		return 0;
	}

	if(pNoiseState->LengthCounter.bCurrCounter == 0)
	{
		// not playing
		return 0;
	}

	if(pNoiseState->dwCurrSamplesRemaining == 0)
	{
		// calculate native output frequency
		dwOutputFrequencyHz = (gSystem.RegionSpecificSettings.dwMasterClockSpeedHz / gSystem.RegionSpecificSettings.dwApuCyclePeriod) / pNoiseState->wFrequencyTimer;
		dwTotalSamples = AUDIO_SAMPLE_RATE / dwOutputFrequencyHz;
		if(dwTotalSamples == 0)
		{
			// must be at least one sample
			dwTotalSamples = 1;
		}

		pNoiseState->dwCurrRandomSequenceIndex++;
		if(pNoiseState->bShortMode == 0)
		{
			// long sequence mode
			if(pNoiseState->dwCurrRandomSequenceIndex >= sizeof(gSystem.Apu.bNoiseRandomSequence_Long))
			{
				pNoiseState->dwCurrRandomSequenceIndex = 0;
			}

			pNoiseState->bCurrSampleOn = gSystem.Apu.bNoiseRandomSequence_Long[pNoiseState->dwCurrRandomSequenceIndex];
		}
		else
		{
			// short sequence mode
			if(pNoiseState->dwCurrRandomSequenceIndex >= sizeof(gSystem.Apu.bNoiseRandomSequence_Short))
			{
				pNoiseState->dwCurrRandomSequenceIndex = 0;
			}

			pNoiseState->bCurrSampleOn = gSystem.Apu.bNoiseRandomSequence_Short[pNoiseState->dwCurrRandomSequenceIndex];
		}

		pNoiseState->dwCurrSamplesRemaining = dwTotalSamples;
	}

	pNoiseState->dwCurrSamplesRemaining--;

	if(pNoiseState->bCurrSampleOn != 0)
	{
		bSample = pAudioChannel->bVolume;
	}
	else
	{
		bSample = 0;
	}

	return bSample;
}
