#pragma once
#ifndef DEC_INCLUDE_GL_H
#define DEC_INCLUDE_GL_H

#include "stdafx.h"
#include "resource.h"
#include "miniz.h"

typedef struct _OPEN_GL_REN_ENV_ {
	const BOOL *bRunning;						//运行（是否）
	const BOOL *bRecord;						//录音（是否）
	const BOOL *bMinimize;						//最小化（是否）
	PaUtilRingBuffer* pRingbuff;
	PSRWLOCK pLock;
	PCONDITION_VARIABLE pRVariable;
	PCONDITION_VARIABLE pWVariable;
	BOOL bRestart;								//重启定时器
	DWORD iTicket;
	DWORD iMinizeTicket[2];						//最小化和恢复的时间
	POINT point;								//坐标坐标
	RECT *pRect;
} OPEN_GL_REN_ENV, *LP_OPEN_GL_REN_ENV;

BOOL			InitGlWindow(HWND, INT, INT, INT, mz_zip_archive*);
BOOL			UnInitGlWindow();
ATOM			RegisterGlCls(WNDPROC, HINSTANCE);
DWORD WINAPI	RenderGl(LPVOID);
VOID			ReSizeGl(INT, INT, INT, INT);
VOID			GlReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
VOID			GlTextout(FLOAT, FLOAT, CONST CHAR*);

extern unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern void stbi_image_free(void *retval_from_stbi_load);
extern DWORD LoadResourceFromRes(HINSTANCE hInstace, int resId, LPVOID* outBuff, LPWSTR resType);
extern INT LoadResourceFromZip(mz_zip_archive *pZip, const char* pName, LPVOID *pBuff);

#endif
