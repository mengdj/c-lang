#pragma once
#ifndef DEC_INCLUDE_AUDIO_PORTAUDIO_H
#define DEC_INCLUDE_AUDIO_PORTAUDIO_H

#include "stdafx.h"
#ifndef FRAGMENT_NUM
#define FRAGMENT_NUM 8
#endif
#ifndef FRAGMENT_SIZE
#define FRAGMENT_SIZE 512
#endif
#include <mmsystem.h>

#pragma pack(4)
typedef void(*DecWaveInProc)(
	HWAVEIN hwi,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
	);

BOOL		Audio_PortAudio_StartDec(DWORD, DWORD, DWORD, DWORD, DWORD, BOOL, DecWaveInProc);
BOOL		Audio_PortAudio_StopDec();
INT			Audio_PortAudio_Num();
BOOL		Audio_PortAudio_Reset();
BOOL		Audio_PortAudio_Name(CHAR *cName);
#endif
