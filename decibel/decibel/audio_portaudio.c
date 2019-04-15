#include "stdafx.h"
#include "audio_portaudio.h"
#include "portaudio.h"
#ifdef _DEBUG
#pragma comment(lib,"portaudio_static_x86_d.lib")
#else
#pragma comment(lib,"portaudio_static_x86.lib")
#endif // DEBUG

typedef struct {
	PaUtilRingBuffer ptr;
	VOID* mem;
	INT len;
	SRWLOCK lock;
	CONDITION_VARIABLE cond[2];
	HANDLE task;
} rt_audio_buff;


static BOOL bStart = FALSE;
static BOOL bInit = FALSE;
static BOOL bOpen = FALSE;
static rt_audio_buff rt_ing = { 0 };
static PaDeviceIndex iDeviceIndex = 0;
static PaStream* pInputStream = NULL;
static PaStreamParameters inputParameters = { 0 };

BOOL portaudio_init() {
	if (!bInit) {
		if (paNoError == Pa_Initialize()) {
			iDeviceIndex = Pa_GetDefaultInputDevice();
			InterlockedExchange(&bInit, TRUE);
		}
		else {
			InterlockedExchange(&bInit, FALSE);
		}
	}
	return bInit;
}

int InputPaStreamCallback(
	const void* input,
	void* output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData
) {
	if (input) {
		if (TryAcquireSRWLockExclusive(&rt_ing.lock)) {
			__try {
				//达到临界值就唤醒（一半）
				if (PaUtil_GetRingBufferWriteAvailable(&rt_ing.ptr) <= 8) {
					WakeConditionVariable(&rt_ing.cond[0]);
					//20ms
					if (SleepConditionVariableSRW(&rt_ing.cond[1], &rt_ing.lock, 20, 0)) {
						PaUtil_WriteRingBuffer(&rt_ing.ptr, input, (frameCount << 2) / FRAGMENT_SIZE);
					}
				}
				else {
					PaUtil_WriteRingBuffer(&rt_ing.ptr, input, (frameCount << 2) / FRAGMENT_SIZE);
				}
			}
			__finally {
				ReleaseSRWLockExclusive(&rt_ing.lock);
			}
		}
	}
	return 0;
}

BOOL	Audio_PortAudio_Name(CHAR * cName) {
	if (portaudio_init()) {
		if (cName) {
			const PaDeviceInfo* pDeviceInfo = Pa_GetDeviceInfo(iDeviceIndex);
			CopyMemory(cName, pDeviceInfo->name, strlen(pDeviceInfo->name));
			return TRUE;
		}
	}
	return FALSE;
}

DWORD WINAPI TaskProc(LPVOID lpParameter) {
	INT iSize = 0, iRead = 0;
	BYTE buff[FRAGMENT_SIZE] = { NULL };
	while (bStart) {
		if (TryAcquireSRWLockShared(&rt_ing.lock)) {
			if (SleepConditionVariableSRW(&rt_ing.cond[0], &rt_ing.lock, INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED)) {
				iSize = PaUtil_GetRingBufferReadAvailable(&rt_ing.ptr) / 1;
				if (iSize) {
					DecWaveInProc dwCallback = (DecWaveInProc)lpParameter;
					if (dwCallback) {
						while (iSize > 0) {
							if ((iRead = PaUtil_ReadRingBuffer(&rt_ing.ptr, buff, 1))) {
								WAVEHDR hdr = { 0 };
								hdr.dwBytesRecorded = iRead * FRAGMENT_SIZE;
								hdr.lpData = buff;
								dwCallback(NULL, WIM_DATA, NULL, &hdr, NULL);
							}
							WakeConditionVariable(&rt_ing.cond[1]);
							iSize--;
						}
					}
				}
				else {
					WakeConditionVariable(&rt_ing.cond[1]);
				}
			}
			ReleaseSRWLockShared(&rt_ing.lock);
		}
	}
	return TRUE;
}

BOOL Audio_PortAudio_StartDec(DWORD wFormatTag, DWORD nChannels, DWORD nSamplesPerSec, DWORD nBlockAlign, DWORD wBitsPerSample, BOOL bLoopback, DecWaveInProc dwCallback) {
	PaError iError = 0;
	INT iFlag = paClipOff;
	if (!bStart) {
		ZeroMemory(&rt_ing, sizeof(rt_audio_buff));
		if (portaudio_init()) {
			inputParameters.device = iDeviceIndex;
			inputParameters.channelCount = nChannels;
			inputParameters.sampleFormat = paInt16;
			inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
			inputParameters.hostApiSpecificStreamInfo = NULL;
			if (Pa_IsFormatSupported(&inputParameters, NULL, nSamplesPerSec) == paFormatIsSupported) {
				/*if (bLoopback) {
					PaHostApiIndex iHAC = Pa_GetHostApiCount();
					while (iHAC > 0) {
						iHAC -= 1;
						const PaHostApiInfo* pHAI = Pa_GetHostApiInfo(iHAC);
						if (pHAI&&pHAI->type == paWDMKS) {
							//获取设备信息loopback
							const PaDeviceInfo* pDI = Pa_GetDeviceInfo(pHAI->defaultInputDevice);
							if (pDI&&pDI->maxInputChannels > 0) {
								if (pHAI->defaultInputDevice != paNoDevice) {
									inputParameters.device = pHAI->defaultInputDevice;
									inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
									iError = Pa_IsFormatSupported(&inputParameters, NULL, nSamplesPerSec);
									if (iError == paFormatIsSupported) {
										//iFlag = (1 << 0);
										break;
									}
								}
							}
						}
					}
				}*/
				iError = Pa_OpenStream(&pInputStream, &inputParameters, NULL, nSamplesPerSec, FRAGMENT_SIZE, iFlag, InputPaStreamCallback, NULL);
				if (iError == paNoError) {
					InterlockedExchange(&bOpen, TRUE);
					rt_ing.len = FRAGMENT_SIZE << 4;
					//分配4 PAGE
					rt_ing.mem = VirtualAlloc(NULL, rt_ing.len, MEM_COMMIT, PAGE_READWRITE);
					ZeroMemory(rt_ing.mem, rt_ing.len);
					if (rt_ing.mem) {
						if (PaUtil_InitializeRingBuffer(&rt_ing.ptr, FRAGMENT_SIZE, 16, rt_ing.mem) != -1) {
							//C
							InitializeConditionVariable(&rt_ing.cond[0]);
							//P
							InitializeConditionVariable(&rt_ing.cond[1]);
							InitializeSRWLock(&rt_ing.lock);
							if (Pa_StartStream(pInputStream) == paNoError) {
								InterlockedExchange(&bStart, TRUE);
								rt_ing.task = _beginthreadex(NULL, 0, TaskProc, dwCallback, 0, NULL);
							}
							else {
								Audio_PortAudio_StopDec();
							}
						}
						else {
							//缓冲区初始化失败
							Audio_PortAudio_StopDec();
						}
					}
					else {
						Audio_PortAudio_StopDec();
					}
				}
				else {
					Audio_PortAudio_StopDec();
				}
			}
			else {
				Audio_PortAudio_StopDec();
			}

		}
	}
	return bStart;
}


BOOL  Audio_PortAudio_Reset() {
	if (rt_ing.mem) {
		ZeroMemory(rt_ing.mem, rt_ing.len);
		PaUtil_FlushRingBuffer(&rt_ing.ptr);
		return TRUE;
	}
	return FALSE;
}

BOOL Audio_PortAudio_StopDec() {
	if (bInit) {
		if (InterlockedExchange(&bOpen, FALSE) == TRUE) {
			if (InterlockedExchange(&bStart, FALSE) == TRUE) {
				WakeConditionVariable(&rt_ing.cond[0]);
				//Pa_StopStream(pInputStream);
				Pa_AbortStream(pInputStream);
			}
			Pa_CloseStream(pInputStream);
		}
		InterlockedExchange(&bInit, FALSE);
		if (rt_ing.mem != NULL) {
			VirtualFree(rt_ing.mem, rt_ing.len, MEM_DECOMMIT | MEM_RELEASE);
			rt_ing.mem = NULL;
		}
		Pa_Terminate();
	}
	return TRUE;
}

INT  Audio_PortAudio_Num() {
	if (portaudio_init()) {
		//获取默认输入设备
		if (iDeviceIndex == paNoDevice) {
			Pa_Terminate();
			if (Pa_Initialize() == paNoError) {
				iDeviceIndex = Pa_GetDefaultInputDevice();
				if (iDeviceIndex != paNoDevice) {
					return iDeviceIndex;
				}
				else {
					return 0;
				}
			}
		}
		else {
			return iDeviceIndex;
		}
	}
	return 0;
}