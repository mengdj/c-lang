#pragma once
#ifndef DEC_INCLUDE_DECIBEL_H
#define DEC_INCLUDE_DECIBEL_H

#include "stdafx.h"
#include "resource.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_MSC_SECURE_CRT
#include "stb_image.h"
#include "stb_image_write.h"
#endif

#ifdef _DEBUG
#ifndef STB_LEAKCHECK_IMPLEMENTATION
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#endif
#endif // DEBUG

#include "gl.h"
#define AUDIO_ENC_LAME_MP3
#ifdef AUDIO_ENC_LAME_MP3
//LAME
#include "rec_lame_mp3.h"
#define Init_Encoder				LAME_Init_Encoder
#define Encode_Buffer_Interleaved	LAME_Encode_Buffer_Interleaved
#define Encode_Flush				LAME_Encode_Flush
#define Get_Samples_Per_Pass		LAME_Get_Samples_Per_Pass
#define Destory_Encoder				LAME_Destory_Encoder
#define StartAnalysis_Encoder		LAME_StartAnalysis_Encoder
#define EndAnalysis_Encoder			LAME_EndAnalysis_Encoder
#else
//SHINE(록폴)
#include "rec.h"
#endif

#define AUDIO_INPUT_PORTAUDIO	1
#ifdef AUDIO_INPUT_PORTAUDIO
//WASAPI
#include "audio_portaudio.h"
#define Audio_StartDec	Audio_PortAudio_StartDec
#define Audio_StopDec	Audio_PortAudio_StopDec
#define Audio_Num		Audio_PortAudio_Num
#define Audio_Reset		Audio_PortAudio_Reset
#define Audio_Name		Audio_PortAudio_Name
#else
//WINMM(록폴)
#include "audio.h"
#endif

#include "util.h"
#include <shellapi.h>

#endif
