#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include "shine_mp3.h"
#include "ring_buf.h"
#pragma comment(lib,"winmm.lib")   
#pragma comment(lib,"user32.lib")
#define FRAGMENT_NUM 8
#define FRAGMENT_SIZE 1024

typedef unsigned char BYTE;
void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void writeMp3(HANDLE pSource);

BOOL stopWrite = FALSE;
CRITICAL_SECTION  cs;
WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	if (waveInGetNumDevs()) {
		HWAVEIN hWaveIn;
		WAVEFORMATEX wavform;
		wavform.wFormatTag = WAVE_FORMAT_PCM;
		wavform.nChannels = 2;
		wavform.nSamplesPerSec = 44100;
		wavform.nAvgBytesPerSec = 44100 * 16 * 2 / 8;
		wavform.nBlockAlign = 4;
		wavform.wBitsPerSample = 16;
		wavform.cbSize = 0;
		LP_RING_BUF lrb = ring_buf_init(1024, 512);
		InitializeCriticalSection(&cs);
		waveInOpen(&hWaveIn, WAVE_MAPPER, &wavform, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
		WAVEHDR wh[FRAGMENT_NUM];
		BYTE *pBuffer = (BYTE*)GlobalAlloc(GMEM_ZEROINIT, FRAGMENT_SIZE*FRAGMENT_NUM);
		for (int i = 0; i < FRAGMENT_NUM; i++) {
			wh[i].lpData = pBuffer + (i*FRAGMENT_SIZE);
			wh[i].dwBufferLength = FRAGMENT_SIZE;
			wh[i].dwBytesRecorded = 0;
			wh[i].dwUser = (DWORD_PTR)lrb;
			wh[i].dwFlags = 0;
			wh[i].dwLoops = 1;
			wh[i].lpNext = NULL;
			wh[i].reserved = 0;
			waveInPrepareHeader(hWaveIn, &wh[i], sizeof(WAVEHDR));
			waveInAddBuffer(hWaveIn, &wh[i], sizeof(WAVEHDR));
		}
		waveInStart(hWaveIn);
		Sleep(9000);
		EnterCriticalSection(&cs);
		stopWrite = TRUE;
		LeaveCriticalSection(&cs);
		waveInStop(hWaveIn);
		waveInReset(hWaveIn);
		for (int i = 0; i < FRAGMENT_NUM; i++) {
			waveInUnprepareHeader(hWaveIn, &wh[i], sizeof(WAVEHDR));
		}
		waveInClose(hWaveIn);
		GlobalFree(pBuffer);
		DeleteCriticalSection(&cs);
		ring_buf_destory(lrb);
	}
	else {
		MessageBox(NULL, "找不到录音设备(0)", "错误", MB_OK | MB_ICONWARNING);
	}
	return S_OK;
}

/************************************************************************/
/* 回调函数                                                             */
/************************************************************************/
void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	if (WIM_DATA == uMsg) {
		LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;
		LP_RING_BUF lrb = (LP_RING_BUF)pwh->dwUser;
		if (lrb != NULL) {
			ring_buf_write(lrb, pwh->lpData, pwh->dwBytesRecorded);
		}
		EnterCriticalSection(&cs);
		if (stopWrite == FALSE) {
			waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
		}
		LeaveCriticalSection(&cs);
	}
}

/************************************************************************/
/* 写入mp3                                                              */
/************************************************************************/
void writeMp3(HANDLE pSource) {
	if (pSource != NULL) {
		shine_config_t config;
		int16_t *pBuffer;
		unsigned char * pData = NULL;
		int writeBytes = 0, readBytes = 0, samples_per_pass = 0;
		DWORD dwreturnsize;
		shine_set_config_mpeg_defaults(&config.mpeg);
		config.wave.samplerate = 44100;
		config.wave.channels = 2;
		config.mpeg.mode = DUAL_CHANNEL;
		config.mpeg.bitr = 128;
		config.mpeg.original = 1;
		shine_t st = shine_initialise(&config);
		if (st != NULL) {
			samples_per_pass = shine_samples_per_pass(st) << config.wave.channels;
			pBuffer = (int16_t*)GlobalAlloc(GMEM_ZEROINIT, samples_per_pass);
			char destNames[128];
			//格式化文件名
			SYSTEMTIME systime;
			GetLocalTime(&systime);
			sprintf_s(destNames, 128, "%d-%02d-%02d-%02d-%02d-%02d.mp3", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
			HANDLE hDest = CreateFile(destNames, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			//移动文件指针到文件头，因为之前写数据指针已到尾部了
			SetFilePointer(pSource, 0, NULL, FILE_BEGIN);
			while (ReadFile(pSource, pBuffer, samples_per_pass, &readBytes, NULL) && readBytes) {
				pData = shine_encode_buffer_interleaved(st, pBuffer, &readBytes);
				if (readBytes) {
					WriteFile(hDest, pData, readBytes, &dwreturnsize, NULL);
				}
			}
			pData = shine_flush(st, &readBytes);
			if (readBytes) {
				WriteFile(hDest, pData, readBytes, &dwreturnsize, NULL);
			}
			GlobalFree(pBuffer);
			CloseHandle(hDest);
		}
	}
}