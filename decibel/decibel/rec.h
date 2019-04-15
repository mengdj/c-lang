#pragma once
#ifndef DEC_INCLUDE_SHINE_H
#define DEC_INCLUDE_SHINE_H

#include "stdafx.h"
#include "shine_mp3.h"

typedef struct {
	LPVOID st;
	BOOL start;
	INT samples_per_pass;
	INT channels;
	void *buff;
} T_MP3, *LP_T_MP3;

LP_T_MP3	Init_Encoder(int, int, int, int, int);
BYTE		 *Encode_Buffer_Interleaved(LP_T_MP3, INT16 *,INT, INT*);
BYTE		 *Encode_Flush(LP_T_MP3, INT*);
INT			 Get_Samples_Per_Pass(LP_T_MP3);
BOOL		 Destory_Encoder(LP_T_MP3);
BOOL		StartAnalysis_Encoder(LP_T_MP3, LPVOID);
BOOL		EndAnalysis_Encoder(LP_T_MP3, LPVOID);

#endif