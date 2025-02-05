#include "..\NES.h"

DWORD ApuMemoryMappedRegisters_Write_DeltaModulation(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue)
{
	AudioChannelState_DeltaModulationStruct *pDeltaModulationState = NULL;
	BYTE bIrqEnabled = 0;
	BYTE bLoop = 0;
	BYTE bRateIndex = 0;
	BYTE bOutputLevel = 0;
	WORD wSampleAddress = 0;
	WORD wSampleLength = 0;

	// handle apu register memory write 
	pDeltaModulationState = &pAudioChannel->DeltaModulation;
	if(wApuRegisterAddress == APU_REGISTER_DELTA_MODULATION_CTRL1)
	{
		bIrqEnabled = (bValue >> 7) & 1;
		bLoop = (bValue >> 6) & 1;
		bRateIndex = bValue & 0xF;

		if(bIrqEnabled == 0)
		{
			// clear IRQ
			gSystem.Cpu.bIrqPending_ApuDmc = 0;
		}

		if(bRateIndex >= sizeof(gSystem.RegionSpecificSettings.ApuDeltaModulationOutputRateTable.bRateTable))
		{
			return 1;
		}

		pDeltaModulationState->bIrqEnabled = bIrqEnabled;
		pDeltaModulationState->bLoop = bLoop;
		pDeltaModulationState->bOrigCounter = gSystem.RegionSpecificSettings.ApuDeltaModulationOutputRateTable.bRateTable[bRateIndex];
		pDeltaModulationState->bCurrCounter = 0;
	}
	else if(wApuRegisterAddress == APU_REGISTER_DELTA_MODULATION_CTRL2)
	{
		bOutputLevel = bValue & 0x7F;
		pDeltaModulationState->bOutputLevel = bOutputLevel;
	}
	else if(wApuRegisterAddress == APU_REGISTER_DELTA_MODULATION_CTRL3)
	{
		wSampleAddress = 0xC000 + (bValue * 64);
		pDeltaModulationState->wSampleAddress = wSampleAddress;
	}
	else if(wApuRegisterAddress == APU_REGISTER_DELTA_MODULATION_CTRL4)
	{
		wSampleLength = 1 + (bValue * 16);
		pDeltaModulationState->wSampleLength = wSampleLength;
	}
	else
	{
		return 1;
	}

	return 0;
}

DWORD UpdateDeltaModulationOutput(AudioChannelState_DeltaModulationStruct *pDeltaModulation)
{
	BYTE bCurrBit = 0;

	if(pDeltaModulation->bCurrByte_BitsRemaining == 0)
	{
		// read next byte
		Memory_Read8(pDeltaModulation->wCurrAddress, &pDeltaModulation->bCurrByte);

		// update address
		if(pDeltaModulation->wCurrAddress == 0xFFFF)
		{
			// wrap around from 0xFFFF to 0x8000
			pDeltaModulation->wCurrAddress = 0x8000;
		}
		else
		{
			pDeltaModulation->wCurrAddress++;
		}

		// update counters
		pDeltaModulation->wBytesRemaining--;
		pDeltaModulation->bCurrByte_BitsRemaining = 8;
	}

	// get next bit
	bCurrBit = (pDeltaModulation->bCurrByte >> (8 - pDeltaModulation->bCurrByte_BitsRemaining)) & 1;
	if(bCurrBit == 0)
	{
		// decrease level
		if(pDeltaModulation->bOutputLevel <= DELTA_MODULATION_STEP_COUNT)
		{
			// min value
			pDeltaModulation->bOutputLevel = 0;
		}
		else
		{
			pDeltaModulation->bOutputLevel -= DELTA_MODULATION_STEP_COUNT;
		}
	}
	else
	{
		// increase level
		if(pDeltaModulation->bOutputLevel >= (DELTA_MODULATION_MAX_LEVEL - DELTA_MODULATION_STEP_COUNT))
		{
			// max value
			pDeltaModulation->bOutputLevel = DELTA_MODULATION_MAX_LEVEL;
		}
		else
		{
			pDeltaModulation->bOutputLevel += DELTA_MODULATION_STEP_COUNT;
		}
	}

	// decrease counter
	pDeltaModulation->bCurrByte_BitsRemaining--;

	// check if the entire audio sample has been played
	if(pDeltaModulation->bCurrByte_BitsRemaining == 0)
	{
		if(pDeltaModulation->wBytesRemaining == 0)
		{
			// sample finished
			if(pDeltaModulation->bLoop != 0)
			{
				// loop
				pDeltaModulation->wCurrAddress = pDeltaModulation->wSampleAddress;
				pDeltaModulation->wBytesRemaining = pDeltaModulation->wSampleLength;
			}
			else
			{
				// finished, set IRQ if enabled
				if(pDeltaModulation->bIrqEnabled != 0)
				{
					gSystem.Cpu.bIrqPending_ApuDmc = 1;
				}
			}
		}
	}

	return 0;
}

DWORD ApuCycle_DeltaModulation(AudioChannelStateStruct *pAudioChannel)
{
	if(pAudioChannel->bChannelEnabled != 0)
	{
		if(pAudioChannel->DeltaModulation.bCurrCounter != 0)
		{
			pAudioChannel->DeltaModulation.bCurrCounter--;
		}
		else
		{
			// check if there is any audio to play
			if(pAudioChannel->DeltaModulation.wBytesRemaining != 0 || pAudioChannel->DeltaModulation.bCurrByte_BitsRemaining != 0)
			{
				// update output value and reset counter
				UpdateDeltaModulationOutput(&pAudioChannel->DeltaModulation);
			}
			pAudioChannel->DeltaModulation.bCurrCounter = pAudioChannel->DeltaModulation.bOrigCounter;
		}
	}

	return 0;
}

BYTE GetNextSample_DeltaModulation(AudioChannelStateStruct *pAudioChannel)
{
	// return the current output level (0-127 for this channel)
	return pAudioChannel->DeltaModulation.bOutputLevel;
}
