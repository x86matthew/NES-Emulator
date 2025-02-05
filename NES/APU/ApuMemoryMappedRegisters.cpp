#include "..\NES.h"

DWORD ApuMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue)
{
	BYTE bAudioChannelIndex = 0;
	AudioChannelStateStruct *pAudioChannel = NULL;
	AudioChannelLengthCounterStruct *pLengthCounter = NULL;
	BYTE bValue = 0;

	if(wAddress == APU_REGISTER_STATUS)
	{
		// get APU status - check all 5 audio channels
		for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
		{
			// get channel object
			bAudioChannelIndex = (BYTE)(i + 1);
			pAudioChannel = GetAudioChannel(bAudioChannelIndex);
			if(pAudioChannel == NULL)
			{
				return 1;
			}

			// check channel type
			if(bAudioChannelIndex == 5)
			{
				// delta modulation
				if(pAudioChannel->DeltaModulation.wBytesRemaining != 0)
				{
					// enabled
					bValue |= 0x10;
				}
			}
			else
			{
				// get length counter
				if(bAudioChannelIndex == 1 || bAudioChannelIndex == 2)
				{
					// square wave
					pLengthCounter = &pAudioChannel->SquareWave.LengthCounter;
				}
				else if(bAudioChannelIndex == 3)
				{
					// triangle wave
					pLengthCounter = &pAudioChannel->TriangleWave.LengthCounter;
				}
				else if(bAudioChannelIndex == 4)
				{
					// noise
					pLengthCounter = &pAudioChannel->Noise.LengthCounter;
				}
				else
				{
					return 1;
				}

				// check if the current channel is enabled
				if(pAudioChannel->bChannelEnabled != 0)
				{
					if(pLengthCounter->bHalt == 0)
					{
						// enabled
						bValue |= (1 << i);
					}
				}
			}
		}

		if(gSystem.Cpu.bIrqPending_ApuFrame != 0)
		{
			// frame IRQ pending
			bValue |= 0x40;

			// clear frame irq flag
			gSystem.Cpu.bIrqPending_ApuFrame = 0;
		}

		if(gSystem.Cpu.bIrqPending_ApuDmc != 0)
		{
			// delta modulation channel IRQ pending
			bValue |= 0x80;
		}
	}
	else
	{
		return 1;
	}

	*pbValue = bValue;

	return 0;
}

DWORD ApuMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue)
{
	BYTE bAudioChannelIndex = 0;
	AudioChannelStateStruct *pAudioChannel = NULL;
	BYTE bCurrChannelEnabled = 0;
	BYTE b5StepMode = 0;

	if(wAddress == APU_REGISTER_STATUS)
	{
		// set APU status
		for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
		{
			bAudioChannelIndex = (BYTE)(i + 1);
			bCurrChannelEnabled = (bValue >> i) & 1;

			pAudioChannel = GetAudioChannel(bAudioChannelIndex);
			if(pAudioChannel == NULL)
			{
				return 1;
			}
			pAudioChannel->bChannelEnabled = bCurrChannelEnabled;
			if(bAudioChannelIndex == 1 || bAudioChannelIndex == 2)
			{
				// square wave channel
				if(bCurrChannelEnabled == 0)
				{
					pAudioChannel->SquareWave.LengthCounter.bCurrCounter = 0;
				}
			}
			else if(bAudioChannelIndex == 3)
			{
				// triangle wave channel
				if(bCurrChannelEnabled == 0)
				{
					pAudioChannel->TriangleWave.LengthCounter.bCurrCounter = 0;
				}
			}
			else if(bAudioChannelIndex == 4)
			{
				// noise channel
				if(bCurrChannelEnabled == 0)
				{
					pAudioChannel->Noise.LengthCounter.bCurrCounter = 0;
				}
			}
			else if(bAudioChannelIndex == 5)
			{
				// delta modulation channel
				if(bCurrChannelEnabled == 0)
				{
					pAudioChannel->DeltaModulation.wBytesRemaining = 0;
				}
				else
				{
					// only reset if the previous sample has finished playing
					if(pAudioChannel->DeltaModulation.wBytesRemaining == 0 && pAudioChannel->DeltaModulation.bCurrByte_BitsRemaining == 0)
					{
						// reset
						pAudioChannel->DeltaModulation.wBytesRemaining = pAudioChannel->DeltaModulation.wSampleLength;
						pAudioChannel->DeltaModulation.wCurrAddress = pAudioChannel->DeltaModulation.wSampleAddress;
					}
				}

				// clear DMC interrupt
				gSystem.Cpu.bIrqPending_ApuDmc = 0;
			}
		}
	}
	else if(wAddress == APU_REGISTER_FRAME_COUNTER)
	{
		b5StepMode = (bValue >> 7) & 1;

		// reset frame counter
		gSystem.Apu.dwFrameCycleCounter = 0;
		gSystem.Apu.bQuarterFrameIndex = 0;

		if(b5StepMode != 0)
		{
			// 5-step mode
			gSystem.Apu.b5StepFrameMode = 1;
			CycleApu_QuarterFrame();
			CycleApu_HalfFrame();
		}
		else
		{
			// 4-step mode
			gSystem.Apu.b5StepFrameMode = 0;
		}
	}
	else if(wAddress >= 0x4000 && wAddress <= 0x4013)
	{
		// get target audio channel
		bAudioChannelIndex = ((wAddress >> 2) & 7) + 1;
		pAudioChannel = GetAudioChannel(bAudioChannelIndex);
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		// call channel-specific write function
		if(pAudioChannel->pWriteRegister(pAudioChannel, wAddress, bValue) != 0)
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}
