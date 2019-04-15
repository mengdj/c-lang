#pragma once
#ifndef DEC_INCLUDE_LAME_MP3_H
#define DEC_INCLUDE_LAME_MP3_H

#include "stdafx.h"

typedef struct {
	LPVOID st;
	BOOL start;
	INT samples_per_pass;
	INT channels;
	LPVOID *buff;
	LPVOID target;
	BOOL stat;
} T_MP3, *LP_T_MP3;

LP_T_MP3	LAME_Init_Encoder(int, int, int, int, int);
BYTE*		LAME_Encode_Buffer_Interleaved(LP_T_MP3, INT16 *,INT, INT*);
BYTE*		LAME_Encode_Flush(LP_T_MP3, INT*);
INT			LAME_Get_Samples_Per_Pass(LP_T_MP3);
BOOL		LAME_StartAnalysis_Encoder(LP_T_MP3,LPVOID);
BOOL		LAME_EndAnalysis_Encoder(LP_T_MP3, LPVOID);
BOOL		LAME_Destory_Encoder(LP_T_MP3);
#endif