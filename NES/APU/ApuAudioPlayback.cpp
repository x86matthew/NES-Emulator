#include "..\NES.h"

AudioChannelStateStruct *GetAudioChannel(BYTE bAudioChannelIndex)
{
	// get channel object (1 - 5)
	if(bAudioChannelIndex >= 1 && bAudioChannelIndex <= AUDIO_CHANNEL_COUNT)
	{
		return &gSystem.Apu.Channels[bAudioChannelIndex - 1];
	}

	return NULL;
}

VOID CALLBACK WaveOutCallback(HWAVEOUT hWave, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if(uMsg == WOM_DONE)
	{
		// buffer playback complete - set event
		SetEvent((HANDLE)dwInstance);
	}
}

DWORD PopulateAudioPlaybackBuffer(WavePlaybackBufferStruct *pWavePlaybackBuffer)
{
	BYTE bError = 0;

	EnterCriticalSection(&gSystem.Apu.OutputBufferCriticalSection);

	// ensure there are enough queued input samples to fill an entire playback buffer
	if(gSystem.Apu.dwOutputBufferSampleCount < PLAYBACK_BUFFER_SAMPLE_COUNT)
	{
		ERROR_CLEAN_UP(1);
	}

	// copy samples into playback buffer
	for(DWORD i = 0 ; i < PLAYBACK_BUFFER_SAMPLE_COUNT; i++)
	{
		pWavePlaybackBuffer->bBuffer[i] = gSystem.Apu.bOutputBuffer[i];
	}

	// remove copied samples from input queue
	memmove(&gSystem.Apu.bOutputBuffer[0], &gSystem.Apu.bOutputBuffer[PLAYBACK_BUFFER_SAMPLE_COUNT], gSystem.Apu.dwOutputBufferSampleCount - PLAYBACK_BUFFER_SAMPLE_COUNT);
	gSystem.Apu.dwOutputBufferSampleCount -= PLAYBACK_BUFFER_SAMPLE_COUNT;

CLEAN_UP:
	LeaveCriticalSection(&gSystem.Apu.OutputBufferCriticalSection);
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD InitialisePlaybackBuffers(HWAVEOUT hWaveOut, WavePlaybackBufferStruct *pWavePlaybackBufferList)
{
	WavePlaybackBufferStruct *pCurrWavePlaybackBuffer = NULL;

	for(DWORD i = 0; i < PLAYBACK_LOOP_BUFFER_COUNT; i++)
	{
		// get current buffer
		pCurrWavePlaybackBuffer = &pWavePlaybackBufferList[i];

		// prepare current buffer
		memset(pCurrWavePlaybackBuffer, 0, sizeof(WavePlaybackBufferStruct));
		pCurrWavePlaybackBuffer->WaveHeader.lpData = (LPSTR)pWavePlaybackBufferList[i].bBuffer;
		pCurrWavePlaybackBuffer->WaveHeader.dwBufferLength = sizeof(pWavePlaybackBufferList[i].bBuffer);
		pCurrWavePlaybackBuffer->WaveHeader.dwFlags = 0;
		if(waveOutPrepareHeader(hWaveOut, &pCurrWavePlaybackBuffer->WaveHeader, sizeof(pCurrWavePlaybackBuffer->WaveHeader)) != MMSYSERR_NOERROR)
		{
			return 1;
		}

		// mark buffer as "done", ready to re-populate
		pCurrWavePlaybackBuffer->WaveHeader.dwFlags |= WHDR_DONE;
	}

	return 0;
}

DWORD AudioPlaybackLoop(HWAVEOUT hWaveOut, WavePlaybackBufferStruct *pWavePlaybackBufferList, HANDLE hBufferCompleteEvent)
{
	WavePlaybackBufferStruct *pCurrWavePlaybackBuffer = NULL;
	DWORD dwCurrBufferIndex = 0;
	HANDLE hWaitHandleList[2];
	BYTE bShutDown = 0;

	// set initial event to begin playback
	SetEvent(hBufferCompleteEvent);

	for(;;)
	{
		// wait for event from wave output callback
		if(WaitForSingleObject(hBufferCompleteEvent, INFINITE) != WAIT_OBJECT_0)
		{
			return 1;
		}

		for(;;)
		{
			// get current buffer
			pCurrWavePlaybackBuffer = &pWavePlaybackBufferList[dwCurrBufferIndex];

			// check if the current buffer is ready to re-populate
			if((pCurrWavePlaybackBuffer->WaveHeader.dwFlags & WHDR_DONE) == 0)
			{
				// current buffer is still playing, stop
				break;
			}

			// wait for the APU to provide another a block of samples
			hWaitHandleList[0] = gSystem.Apu.hOutputBufferEvent;
			hWaitHandleList[1] = gSystem.hShutDownEvent;
			if(WaitForMultipleObjects(2, hWaitHandleList, 0, INFINITE) != WAIT_OBJECT_0)
			{
				// shutdown event caught
				bShutDown = 1;
				break;
			}

			// copy samples from the APU buffer to the playback queue
			if(PopulateAudioPlaybackBuffer(pCurrWavePlaybackBuffer) == 0)
			{
				// success - queue next buffer
				if(waveOutWrite(hWaveOut, &pCurrWavePlaybackBuffer->WaveHeader, sizeof(pCurrWavePlaybackBuffer->WaveHeader)) != MMSYSERR_NOERROR)
				{
					return 1;
				}

				// increase buffer index
				dwCurrBufferIndex++;
				if(dwCurrBufferIndex >= PLAYBACK_LOOP_BUFFER_COUNT)
				{
					// reset index
					dwCurrBufferIndex = 0;
				}
			}
		}

		if(bShutDown != 0)
		{
			// shutdown flag set
			break;
		}
	}

	return 0;
}

DWORD WINAPI ApuAudioPlaybackThread(LPVOID lpArg)
{
	BYTE bError = 0;
	WAVEFORMATEX WaveFormat;
	HANDLE hBufferCompleteEvent = NULL;
	HWAVEOUT hWaveOut;
	WavePlaybackBufferStruct *pWavePlaybackBufferList = NULL;

	// initialise wave format data
	memset(&WaveFormat, 0, sizeof(WaveFormat));
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.wBitsPerSample = AUDIO_BITS_PER_SAMPLE;
	WaveFormat.nChannels = 1;
	WaveFormat.nSamplesPerSec = AUDIO_SAMPLE_RATE;
	WaveFormat.nAvgBytesPerSec = AUDIO_BITS_PER_SAMPLE * (AUDIO_BITS_PER_SAMPLE / 8);
	WaveFormat.nBlockAlign = AUDIO_BITS_PER_SAMPLE / 8;
	WaveFormat.cbSize = 0;

	// create event
	hBufferCompleteEvent = CreateEvent(NULL, 0, 0, NULL);
	if(hBufferCompleteEvent == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// allocate playback buffers
	pWavePlaybackBufferList = (WavePlaybackBufferStruct*)malloc(PLAYBACK_LOOP_BUFFER_COUNT * sizeof(WavePlaybackBufferStruct));
	if(pWavePlaybackBufferList == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// open output device
	if(waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD_PTR)WaveOutCallback, (DWORD_PTR)hBufferCompleteEvent, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		ERROR_CLEAN_UP(1);
	}

	// initialise buffers
	if(InitialisePlaybackBuffers(hWaveOut, pWavePlaybackBufferList) != 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// audio playback ready
	SetEvent((HANDLE)lpArg);

	// begin playback loop
	if(AudioPlaybackLoop(hWaveOut, pWavePlaybackBufferList, hBufferCompleteEvent) != 0)
	{
		ERROR_CLEAN_UP(1);
	}

	// clean up
CLEAN_UP:
	if(hWaveOut != NULL)
	{
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
	}
	if(pWavePlaybackBufferList != NULL)
	{
		free(pWavePlaybackBufferList);
	}
	if(hBufferCompleteEvent != NULL)
	{
		CloseHandle(hBufferCompleteEvent);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD ApuCreateThread()
{
	BYTE bError = 0;
	HANDLE hReadyEvent = NULL;
	HANDLE hWaitHandleList[2];

	// create ready event
	hReadyEvent = CreateEvent(NULL, 0, 0, NULL);
	if(hReadyEvent == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// create thread
	gSystem.Apu.hThread = CreateThread(NULL, 0, ApuAudioPlaybackThread, hReadyEvent, 0, NULL);
	if(gSystem.Apu.hThread == NULL)
	{
		ERROR_CLEAN_UP(1);
	}

	// wait for thread ready event
	hWaitHandleList[0] = hReadyEvent;
	hWaitHandleList[1] = gSystem.Apu.hThread;
	if(WaitForMultipleObjects(2, hWaitHandleList, 0, INFINITE) != WAIT_OBJECT_0)
	{
		ERROR_CLEAN_UP(1);
	}

CLEAN_UP:
	if(hReadyEvent != NULL)
	{
		CloseHandle(hReadyEvent);
	}
	if(bError != 0)
	{
		return 1;
	}

	return 0;
}

DWORD InitialiseAudioChannels()
{
	AudioChannelStateStruct *pAudioChannel = NULL;
	BYTE bAudioChannelIndex = 0;

	// initialise audio channels
	for(DWORD i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		// get current channel object
		bAudioChannelIndex = (BYTE)i + 1;
		pAudioChannel = GetAudioChannel(bAudioChannelIndex);
		if(pAudioChannel == NULL)
		{
			return 1;
		}

		// set index
		pAudioChannel->bAudioChannelIndex = bAudioChannelIndex;

		// initialise channel-specific data
		if(bAudioChannelIndex == 1 || bAudioChannelIndex == 2)
		{
			// square wave channel
			pAudioChannel->SquareWave.Sweep.pwFrequencyTimer = &pAudioChannel->SquareWave.wFrequencyTimer;
			pAudioChannel->SquareWave.VolumeDecay.pbVolume = &pAudioChannel->bVolume;

			pAudioChannel->pCycleHandler = NULL;
			pAudioChannel->pQuarterFrameHandler = ApuQuarterFrame_SquareWave;
			pAudioChannel->pHalfFrameHandler = ApuHalfFrame_SquareWave;
			pAudioChannel->pWriteRegister = ApuMemoryMappedRegisters_Write_SquareWave;
			pAudioChannel->pGetNextSample = GetNextSample_SquareWave;
		}
		else if(bAudioChannelIndex == 3)
		{
			// triangle wave channel
			pAudioChannel->pCycleHandler = NULL;
			pAudioChannel->pQuarterFrameHandler = ApuQuarterFrame_TriangleWave;
			pAudioChannel->pHalfFrameHandler = ApuHalfFrame_TriangleWave;
			pAudioChannel->pWriteRegister = ApuMemoryMappedRegisters_Write_TriangleWave;
			pAudioChannel->pGetNextSample = GetNextSample_TriangleWave;
		}
		else if(bAudioChannelIndex == 4)
		{
			// noise channel
			pAudioChannel->Noise.VolumeDecay.pbVolume = &pAudioChannel->bVolume;
			pAudioChannel->pCycleHandler = NULL;
			pAudioChannel->pQuarterFrameHandler = ApuQuarterFrame_Noise;
			pAudioChannel->pHalfFrameHandler = ApuHalfFrame_Noise;
			pAudioChannel->pWriteRegister = ApuMemoryMappedRegisters_Write_Noise;
			pAudioChannel->pGetNextSample = GetNextSample_Noise;
		}
		else if(bAudioChannelIndex == 5)
		{
			// DMC
			pAudioChannel->pCycleHandler = ApuCycle_DeltaModulation;
			pAudioChannel->pQuarterFrameHandler = NULL;
			pAudioChannel->pHalfFrameHandler = NULL;
			pAudioChannel->pWriteRegister = ApuMemoryMappedRegisters_Write_DeltaModulation;
			pAudioChannel->pGetNextSample = GetNextSample_DeltaModulation;
		}
		else
		{
			return 1;
		}
	}

	// initialise random seed
	srand(GetTickCount());

	// generate long random noise sequence
	for(DWORD i = 0; i < sizeof(gSystem.Apu.bNoiseRandomSequence_Long); i++)
	{
		gSystem.Apu.bNoiseRandomSequence_Long[i] = (BYTE)(rand() % 2);
	}

	// generate short random noise sequence
	for(DWORD i = 0; i < sizeof(gSystem.Apu.bNoiseRandomSequence_Short); i++)
	{
		gSystem.Apu.bNoiseRandomSequence_Short[i] = (BYTE)(rand() % 2);
	}

	// create output buffer critical section
	InitializeCriticalSection(&gSystem.Apu.OutputBufferCriticalSection);
	gSystem.Apu.bOutputBufferCriticalSectionReady = 1;

	// create output buffer event
	gSystem.Apu.hOutputBufferEvent = CreateEventA(NULL, 0, 0, NULL);
	if(gSystem.Apu.hOutputBufferEvent == NULL)
	{
		return 1;
	}

	// create APU thread
	if(ApuCreateThread() != 0)
	{
		return 1;
	}

	return 0;
}
