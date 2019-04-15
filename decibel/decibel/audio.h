#pragma once
#ifndef DEC_INCLUDE_AUDIO_H
#define DEC_INCLUDE_AUDIO_H

#include "stdafx.h"
#ifndef FRAGMENT_NUM
#define FRAGMENT_NUM 4
#endif
//每20ms秒取1次数据
#ifndef FRAGMENT_SIZE
#define FRAGMENT_SIZE 1024
#endif

typedef void(*DecWaveInProc)(
	HWAVEIN hwi,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
	);

BOOL		Audio_StartDec(DWORD, DWORD, DWORD, DWORD, DWORD, BOOL, DecWaveInProc);
BOOL		Audio_StopDec();
INT			Audio_Num();
BOOL		Audio_Reset();
BOOL		Audio_Name(CHAR *cName);
#endif
