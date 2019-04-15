#include "stdafx.h"
#include "audio.h" 
/************************************************************************/
/* ÒôÆµ²É¼¯PCM                                                          */
/************************************************************************/
static HWAVEIN hWaveIn;
static BOOL bStart = FALSE;
static CRITICAL_SECTION  cs;
static WAVEHDR aWavHdr[FRAGMENT_NUM];
static BYTE *pBuffer = NULL;
static void CALLBACK SelfWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

INT Audio_Num() {
	return waveInGetNumDevs();
}

BOOL  Audio_Reset() {
	return FALSE;
}
BOOL  Audio_Name(CHAR *cName) {
	return FALSE;
}

BOOL Audio_StartDec(DWORD wFormatTag, DWORD nChannels, DWORD nSamplesPerSec, DWORD nBlockAlign, DWORD wBitsPerSample, BOOL bLoopback, DecWaveInProc dwCallback) {
	if (Audio_Num()) {
		WAVEFORMATEX wavform;
		wavform.wFormatTag = (WORD)wFormatTag;
		wavform.nChannels = (WORD)nChannels;
		wavform.nSamplesPerSec = nSamplesPerSec;
		wavform.nAvgBytesPerSec = nSamplesPerSec * (wBitsPerSample >> 3) * nChannels;
		wavform.nBlockAlign = (WORD)nBlockAlign;
		wavform.wBitsPerSample = (WORD)wBitsPerSample;
		wavform.cbSize = 0;
		if (MMSYSERR_NOERROR == waveInOpen(&hWaveIn, WAVE_MAPPER, &wavform, (DWORD_PTR)SelfWaveInProc, 0, CALLBACK_FUNCTION)) {
			pBuffer = VirtualAlloc(NULL, FRAGMENT_NUM*FRAGMENT_NUM, MEM_COMMIT, PAGE_READWRITE);
			ZeroMemory(pBuffer, FRAGMENT_SIZE*FRAGMENT_NUM);
			for (int i = 0; i < FRAGMENT_NUM; i++) {
				aWavHdr[i].lpData = pBuffer + (i*FRAGMENT_SIZE);
				aWavHdr[i].dwBufferLength = FRAGMENT_SIZE;
				aWavHdr[i].dwBytesRecorded = 0;
				aWavHdr[i].dwUser = (DWORD_PTR)dwCallback;
				aWavHdr[i].dwFlags = 0;
				aWavHdr[i].dwLoops = 1;
				aWavHdr[i].lpNext = NULL;
				aWavHdr[i].reserved = 0;
				waveInPrepareHeader(hWaveIn, &aWavHdr[i], sizeof(WAVEHDR));
				waveInAddBuffer(hWaveIn, &aWavHdr[i], sizeof(WAVEHDR));
			}
			if (MMSYSERR_NOERROR == waveInStart(hWaveIn)) {
				bStart = TRUE;
				InitializeCriticalSection(&cs);
			}
		}
	}
	return bStart;
}

static void CALLBACK SelfWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	if (WIM_DATA == uMsg) {
		LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;
		if (pwh->dwUser) {
			DecWaveInProc pDwip = (DecWaveInProc)pwh->dwUser;
			if (pDwip) {
				pDwip(hwi, uMsg, dwInstance, dwParam1, dwParam2);
			}
		}
		EnterCriticalSection(&cs);
		if (bStart == TRUE) {
			waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
		}
		LeaveCriticalSection(&cs);
	}
}

BOOL Audio_StopDec() {
	BOOL oldStart = FALSE;
	EnterCriticalSection(&cs);
	oldStart = bStart;
	bStart = FALSE;
	LeaveCriticalSection(&cs);
	if (oldStart) {
		waveInStop(hWaveIn);
		waveInReset(hWaveIn);
		for (int i = 0; i < FRAGMENT_NUM; i++) {
			waveInUnprepareHeader(hWaveIn, &aWavHdr[i], sizeof(WAVEHDR));
		}
		waveInClose(hWaveIn);
		if (pBuffer != NULL) {
			VirtualFree(pBuffer, FRAGMENT_SIZE*FRAGMENT_NUM, MEM_DECOMMIT);
		}
		DeleteCriticalSection(&cs);
	}
	return (bStart == TRUE) ? FALSE : TRUE;
}