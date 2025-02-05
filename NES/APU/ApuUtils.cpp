#include "..\NES.h"

DWORD ApuLengthCounter_Set(AudioChannelLengthCounterStruct *pLengthCounter, BYTE bLengthTableIndex)
{
	BYTE bLengthLookupTable[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

	if(bLengthTableIndex >= sizeof(bLengthLookupTable))
	{
		return 1;
	}

	// set counter value from lookup table
	pLengthCounter->bCurrCounter = bLengthLookupTable[bLengthTableIndex];

	return 0;
}

DWORD ApuLengthCounter_Process(AudioChannelLengthCounterStruct *pLengthCounter)
{
	// check if length counter is halted
	if(pLengthCounter->bHalt == 0)
	{
		// decrease length counter
		if(pLengthCounter->bCurrCounter != 0)
		{
			pLengthCounter->bCurrCounter--;
		}
	}

	return 0;
}

DWORD ApuVolumeDecay_Set(AudioChannelVolumeDecayStruct *pVolumeDecay, BYTE bOrigCounter, BYTE bLoop)
{
	// enable volume decay
	pVolumeDecay->bEnabled = 1;
	pVolumeDecay->bOrigCounter = bOrigCounter;
	pVolumeDecay->bResetFlag = 1;
	pVolumeDecay->bLoop = bLoop;

	return 0;
}

DWORD ApuVolumeDecay_Process(AudioChannelVolumeDecayStruct *pVolumeDecay)
{
	BYTE bVolume = 0;
	BYTE bReset = 0;

	// get the current volume level for this channel
	bVolume = *pVolumeDecay->pbVolume;

	if(pVolumeDecay->bEnabled != 0)
	{
		if(pVolumeDecay->bResetFlag != 0)
		{
			// reset
			bReset = 1;
			pVolumeDecay->bResetFlag = 0;
		}
		else
		{
			// decrease counter
			if(pVolumeDecay->bCurrCounter != 0)
			{
				pVolumeDecay->bCurrCounter--;
			}
			else
			{
				// decrease volume
				if(bVolume != 0)
				{
					// decrease
					bVolume--;

					// reset counter
					pVolumeDecay->bCurrCounter = pVolumeDecay->bOrigCounter;
				}
				else
				{
					if(pVolumeDecay->bLoop != 0)
					{
						// loop
						bReset = 1;
					}
				}
			}
		}

		if(bReset != 0)
		{
			// reset
			bVolume = MAX_VOLUME;
			pVolumeDecay->bCurrCounter = pVolumeDecay->bOrigCounter;
		}
	}

	// store updated volume value
	*pVolumeDecay->pbVolume = bVolume;

	return 0;
}

DWORD ApuSweep_Set(AudioChannelSweepStruct *pSweep, BYTE bOrigCounter, BYTE bNegate, BYTE bNegateAdditional, BYTE bShiftCount)
{
	// enable sweep
	pSweep->bEnabled = 1;
	pSweep->bOrigCounter = bOrigCounter;
	pSweep->bResetFlag = 1;
	pSweep->bNegate = bNegate;
	pSweep->bNegateAdditional = bNegateAdditional;
	pSweep->bShiftCount = bShiftCount;

	return 0;
}

DWORD ApuSweep_Process(AudioChannelSweepStruct *pSweep)
{
	WORD wFrequencyTimer = 0;
	BYTE bMuteChannel = 0;
	WORD wDifference = 0;
	WORD wNewFrequencyTimer = 0;

	// get the current sweep frequency timer for this channel
	wFrequencyTimer = *pSweep->pwFrequencyTimer;

	if(pSweep->bEnabled != 0)
	{
		// the reset flag should be ignored if the counter is already 0
		if(pSweep->bCurrCounter != 0)
		{
			if(pSweep->bResetFlag != 0)
			{
				// reset counter
				pSweep->bCurrCounter = pSweep->bOrigCounter;
			}
			else
			{
				// decrease counter
				pSweep->bCurrCounter--;
			}
		}
		else
		{
			// reset counter
			pSweep->bCurrCounter = pSweep->bOrigCounter;

			// mute channel if frequency timer drops below 8
			if(wFrequencyTimer < 8)
			{
				bMuteChannel = 1;
			}

			// mute channel if shift count was set to 0
			if(pSweep->bShiftCount == 0)
			{
				bMuteChannel = 1;
			}

			// calculate difference
			wDifference = wFrequencyTimer >> pSweep->bShiftCount;

			// calculate new frequency timer value
			wNewFrequencyTimer = wFrequencyTimer;
			if(pSweep->bNegate != 0)
			{
				if(pSweep->bNegateAdditional != 0)
				{
					// square wave channel 2 behaves differently in negate mode
					wDifference++;
				}

				wNewFrequencyTimer -= wDifference;
			}
			else
			{
				wNewFrequencyTimer += wDifference;
			}

			// mute channel if the new frequency timer rises above 0x7FF
			if(wNewFrequencyTimer > 0x7FF)
			{
				bMuteChannel = 1;
			}

			if(bMuteChannel == 0)
			{
				// update frequency timer
				wFrequencyTimer = wNewFrequencyTimer;
			}
			else
			{
				// mute channel
				pSweep->bEnabled = 0;
				wFrequencyTimer = 0;
			}
		}

		// clear reset flag
		pSweep->bResetFlag = 0;
	}

	// store updated frequency timer value
	*pSweep->pwFrequencyTimer = wFrequencyTimer;

	return 0;
}

DWORD ApuLinearCounter_Set(AudioChannelLinearCounterStruct *pLinearCounter, BYTE bControlFlag, BYTE bLinearCounter)
{
	// set linear counter
	pLinearCounter->bControlFlag = bControlFlag;
	pLinearCounter->bOrigCounter = bLinearCounter;

	return 0;
}

DWORD ApuLinearCounter_Process(AudioChannelLinearCounterStruct *pLinearCounter)
{
	if(pLinearCounter->bResetFlag != 0)
	{
		// reset counter
		pLinearCounter->bCurrCounter = pLinearCounter->bOrigCounter;
	}
	else
	{
		if(pLinearCounter->bCurrCounter != 0)
		{
			// decrease counter
			pLinearCounter->bCurrCounter--;
		}
	}

	if(pLinearCounter->bControlFlag == 0)
	{
		// only clear the reset flag if the control flag is not set
		pLinearCounter->bResetFlag = 0;
	}

	return 0;
}
