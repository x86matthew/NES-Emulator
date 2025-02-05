#include "..\NES.h"

DWORD ApuOutputSample(BYTE bSample)
{
	// lock buffer
	EnterCriticalSection(&gSystem.Apu.OutputBufferCriticalSection);

	if(gSystem.Apu.dwOutputBufferSampleCount >= sizeof(gSystem.Apu.bOutputBuffer))
	{
		// buffer full - discard data and reset
		gSystem.Apu.dwOutputBufferSampleCount = 0;
	}

	// add sample to end of buffer
	gSystem.Apu.bOutputBuffer[gSystem.Apu.dwOutputBufferSampleCount] = bSample;
	gSystem.Apu.dwOutputBufferSampleCount++;

	// check if enough samples have been buffered to re-populate a playback buffer
	if(gSystem.Apu.dwOutputBufferSampleCount >= PLAYBACK_BUFFER_SAMPLE_COUNT)
	{
		// buffer ready to be added to the playback queue
		SetEvent(gSystem.Apu.hOutputBufferEvent);
	}

	// unlock buffer
	LeaveCriticalSection(&gSystem.Apu.OutputBufferCriticalSection);

	return 0;
}

DWORD ApuGetNextSample(BYTE *pbSample)
{
	AudioChannelStateStruct *pAudioChannel = NULL;
	BYTE wMixedSample = 0;

	// set initial value to midpoint of wave - output samples are only positive (128 - 255)
	wMixedSample = 128;

	// get sample from each channel and mix
	for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		// get channel object
		pAudioChannel = GetAudioChannel((BYTE)(i + 1));
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		if(pAudioChannel->bChannelEnabled != 0)
		{
			// add sample from current channel to mixer
			wMixedSample += pAudioChannel->pGetNextSample(pAudioChannel);
		}
	}

	// prevent 8-bit overflow
	if(wMixedSample > 255)
	{
		wMixedSample = 255;
	}

	// store 8-bit sample
	*pbSample = (BYTE)wMixedSample;

	return 0;
}

DWORD ApuGenerateOutputSample()
{
	BYTE bCurrSample = 0;

	// get next sample
	if(ApuGetNextSample(&bCurrSample) != 0)
	{
		return 1;
	}

	// output sample
	if(ApuOutputSample(bCurrSample) != 0)
	{
		return 1;
	}

	return 0;
}
