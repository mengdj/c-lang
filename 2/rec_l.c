#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include "shine_mp3.h"
#pragma comment(lib,"winmm.lib")   
#pragma comment(lib,"user32.lib")
#define FRAGMENT_NUM 8
#define FRAGMENT_SIZE 1024

typedef unsigned char BYTE;
void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void writeMp3(FILE* pSource);

BOOL stopWrite = FALSE;
CRITICAL_SECTION  cs;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
		InitializeCriticalSection(&cs);
		waveInOpen(&hWaveIn, WAVE_MAPPER, &wavform, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
		WAVEHDR wh[FRAGMENT_NUM];
		//开辟一大快缓冲区避免重复分配内存
		BYTE *pBuffer = (BYTE*)GlobalAlloc(GMEM_ZEROINIT, FRAGMENT_SIZE*FRAGMENT_NUM);
		FILE *pFile = tmpfile();
		for (int i = 0; i < FRAGMENT_NUM; i++) {
			wh[i].lpData = pBuffer + (i*FRAGMENT_SIZE);
			wh[i].dwBufferLength = FRAGMENT_SIZE;
			wh[i].dwBytesRecorded = 0;
			wh[i].dwUser = (DWORD_PTR)pFile;
			wh[i].dwFlags = 0;
			wh[i].dwLoops = 1;
			wh[i].lpNext = NULL;
			wh[i].reserved = 0;
			waveInPrepareHeader(hWaveIn, &wh[i], sizeof(WAVEHDR));
			waveInAddBuffer(hWaveIn, &wh[i], sizeof(WAVEHDR));
		}
		waveInStart(hWaveIn);
		//录制60秒钟
		Sleep(60000);
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
		writeMp3(pFile);
		if (pFile != NULL) {
			fclose(pFile);
		}
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
		FILE *pFile = (FILE*)pwh->dwUser;
		if (pFile != NULL) {
			fwrite(pwh->lpData, pwh->dwBytesRecorded, 1, pFile);
		}
		EnterCriticalSection(&cs);
		if (stopWrite == FALSE) {
			waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
		}else {
			fflush(pFile);
		}
		LeaveCriticalSection(&cs);
	}
}

/************************************************************************/
/* 写入mp3                                                              */
/************************************************************************/
void writeMp3(FILE* pSource) {
	if (pSource != NULL) {
		shine_config_t config;
		int16_t *pBuffer;
		unsigned char * pData = NULL;
		int writeBytes = 0, readBytes = 0, samples_per_pass = 0;
		if (!fseek(pSource, 0, SEEK_END)) {
			readBytes = ftell(pSource);
			fseek(pSource, 0, SEEK_SET);
		}
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
			FILE* pDest = fopen(destNames, "wb+");
			while (fread(pBuffer, samples_per_pass, 1, pSource) != 0) {
				pData = shine_encode_buffer_interleaved(st, pBuffer, &writeBytes);
				if (writeBytes) {
					fwrite(pData, writeBytes, 1, pDest);
				}
			}
			pData = shine_flush(st, &writeBytes);
			if (writeBytes) {
				fwrite(pData, writeBytes, 1, pDest);
			}
			GlobalFree(pBuffer);
			fclose(pDest);
		}
	}
}