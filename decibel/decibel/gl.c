//http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
//https://www.jianshu.com/p/88cb8da9a494?utm_campaign=maleskine&utm_content=note&utm_medium=reader_share&utm_source=qzone
#include "stdafx.h"
#include "gl.h"
#define REN_FRAME_COUNT		1024
#define REN_REL_VVAL		0.001953125f		// 2/1024
#define MAX_CHAR			128
#define OPENGL_PI			3.1415926f
#define FFT_FACTOR			0.1f
#define	BACKGROUND_WIDTH	500
#define BACKGROUND_HEIGHT	130
#ifndef MAX_LOADSTRING
#define MAX_LOADSTRING		128
#endif // !

static HGLRC				hRC = NULL;
static HDC					hDC = NULL;
static HWND					hOpenGlWnd = NULL;
static HINSTANCE			hInst = NULL;
static GLfloat				aHdata[REN_FRAME_COUNT];
static GLfloat				aVdata[3276];
static HFONT				hFont[2] = { 0 };
static HANDLE				hFontsInstalled = NULL;
static GLfloat				iPointXPre = 0;
static GLuint				texturesIds[1];
static GLfloat				vPrecent = 0.0f;
static SIZE					sMsgSize = { 0,0 };
static CHAR* bgNames[2] = { "res/background_a.png","res/background_b.png" };
static kiss_fft_cfg			fft_cfg = NULL;
static INT					pre_channel = 0;

ATOM RegisterGlCls(WNDPROC wndProc, HINSTANCE hInstance) {
	hInst = hInstance;
	WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR_DEF));
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("OpenGL");
	return RegisterClass(&wc);
}

BOOL UnInitGlWindow() {
	BOOL ret = TRUE;
	glDeleteTextures(1, texturesIds);
	if (hRC) {
		if (hRC) {
			if (!wglMakeCurrent(NULL, NULL)) {
				ret = FALSE;
			}
			if (!wglDeleteContext(hRC)) {
				ret = FALSE;
			}
			hRC = NULL;
		}
		if (hFont[1]) {
			DeleteObject(SelectObject(hDC, hFont[1]));
		}
		if (hDC && !ReleaseDC(hOpenGlWnd, hDC)) {
			hDC = NULL;
			ret = FALSE;
		}
		if (hOpenGlWnd && !DestroyWindow(hOpenGlWnd)) {
			hOpenGlWnd = NULL;
		}
		if (!UnregisterClass("OpenGL", hInst)) {
			hInst = NULL;
			ret = FALSE;
		}
	}
	if (fft_cfg) {
		kiss_fft_free(fft_cfg);
		fft_cfg = NULL;
	}
	return ret;
}

VOID GlTextout(FLOAT x, FLOAT y, CONST WCHAR* wstring) {
	int len, i;
	HDC hDC = wglGetCurrentDC();
	GLuint list = glGenLists(1);

	len = 0;
	for (i = 0; wstring[i] != '\0'; ++i) {
		if (IsDBCSLeadByte(wstring[i]))
			++i;
		++len;
	}
	glRasterPos2f(x, y);
	// 逐个输出字符
	for (i = 0; i < len; ++i) {
		wglUseFontBitmapsW(hDC, wstring[i], 1, list);
		glCallList(list);
	}
	// 回收所有临时资源
	glDeleteLists(list, 1);
}

VOID  ReSizeGl(INT iX, INT iY, INT iWidth, INT iHeight) {
	vPrecent = 1.0f / (iWidth >> 1);
	sMsgSize.cx = sMsgSize.cy = 0;
	glViewport(iX, iY, iWidth, iHeight);
}

DWORD WINAPI RenderGl(LPVOID lpParameter) {
	static SYSTEMTIME sysTime = { 0 };
	kiss_fft_cpx kfc_in[REN_FRAME_COUNT] = { 0 }, kfc_out[REN_FRAME_COUNT] = { 0 };
	GLfloat gfPointVal[3][2] = { {-1.0f, 0.0f},{1.0f, 0.0f},{-1.0f,0.0f} }, gfCurrentColor[4] = { 0 };
	INT iSize = 0, iRead = 0, iIndex = 0, iCount = 0;
	INT16 iVal = 0;
	LP_OPEN_GL_REN_ENV pEnv = (LP_OPEN_GL_REN_ENV)lpParameter;
	if (pEnv) {
		if (*pEnv->bRunning == TRUE) {
			//复位坐标到中心点
			glMatrixMode(GL_MODELVIEW);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			glGetFloatv(GL_CURRENT_COLOR, gfCurrentColor);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texturesIds[0]);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 1.0);	//左下
			glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, 1.0, 1.0);		//左上
			glTexCoord2f(1.0, 0.0); glVertex3f(1.0, 1.0, 1.0);		//右上
			glTexCoord2f(1.0, 1.0); glVertex3f(1.0, -1.0, 1.0);		//右下
			glEnd();
			glDisable(GL_TEXTURE_2D);

			glColor3ub(0x94, 0xE6, 0x5C);
			glLineWidth(1.0f);
			glBegin(GL_LINES);
			glVertex2fv(&gfPointVal[0]);
			glVertex2fv(&gfPointVal[1]);
			glEnd();

			if (TryAcquireSRWLockShared(pEnv->pLock)) {
				//INFINITE
				iIndex = pEnv->pRingbuff->readIndex;
				iSize = PaUtil_GetRingBufferReadAvailable(pEnv->pRingbuff);
				iRead = MIN(iSize, REN_FRAME_COUNT);

				if (iSize > REN_FRAME_COUNT) {
					PaUtil_AdvanceRingBufferReadIndex(pEnv->pRingbuff, iSize - REN_FRAME_COUNT);
					iIndex = pEnv->pRingbuff->readIndex;
					iSize -= (iSize - REN_FRAME_COUNT);
				}
				if (iRead) {
					if (PaUtil_ReadRingBuffer(pEnv->pRingbuff, kfc_in, iRead) == iRead) {
						//kiss_fft(fft_cfg, kfc_in, kfc_out);
						iCount = 0;
						while (iCount < iRead) {
							iVal = kfc_in[iCount].r;
							if (iVal != 0) {
								glBegin(GL_LINES);
								if (iVal > 0) {
									gfPointVal[0][0] = -aHdata[iCount];
									gfPointVal[0][1] = aVdata[(int)iVal / 10];
									gfPointVal[1][0] = gfPointVal[0][0];
									gfPointVal[1][1] = 0;
								}
								else {
									iVal = abs(iVal);
									gfPointVal[0][0] = -aHdata[iCount];
									gfPointVal[0][1] = 0;
									gfPointVal[1][0] = gfPointVal[0][0];
									gfPointVal[1][1] = -aVdata[(int)iVal / 10];
									glColor3ub(0xFF, 0x74, 0x00);;
								}
								glColor3ub(0xFF, 0x74, 0x00);
								glVertex2fv(&gfPointVal[0]);
								glVertex2fv(&gfPointVal[1]);
								glEnd();
							}
							iCount++;
						}
					}
				}
				if (iCount && PaUtil_GetRingBufferReadAvailable(pEnv->pRingbuff) < REN_FRAME_COUNT) {
					pEnv->pRingbuff->readIndex = iIndex - 1;
					PaUtil_AdvanceRingBufferReadIndex(pEnv->pRingbuff, 1);
				}
				if (pEnv->pWVariable != NULL) {
					WakeConditionVariable(pEnv->pWVariable);
				}
				ReleaseSRWLockShared(pEnv->pLock);
			}
			//录音状态
			DWORD dTicket = GetTickCount();
			if (*pEnv->bRecord) {
				if (pEnv->bRestart) {
					ZeroMemory(&sysTime, sizeof(SYSTEMTIME));
					pEnv->bRestart = FALSE;
					pEnv->iTicket = 0;
				}
				else {
					if (pEnv->iMinizeTicket[1]) {
						//转化时间(从最小化和恢复来计算持续时间)
						INT iCalcSecond = (pEnv->iMinizeTicket[1] - pEnv->iMinizeTicket[0]) / 1000;
						if (iCalcSecond) {
							InterlockedAdd(&sysTime.wSecond, iCalcSecond);
							if (sysTime.wSecond >= 60) {
								InterlockedAdd(&sysTime.wMinute, sysTime.wSecond / 60);
								if (sysTime.wMinute >= 60) {
									InterlockedAdd(&sysTime.wHour, sysTime.wMinute / 60);
									InterlockedExchange(&sysTime.wMinute, sysTime.wMinute % 60);
								}
								InterlockedExchange(&sysTime.wSecond, sysTime.wSecond % 60);
							}
						}
						pEnv->iTicket = dTicket;
						pEnv->iMinizeTicket[0] = pEnv->iMinizeTicket[1] = 0;
					}
				}
				//1000ms
				if ((dTicket - pEnv->iTicket) > 1000) {
					pEnv->iTicket = dTicket;
					InterlockedAdd(&sysTime.wSecond, 1);
					if (sysTime.wSecond >= 60) {
						InterlockedExchange(&sysTime.wSecond, 0);
						InterlockedAdd(&sysTime.wMinute, 1);
						if (sysTime.wMinute >= 60) {
							InterlockedExchange(&sysTime.wMinute, 0);
							InterlockedAdd(&sysTime.wHour, 1);
						}
					}
				}
				glColor3ub(0xFF, 0x7C, 0x00);
				WCHAR cTimeStr[MAX_CHAR];
				swprintf_s(cTimeStr, MAX_CHAR, TEXT("%.2d:%.2d:%.2d"), sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
				//计算坐标
				if (sMsgSize.cx == 0) {
					GetTextExtentPoint32(wglGetCurrentDC(), cTimeStr, lstrlen(cTimeStr), &sMsgSize);
				}
				GlTextout(1 - vPrecent * (sMsgSize.cx + 7), -0.90f, cTimeStr);
			}
			glColor4fv(gfCurrentColor);
			glFlush();
			SwapBuffers(hDC);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL InitGlWindow(HWND hWnd, INT bits, INT iWidth, INT iHeight, mz_zip_archive * pZip) {
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,																//版本
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,															// 选定色彩gL深度
		0, 0, 0, 0, 0, 0,												// 忽略的色彩位
		0,																// 无Alpha缓存
		0,																// 忽略Shift Bit
		0,																// 无累加缓存
		0, 0, 0, 0,														// 忽略聚集位
		24,																// 16位 Z-缓存 (深度缓存)
		0,																// 无蒙板缓存
		0,																// 无辅助缓存
		PFD_MAIN_PLANE,													// 主绘图层
		0,																// Reserved
		0, 0, 0															// 忽略层遮罩
	};
	hOpenGlWnd = hWnd;
	GLuint iPixelFormat = 0;
	LPVOID resBuff = NULL;
	INT iSize = 0, iRealBytes = 0;
	hDC = GetDC(hWnd);
	if (!hDC) {
		UnInitGlWindow();
		return -1;
	}
	if (!(iPixelFormat = ChoosePixelFormat(hDC, &pfd))) {
		UnInitGlWindow();
		return -2;
	}
	if (!SetPixelFormat(hDC, iPixelFormat, &pfd)) {
		UnInitGlWindow();
		return -3;
	}
	DescribePixelFormat(hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	if (!(hRC = wglCreateContext(hDC))) {
		UnInitGlWindow();
		return -4;
	}
	if (!wglMakeCurrent(hDC, hRC)) {
		UnInitGlWindow();
		return -5;
	}
	ReSizeGl(0, 0, iWidth, iHeight);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLoadIdentity();
	//加载字体
	hFont[0] = CreateFontA(32, 0, 0, 0, FW_THIN, 0, 0, 0,
		GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "方正细黑一简体");
	if (hFont[0] != NULL) {
		hFont[1] = SelectObject(hDC, hFont[0]);
	}
#ifndef _DEBUG
	free(resBuff);
#endif
	resBuff = NULL;
	GLfloat iBaseVal = 32767 / 10;
	iBaseVal = 1 / iBaseVal;
	for (int i = 0; i <= 3276; i++) {
		aVdata[i] = i * iBaseVal;
	}
	//构建查询缓存)
	for (int j = 0; j < REN_FRAME_COUNT; j++) {
		aHdata[j] = 1 - REN_REL_VVAL * j;
	}
	iPointXPre = (GLfloat)(1.0f / iWidth);
	//纹理数据
	srand((unsigned)time(NULL));
	INT w = 0, h = 0, c = 0;
	glGenTextures(1, texturesIds);
	glEnable(GL_TEXTURE_2D);
	BYTE * resDecBuff = NULL;
	CHAR * pBackgroundName = NULL;
	WCHAR wName[MAX_LOADSTRING] = { 0 };
	if (GetPrivateProfileStringLocal(TEXT("OPENGL"), TEXT("background"), TEXT("NULL"), wName, MAX_LOADSTRING)) {
		if (lstrcmp(wName, TEXT("NULL")) != 0) {
			HANDLE hFile = CreateFile(
				wName,
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				0
			);
			if (hFile != INVALID_HANDLE_VALUE) {
				//resBuff
				if (iSize = GetFileSize(hFile, NULL)) {
					resBuff = malloc(iSize);
					if (!ReadFile(hFile, resBuff, iSize, &iRealBytes, NULL)) {
						free(resBuff);
						resBuff = NULL;
					}
				}
				CloseHandle(hFile);
			}
		}
	}

	if ((resBuff != NULL) || (iSize = LoadResourceFromZip(pZip, bgNames[(rand() % 2)], &resBuff))) {
		resDecBuff = stbi_load_from_memory((BYTE*)resBuff, iSize, &w, &h, &c, 0);
		if (resDecBuff != NULL) {
			//0
			glBindTexture(GL_TEXTURE_2D, texturesIds[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			pre_channel = c;
			if (c == 3) {
				//3通道
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, resDecBuff);
			}
			else {
				//4通道
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, resDecBuff);
			}
			stbi_image_free(resDecBuff);
		}
#ifndef _DEBUG
		free(resBuff);
#endif
		resBuff = NULL;
	}
	glDisable(GL_TEXTURE_2D);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	fft_cfg = kiss_fft_alloc(REN_FRAME_COUNT, 0, NULL, NULL);
	return TRUE;
}

BOOL	GlUpdateBackgroundFromFile(CONST CHAR * cName) {
	INT w = 0, h = 0, c = 0;
	BOOL ret = FALSE;
	BYTE* resDecBuff = stbi_load(cName, &w, &h, &c, 0);
	if (resDecBuff) {
		if (texturesIds[1]) {
			if (pre_channel == c && w == BACKGROUND_WIDTH && h == BACKGROUND_HEIGHT) {
				glBindTexture(GL_TEXTURE_2D, texturesIds[0]);
				if (c == 3) {
					//3通道
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, resDecBuff);
					ret = TRUE;
				}
				else {
					//4通道
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, resDecBuff);
				}
			}
			else {
				glDeleteTextures(1, texturesIds);
				glGenTextures(1, texturesIds);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, texturesIds[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				pre_channel = c;
				if (c == 3) {
					//3通道
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, resDecBuff);
					ret = TRUE;
				}
				else {
					//4通道
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, resDecBuff);
				}
			}
			if (ret) {
				WCHAR wName[MAX_LOADSTRING] = { 0 };
				Char2WChar(cName, wName);
				if (!WritePrivateProfileStringLocal(TEXT("OPENGL"), TEXT("background"), wName)) {
				}
			}
		}
		stbi_image_free(resDecBuff);
		resDecBuff = NULL;
	}
	return ret;
}

VOID GlReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels) {
	//字节对齐
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glReadPixels(0, 0, width, height, format, type, pixels);
	//图像翻转，由下到上转为由上到下
	GLubyte* bufferConv = (GLubyte*)malloc(width * height * 4);
	if (bufferConv) {
		GLubyte* pFlipBuffer = pixels;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width * 4; x++) {
				bufferConv[(height - 1 - y) * width * 4 + x] = pFlipBuffer[y * 4 * width + x];
			}
		}
		CopyMemory(pixels, bufferConv, width * height * 4);
		free(bufferConv);
	}
}