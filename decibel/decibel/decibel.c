#include "stdafx.h"
#include "decibel.h"
#define MAX_LOADSTRING					128
#define WS_WIDTH						400
#define WS_RESIZE_WIDTH					80
#define WS_HEIGHT						680
#define WS_DEC_HEIGHT					500
#define WS_ABOUT_CONTENT_L				180
#define NOTIFY_DEF_ID					100
#define IDM_NOTIFY_EXIT					101
#define IDM_NOTIFY_VERSION				102

#define PCM_CHANNEL						2
#define PCM_SAMPLESPERSEC				44100

#define MUTEXT_ONLY_INSTANCE			0
#define HANDLE_MAIN_WND					1
#define MUTEXT_RECORDER					2
#define ENC_OUT_CREATE_FILE				3
#define OPENGL_WND						4
#define THREAD_START_DEV				5
#define THREAD_UPNP_DEV					6
#define THREAD_SHINE					7
#define HANDLE_RES_ZIP					8
#define HANDLE_LOAD_GIF_EVENT			9
#define THREAD_LOAD_GIF_EVENT			10
#define HANDLE_DEF_FONT					11
#define HANDLE_TOOLTIP_WND				12

#define ICON_IDB_DEC					0
#define ICON_IDB_DEC_SMALL				1
#define ICON_IDB_REC					2

//读写锁
#define SHINE_RW_LOCK					0
#define DEC_RW_LOCK						1
#define OPENGL_RW_LOCK					2

#define SHINE_COND_PRO					0
#define SHINE_COND_CUS					1
#define DEC_COND_PRO					2

#define RING_IND_DEC_EFFECT				0
#define RING_IND_ENC_OUT				1
#define RING_IND_OPENGL_EFFECT			2

#define PCM_WEBRTC_PROCESS_FRAME		FRAGMENT_SIZE
#define LOG_TRACE(...)					DecLog(__LINE__, __VA_ARGS__)

typedef struct {
	LPBYTE				data;
	INT					width;
	INT					height;
	INT					comp;
	INT					len;
	INT					channel;
	INT					x;
	INT					y;
	INT					z;				//索引
	INT					d;				//gif delay
	INT					i;				//编号
	BOOL				u;				//是否使用
	LPVOID				o;				//其他对象指针
	HBITMAP				hBitmap;
} STB_IMAGE_DATA, * PSTB_IMAGE_DATA;

typedef struct {
	INT					x;
	INT					y;
	INT					c_x;
	INT					c_y;
	BOOL				l_button_down;
	BOOL				r_button_down;
} MOUSE_POS;

typedef struct {
	INT					dec_devs[3];
	INT					dec_type;
	BOOL				start;
	BOOL				first_run;
	BOOL				stop_loading;
	BOOL				minimize;
	BOOL				about;
	BOOL				resize;
	BOOL				close_hover;
	BOOL				dec_hover;
	BOOL				dec_record_hover;
	BOOL				dec_cut_hover;
	BOOL				dec_update_hover;
	BOOL				dec_min_hover;
	BOOL				dec_about_hover;
	BOOL				dec_gif_hover;
	BOOL				dec_resize_hover;
	BOOL				dec_refresh;
	HFONT				hFont[4];
	HPEN				hPen[3];
	HBRUSH				hBrush[2];
	HICON				hIcon;
	INT					min_db;
	INT					max_db;
	INT					current_db;
} WIN_FORM;

typedef struct {
	LPBYTE				mem;
	INT					length;
	PaUtilRingBuffer* ring[3];
} RING_BUFF;

typedef struct {
	mz_zip_archive		zip;
	LPVOID				data;
	BOOL				init;
} MZ_RES_ZIP, * LP_MZ_RES_ZIP;

typedef struct {
	INT delay;
	INT start;
	BOOL display;
	COLORREF background;
	COLORREF color;
	CONST WCHAR msg[MAX_LOADSTRING];
	SIZE size;
} MSG_TIP;

LP_T_MP3				szRecEnc = NULL;
HINSTANCE				hInst;                                // 当前实例
WCHAR					szTitle[MAX_LOADSTRING];              // 标题栏文本
WCHAR					szWindowClass[MAX_LOADSTRING];        // 主窗口类名
WCHAR					szErrMsg[MAX_LOADSTRING];			  // 错误消息
WCHAR					szReadyMsg[MAX_LOADSTRING];			  // 等待消息
WCHAR					szRecEncName[MAX_LOADSTRING];		  // 编码数据存储文件名
PSTB_IMAGE_DATA			szRes[7][2];						  // 各种按钮资源
PSTB_IMAGE_DATA			szResGif[1] = { NULL };				  //加载动画
PSTB_IMAGE_DATA			szResTip[1] = { NULL };				  //加载动画
PSTB_IMAGE_DATA			szResAbout[2] = { NULL,NULL };
MOUSE_POS				szMouse = { 0,0,0,0,0,0 };
WIN_FORM				szForm = { {0,0,0},1,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,{NULL,NULL,NULL,NULL},{NULL,NULL,NULL},{NULL,NULL},NULL,0,0,0 };
RECT					sClientRect = { 0 }, sUpdateRect = {0}, sAboutContentRect = { 0 }, sMenuBarRect = { 0 }, sAnalysisRect = { 0 }, sResizeRect = { 0 }, sAboutRect = { 0 }, sRecordRect = { 0 }, sMinRect = { 0 }, sCloseRect = { 0 }, sCutRect = { 0 }, sDecValRect = { 0 }, sContentRect = { 0 };
HANDLE					szHandle[13] = { NULL,NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL };
RING_BUFF				szRing = { NULL,0,{NULL,NULL,NULL} }; //环形缓冲区读取音频数据
FILE* szLog = NULL;
INT						szAlignBlock = sizeof(long);		  //内存对齐
SRWLOCK					szRwLock[3];						  //读写锁
CONDITION_VARIABLE		szRwCond[3];						  //唤醒信号
LP_OPEN_GL_REN_ENV		szOpenglEnv = NULL;					  //OPENGL变量
NOTIFYICONDATA			szNotify;							  //系统托盘图标
HMENU					szNotifyMenu = NULL;				  //菜单
HICON					szIcon[3] = { NULL,NULL,NULL };		  //可重用图标
HDC						szMemDc = NULL;
HDC						szPngMemDc = NULL;
HBITMAP					szCompatibleBitmap = NULL;
MZ_RES_ZIP				szResZip = { 0 };
BYTE* szAboutBody = NULL;					//
MSG_TIP					szMsgTip = { 0 };
BOOL					szNeedUpdate = FALSE;
BOOL					szAllowValidate = TRUE;

VOID					ClearGifMemory();
VOID					CenterWindow(HWND);
BOOL					CaptureAndBuildPng(HWND, CONST CHAR*);
BOOL					CaptureForm(HWND, LPBYTE, INT*, INT*, INT*, BOOL);
VOID NTAPI				CheckUpdateVerAdv(PTP_CALLBACK_INSTANCE, PVOID);
ATOM					DecRegisterClass(HINSTANCE);
void					DecibelWaveInProc(HWAVEIN, UINT, DWORD, DWORD, DWORD);
VOID					DecLog(const char* format, ...);
DWORD WINAPI			DecEventProcEx(LPVOID);
BOOL					GetBitmapFromRes(PSTB_IMAGE_DATA);
BOOL					IsNeedUpdate(const WCHAR*);
BOOL					InitInstance(HINSTANCE, int);
BOOL					InitOpenGlWin();
VOID					InitAllRect(HWND);
DWORD WINAPI			Mp3EncodeProc(LPVOID);
LRESULT CALLBACK		OpenGLProc(HWND, UINT, WPARAM, LPARAM);
BOOL					PreProcessCreate(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI			PnpDev(LPVOID);
BOOL					RenderAbout(HDC, HDC, const LPRECT);
BOOL					RenderAlphaBitmap(HDC, HDC, HBITMAP, RECT*, DWORD);
BOOL					RenderBitmap(HDC, HDC, HBITMAP, RECT*);
BOOL					RenderDecErr(HDC, HDC, const LPRECT);
BOOL					RenderDecFrame(HDC, HDC, const LPRECT);
DWORD WINAPI			StartDev(LPVOID);
HRESULT					UpdateDeviceContextEx(HDC, HBITMAP, LPRECT);
BOOL					UnPreProcessCreate(HWND, UINT, WPARAM, LPARAM);
VOID					UpdateTooltip(HWND, HWND, LPRECT, WCHAR*);
BOOL					InvalidateRectOnce(HWND, CONST RECT*, BOOL);
LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);
VOID					WriteMsgContext(INT, INT, COLORREF, COLORREF, CONST WCHAR*, BOOL);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DECIBEL, szWindowClass, MAX_LOADSTRING);
	LoadStringW(hInstance, IDS_ERR_MSG, szErrMsg, MAX_LOADSTRING);
	LoadStringW(hInstance, IDS_READY_MSG, szReadyMsg, MAX_LOADSTRING);

	//仅允许运行1个实例
	szHandle[MUTEXT_ONLY_INSTANCE] = OpenMutex(MUTEX_ALL_ACCESS, FALSE, szTitle);
	if (!szHandle[MUTEXT_ONLY_INSTANCE]) {
		szHandle[MUTEXT_ONLY_INSTANCE] = CreateMutex(NULL, FALSE, szTitle);
	}
	else {
		return 0;
	}
	if (!DecRegisterClass(hInstance)) {
		return FALSE;
	}
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

ATOM DecRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = szIcon[ICON_IDB_DEC] = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DECIBEL));
	wcex.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR_DEF));
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = szIcon[ICON_IDB_DEC_SMALL] = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

//初始化实例
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance;
	HWND hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, szWindowClass, szTitle, WS_POPUP, 0, 0, WS_WIDTH, WS_HEIGHT, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		return FALSE;
	}
	CenterWindow(hWnd);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

/** 重资源文件中获取位图 */
BOOL GetBitmapFromRes(PSTB_IMAGE_DATA pId) {
	if (!pId->hBitmap) {
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = pId->width;
		bmi.bmiHeader.biHeight = -pId->height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = pId->width * pId->height * 4;
		LPBYTE pDest = NULL;
		HBITMAP hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)& pDest, NULL, 0);
		if (hBitmap) {
			BGRA2RGBA(pId->data, pDest, pId->width, pId->height, TRUE);
			pId->hBitmap = hBitmap;
		}
	}
	return pId->hBitmap ? TRUE : FALSE;
}

/** 更新提示信息 */
VOID UpdateTooltip(HWND hWndTool, HWND hWndOwner, LPRECT lPRect, WCHAR* wTip) {
	static RECT prevRect = { 0 };
	static BOOL	bFirstEnter = TRUE;
	if (lPRect && wTip) {
		if (bFirstEnter) {
			TOOLINFO tti = { 0 };
			memset(&tti, 0, sizeof(TOOLINFO));
			tti.cbSize = sizeof(TOOLINFO);
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = hWndOwner;
			tti.rect = *lPRect;
			tti.uId = 0;
			tti.lpszText = wTip;
			SendMessage(hWndTool, TTM_ADDTOOL, 0, (LPARAM)& tti);
			CopyRect(&prevRect, lPRect);
			bFirstEnter = FALSE;
		}
		else {
			if (!EqualRect(&prevRect, lPRect)) {
				TOOLINFO tti = { 0 };
				memset(&tti, 0, sizeof(TOOLINFO));
				tti.cbSize = sizeof(TOOLINFO);
				tti.uFlags = TTF_SUBCLASS;
				tti.hwnd = hWndOwner;
				tti.rect = *lPRect;
				tti.uId = 0;
				tti.lpszText = wTip;
				SendMessage(hWndTool, TTM_SETTOOLINFO, 0, (LPARAM)& tti);
				CopyRect(&prevRect, lPRect);
			}
		}
	}
}
/** 开启声音采集 */
DWORD WINAPI StartDev(LPVOID lpParameter) {
	if (Audio_StartDec(WAVE_FORMAT_PCM, PCM_CHANNEL, PCM_SAMPLESPERSEC, 4, 16, FALSE, (DecWaveInProc)DecibelWaveInProc)) {
		INT iSize = sizeof(PaUtilRingBuffer);
		//读写锁以及生产消费模式
		InitializeConditionVariable(&szRwCond[SHINE_COND_PRO]);
		InitializeConditionVariable(&szRwCond[SHINE_COND_CUS]);
		InitializeSRWLock(&szRwLock[SHINE_RW_LOCK]);
		//共享一段内存内存(分贝+编码器+重采样+OPENGL分析+OPENGL环境+4个内存缓冲区指针)
		szRecEnc = Init_Encoder(PCM_SAMPLESPERSEC, PCM_CHANNEL, 0, 128, 1);
		szRing.length = 256 + 8192 + sizeof(kiss_fft_cpx) * 4096 + szRecEnc->samples_per_pass + sizeof(OPEN_GL_REN_ENV) + iSize * 3;
		szRing.mem = VirtualAlloc(NULL, szRing.length, MEM_COMMIT, PAGE_READWRITE);
		if (!szRing.mem) {
			LOG_TRACE("VirtualAlloc分配失败:%d", szRing.length);
		}
		else {
			ZeroMemory(szRing.mem, szRing.length);
			BYTE* pShare = szRing.mem;
			szRing.ring[RING_IND_DEC_EFFECT] = pShare;
			pShare += iSize;
			szRing.ring[RING_IND_ENC_OUT] = pShare;
			pShare += iSize;
			szRing.ring[RING_IND_OPENGL_EFFECT] = pShare;
			pShare += iSize;
			if (PaUtil_InitializeRingBuffer(szRing.ring[RING_IND_DEC_EFFECT], 4, 64, pShare) == -1) {
				LOG_TRACE("缓冲池创建失败:%d", 32);
			}
			//缓存pcm数据
			pShare += 256;
			if (PaUtil_InitializeRingBuffer(szRing.ring[RING_IND_ENC_OUT], 1, 8192, pShare) == -1) {
				LOG_TRACE("缓冲池创建失败1:%d", 8192);
			}
			pShare += 8192;
			if (PaUtil_InitializeRingBuffer(szRing.ring[RING_IND_OPENGL_EFFECT], sizeof(kiss_fft_cpx), 4096, pShare) == -1) {
				LOG_TRACE("缓冲池创建失败2:%d", 4096);
			}
			pShare += sizeof(kiss_fft_cpx) * 4096;
			szRecEnc->buff = pShare;
			InterlockedExchange(&szRecEnc->start, FALSE);
			szForm.dec_devs[1] = szForm.dec_devs[0];
			if (szHandle[OPENGL_WND]) {
				pShare += szRecEnc->samples_per_pass;
				szOpenglEnv = (LP_OPEN_GL_REN_ENV)pShare;
				if (szOpenglEnv) {
					ZeroMemory(szOpenglEnv, sizeof(OPEN_GL_REN_ENV));
					InitializeSRWLock(&szRwLock[OPENGL_RW_LOCK]);
					szOpenglEnv->bRunning = &szForm.start;
					szOpenglEnv->bRecord = &szRecEnc->start;
					szOpenglEnv->pRVariable = NULL;
					szOpenglEnv->pWVariable = NULL;
					szOpenglEnv->pLock = &szRwLock[OPENGL_RW_LOCK];
					szOpenglEnv->pRingbuff = szRing.ring[RING_IND_OPENGL_EFFECT];
					szOpenglEnv->bRestart = FALSE;
					szOpenglEnv->bMinimize = &szForm.minimize;
					szOpenglEnv->iMinizeTicket[0] = szOpenglEnv->iMinizeTicket[1] = 0;
					szOpenglEnv->pRect = &sAnalysisRect;
				}
			}
			if (szHandle[HANDLE_MAIN_WND] != NULL) {
				InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], NULL, FALSE);
				InterlockedExchange(&szForm.dec_refresh, TRUE);
			}
			return TRUE;
		}
	}
	return FALSE;
}

//音频编码
DWORD WINAPI Mp3EncodeProc(LPVOID lpParameter) {
	CONST INT iNsFrame = szRecEnc->samples_per_pass;
	INT iSize = 0, iWriteBytes = 0, iReadBytes = 0, iRealBytes = 0, iProcess = 0;
	BYTE* pData = NULL;
	BYTE aSpeexDspBuff[PCM_SAMPLESPERSEC << 1];
	while (szRecEnc->start == TRUE) {
		if (szHandle[ENC_OUT_CREATE_FILE]) {
			if (TryAcquireSRWLockShared(&szRwLock[SHINE_RW_LOCK])) {
				if (SleepConditionVariableSRW(&szRwCond[SHINE_COND_CUS], &szRwLock[SHINE_RW_LOCK], INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED)) {
					//处理编码数据
					iSize = PaUtil_GetRingBufferReadAvailable(szRing.ring[RING_IND_ENC_OUT]) / iNsFrame;
					if (iSize) {
						while (iSize > 0) {
							if ((iReadBytes = PaUtil_ReadRingBuffer(szRing.ring[RING_IND_ENC_OUT], szRecEnc->buff, iNsFrame)) == iNsFrame) {
								pData = Encode_Buffer_Interleaved(szRecEnc, (INT16*)szRecEnc->buff, iReadBytes, &iWriteBytes);
								if (iWriteBytes) {
									//写入mp3数据
									WriteFile(szHandle[ENC_OUT_CREATE_FILE], pData, iWriteBytes, &iRealBytes, NULL);
								}
							}
							WakeConditionVariable(&szRwCond[SHINE_COND_PRO]);
							iSize--;
						}
					}
					else {
						WakeConditionVariable(&szRwCond[SHINE_COND_PRO]);
					}
				}
				ReleaseSRWLockShared(&szRwLock[SHINE_RW_LOCK]);
			}
		}
	}
	//缓冲区里可能还有数据需要最后编码
	if ((iReadBytes = PaUtil_ReadRingBuffer(szRing.ring[RING_IND_ENC_OUT], szRecEnc->buff, szRecEnc->samples_per_pass))) {
		pData = Encode_Buffer_Interleaved(szRecEnc, (INT16*)szRecEnc->buff, iReadBytes, &iWriteBytes);
		if (iWriteBytes) {
			WriteFile(szHandle[ENC_OUT_CREATE_FILE], pData, iWriteBytes, &iRealBytes, NULL);
			pData = Encode_Flush(szRecEnc, &iWriteBytes);
			if (iWriteBytes) {
				WriteFile(szHandle[ENC_OUT_CREATE_FILE], pData, iWriteBytes, &iRealBytes, NULL);
			}
		}
	}
	else {
		//刷新缓冲区
		pData = Encode_Flush(szRecEnc, &iWriteBytes);
		if (iWriteBytes) {
			WriteFile(szHandle[ENC_OUT_CREATE_FILE], pData, iWriteBytes, &iRealBytes, NULL);
		}
	}
	PaUtil_FlushRingBuffer(szRing.ring[RING_IND_ENC_OUT]);
	FlushFileBuffers(szHandle[ENC_OUT_CREATE_FILE]);
	EndAnalysis_Encoder(szRecEnc, szHandle[ENC_OUT_CREATE_FILE]);
	return TRUE;
}

//扫描设备(500毫秒扫描1次),仅启动时未找到设备开启扫描线程
DWORD WINAPI PnpDev(LPVOID lpParameter) {
	BOOL ret = FALSE;
	while (TRUE) {
		if ((szForm.dec_devs[0] = Audio_Num())) {
			ret = StartDev(lpParameter);
			break;
		}
		Sleep(500);
	}
	CloseHandle(szHandle[THREAD_UPNP_DEV]);
	szHandle[THREAD_UPNP_DEV] = NULL;
	return ret;
}

VOID	InitAllRect(HWND hWnd) {
	PSTB_IMAGE_DATA sTmpPid = NULL;
	GetClientRect(hWnd, &sClientRect);
	//波形图
	sDecValRect.left = 1;
	sDecValRect.top = sClientRect.bottom - WS_DEC_HEIGHT;
	sDecValRect.right = sClientRect.right;
	sDecValRect.bottom = sClientRect.bottom - 1;

	// 虚拟客户区
	sContentRect.left = 1;
	sContentRect.top = 40;
	sContentRect.right = sClientRect.right - 1;
	sContentRect.bottom = sClientRect.bottom - 1;

	// 顶部区域
	sAnalysisRect.left = sClientRect.left;
	sAnalysisRect.top = 40;
	sAnalysisRect.right = sClientRect.right;
	sAnalysisRect.bottom = sClientRect.top + (WS_HEIGHT - WS_DEC_HEIGHT - 40);

	//菜单栏
	sMenuBarRect.left = sClientRect.left;
	sMenuBarRect.top = sClientRect.top;
	sMenuBarRect.right = sClientRect.right;
	sMenuBarRect.bottom = 40;

	//关闭0
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[0][i];
		sTmpPid->x = sClientRect.right - sTmpPid->width;
		sTmpPid->y = 0;
	}
	sCloseRect.left = sTmpPid->x - 4;
	sCloseRect.top = sTmpPid->y + 8;
	sCloseRect.right = sCloseRect.left + sTmpPid->width;
	sCloseRect.bottom = sCloseRect.top + sTmpPid->height;

	//重置1
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[1][i];
		sTmpPid->x = sCloseRect.left - sTmpPid->width;
		sTmpPid->y = 5;
	}
	sResizeRect.left = sTmpPid->x - 4;
	sResizeRect.top = sTmpPid->y + 3;
	sResizeRect.right = sResizeRect.left + sTmpPid->width;
	sResizeRect.bottom = sResizeRect.top + sTmpPid->height;

	//小化2
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[2][i];
		sTmpPid->x = sResizeRect.left - sTmpPid->width;
		sTmpPid->y = 5;
	}
	sMinRect.left = sTmpPid->x - 4;
	sMinRect.top = sTmpPid->y + 3;
	sMinRect.right = sMinRect.left + sTmpPid->width;
	sMinRect.bottom = sMinRect.top + sTmpPid->height;

	//关于3
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[3][i];
		sTmpPid->x = sMinRect.left - sTmpPid->width;
		sTmpPid->y = sMinRect.top;
	}
	sAboutRect.left = sTmpPid->x - 4;
	sAboutRect.top = sTmpPid->y - 1;
	sAboutRect.right = sAboutRect.left + sTmpPid->width;
	sAboutRect.bottom = sAboutRect.top + sTmpPid->height;


	//录音4
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[4][i];
		sTmpPid->x = sDecValRect.right - sTmpPid->width;
		sTmpPid->y = sDecValRect.top;
	}
	sRecordRect.left = sTmpPid->x - 6;
	sRecordRect.top = sTmpPid->y + 12;
	sRecordRect.right = sRecordRect.left + sTmpPid->width;
	sRecordRect.bottom = sRecordRect.top + sTmpPid->height;

	//截图5
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[5][i];
		sTmpPid->x = sRecordRect.left - sTmpPid->width;
		sTmpPid->y = sDecValRect.top;
	}
	sCutRect.left = sTmpPid->x - 6;
	sCutRect.top = sTmpPid->y + 12;
	sCutRect.right = sCutRect.left + sTmpPid->width;
	sCutRect.bottom = sCutRect.top + sTmpPid->height;

	//关于
	sAboutContentRect.left = ALIGN_OF(sDecValRect.left + WS_ABOUT_CONTENT_L, 2);
	sAboutContentRect.top = sDecValRect.top;
	sAboutContentRect.right = sDecValRect.right;
	sAboutContentRect.bottom = sDecValRect.bottom;

	//更新功能
	for (int i = 0; i < 2; i++) {
		sTmpPid = szRes[6][i];
		sTmpPid->x = sAboutRect.left - sTmpPid->width;
		sTmpPid->y = sAboutRect.top;
	}
	sUpdateRect.left = sTmpPid->x - 4;
	sUpdateRect.top = sTmpPid->y +1;
	sUpdateRect.right = sUpdateRect.left + sTmpPid->width;
	sUpdateRect.bottom = sUpdateRect.top + sTmpPid->height;
}

//检查版本号
BOOL	IsNeedUpdate(const WCHAR* pVer) {
	if (!szNeedUpdate) {
		WCHAR* next_token = NULL, * ver = wcstok_s(pVer, ".", &next_token);
		INT iCouVer = 0;
		INT	iArrVouVer[COU_VER] = { MAJ_VER,MID_VER ,MIN_VER };
		while (iCouVer < COU_VER && ver) {
			if (iswalnum(ver) == 0) {
				if (atoi(ver) > iArrVouVer[iCouVer]) {
					szNeedUpdate = TRUE;
					break;
				}
			}
			ver = wcstok_s(NULL, ".", &next_token);
			iCouVer++;
		}
	}
	return szNeedUpdate;
}

//检查版本号
size_t CheckVer(VOID *ptr, size_t size, size_t nmemb, VOID *stream) {
	//9.9.9
	if (nmemb == 6) {
		WCHAR wSVer[MAX_LOADSTRING] = { 0 }, wDVer[MAX_LOADSTRING] = { 0 };
		Char2WChar(ptr, wSVer);
		CopyMemory(wDVer, wSVer, MAX_LOADSTRING);
		szNeedUpdate = IsNeedUpdate(wSVer);
		if (szNeedUpdate) {
			WCHAR wMsg[MAX_LOADSTRING] = { 0 };
			WritePrivateProfileStringLocal(TEXT("DEC"), TEXT("version"), wDVer);
			swprintf_s(wMsg, MAX_LOADSTRING, TEXT("检测到新版本:%s,建议尽快更新"), wDVer);
			WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0x82, 0x84, 0xFD), wMsg, TRUE);
		}
	}
	return size* nmemb;
}

//检查版本更新信息
VOID NTAPI	CheckUpdateVerAdv(PTP_CALLBACK_INSTANCE Instance, PVOID Context) {
	CURLcode res = CURL_LAST;
	if (!szNeedUpdate) {
		CURL* curl = curl_easy_init();
		if (curl) {
			if ((res = curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/mengdj/c-lang/master/decibel/Release/ver")) == CURLE_OK) {
				if ((res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CheckVer)) == CURLE_OK) {
					if ((res = curl_easy_perform(curl)) != CURLE_OK) {
						LOG_TRACE("CheckUpdateVerAdv(curl_easy_perform):%s", curl_easy_strerror(res));
					}
				}
				else {
					LOG_TRACE("CheckUpdateVerAdv(CURLOPT_WRITEFUNCTION):%s", curl_easy_strerror(res));
				}
			}
			else {
				LOG_TRACE("CheckUpdateVerAdv(CURLOPT_URL):%s", curl_easy_strerror(res));
			}
			curl_easy_cleanup(curl);
			curl = NULL;
		}
	}
}

//资源初始化
BOOL PreProcessCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	DWORD resSize = 0;
	LPVOID resBuff = NULL;
	PSTB_IMAGE_DATA sTmpPid = NULL;
	INT iSize = 0;
	CHAR* resNames[7][2] = {
		{"res/close_normal.png","res/close_active.png"},
		{"res/resize_normal.png","res/resize_active.png"},
		{"res/min_normal.png","res/min_active.png"},
		{"res/about_normal.png","res/about_active.png"},
		{"res/rec_normal.png","res/rec_active.png"},
		{"res/cut_normal.png","res/cut_active.png"},
		{"res/update_normal.png","res/update_active.png"},
	};
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE))) {
		LOG_TRACE("CoInitialize:Failture");
	}
	szHandle[HANDLE_MAIN_WND] = hWnd;
	//初始化压缩资源
	ZeroMemory(&szResZip, sizeof(MZ_RES_ZIP));
	if (mz_zip_reader_init_file(&szResZip.zip, "resources.dat", 0)) {
		szResZip.init = TRUE;
	}
	else {
		if (szResZip.data) {
			free(szResZip.data);
			szResZip.data = NULL;
		}
		MessageBox(hWnd, TEXT("资源文件丢失，请检查程序是否完整"), TEXT("错误"), MB_ICONSTOP);
		TerminateProcess(GetCurrentProcess(), 0);
	}
	if ((iSize = LoadResourceFromZip(&szResZip.zip, "res/default.ttf", &resBuff))) {
		DWORD nFontsInstalled = 0;
		if (!(szHandle[HANDLE_DEF_FONT] = AddFontMemResourceEx(resBuff, iSize, NULL, &nFontsInstalled)) != NULL) {
			LOG_TRACE("字体加载失败");
		}
		free(resBuff);
		resBuff = NULL;
	}
	for (int w = 0; w < 7; w++) {
		for (int h = 0; h < 2; h++) {
			if (resNames[w][h]) {
				if ((resSize = LoadResourceFromZip(&szResZip.zip, resNames[w][h], &resBuff))) {
					sTmpPid = (PSTB_IMAGE_DATA)_aligned_malloc(sizeof(STB_IMAGE_DATA), szAlignBlock);
					memset(sTmpPid, 0, sizeof(STB_IMAGE_DATA));
					sTmpPid->channel = 4;
					sTmpPid->data = NULL;
					sTmpPid->len = resSize;
					sTmpPid->z = 0;
					sTmpPid->d = 0;
					sTmpPid->i = 0;
					sTmpPid->u = TRUE;
					//stb解码图片资源
					sTmpPid->data = stbi_load_from_memory(
						(stbi_uc*)resBuff,
						resSize,
						&sTmpPid->width,
						&sTmpPid->height,
						&sTmpPid->comp,
						sTmpPid->channel
					);
					if (sTmpPid->data != NULL) {
						if (GetBitmapFromRes(sTmpPid) == TRUE) {
							stbi_image_free(sTmpPid->data);
						}
						szRes[w][h] = sTmpPid;
					}
					//释放内存
					free(resBuff);
					resBuff = NULL;
				}
			}
			else {
				szRes[w][h] = NULL;
			}
		}
		sTmpPid = NULL;
	}
	InitAllRect(hWnd);
	//字体
	szForm.hFont[0] = CreateFont(
		20, 0, 0, 0,
		FW_THIN, FALSE, FALSE, FALSE,
		GB2312_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		FF_MODERN, TEXT("方正细黑一简体")
	);
	szForm.hFont[2] = CreateFont(
		16, 0, 0, 0,
		FW_THIN, FALSE, FALSE, FALSE,
		GB2312_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		FF_MODERN, TEXT("方正细黑一简体")
	);
	szForm.hPen[0] = CreatePen(PS_SOLID, 2, RGB(0xFF, 0x79, 0x00));
	szForm.hPen[1] = CreatePen(PS_NULL, 2, RGB(0x00, 0x7A, 0xFF));
	szForm.hPen[2] = CreatePen(PS_DOT, 1, RGB(0xC5, 0xC5, 0xC5));
	szForm.hBrush[0] = CreateSolidBrush(RGB(0xB7, 0x63, 0xC5));
	//背景画刷
	szForm.hBrush[1] = CreateSolidBrush(RGB(0x1E, 0x1E, 0x1E));
	InitializeSRWLock(&szRwLock[DEC_RW_LOCK]);
	InitializeConditionVariable(&szRwCond[DEC_COND_PRO]);

	if (!InitOpenGlWin()) {
		DestroyWindow(szHandle[OPENGL_WND]);
	}
	szForm.dec_devs[0] = Audio_Num();
	szHandle[HANDLE_LOAD_GIF_EVENT] = CreateEvent(NULL, FALSE, TRUE, NULL);
	//GIF(渲染线程)
	szHandle[THREAD_LOAD_GIF_EVENT] = _beginthreadex(NULL, 0, DecEventProcEx, GetDC(hWnd), 0, NULL);
	InvalidateRectOnce(hWnd, &sDecValRect, FALSE);
	if (szForm.dec_devs[0]) {
		//准备音频设备
		szHandle[THREAD_START_DEV] = _beginthreadex(NULL, 0, StartDev, NULL, 0, NULL);
	}
	else {
		//开启一个扫描线程扫描硬件改动
		szHandle[THREAD_UPNP_DEV] = _beginthreadex(NULL, 0, PnpDev, NULL, 0, NULL);
	}
	//添加托盘图标
	ZeroMemory(&szNotify, sizeof(NOTIFYICONDATA));
	szNotify.cbSize = sizeof(NOTIFYICONDATA);
	szNotify.hWnd = hWnd;
	szNotify.uID = NOTIFY_DEF_ID;
	szNotify.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	szNotify.uCallbackMessage = WM_USER;
	szNotify.hIcon = szIcon[ICON_IDB_DEC];
	lstrcpy(szNotify.szTip, szTitle);
	Shell_NotifyIcon(NIM_ADD, &szNotify);
	szNotifyMenu = CreatePopupMenu();
	AppendMenu(szNotifyMenu, MF_STRING, IDM_NOTIFY_VERSION, TEXT("更新"));
	AppendMenu(szNotifyMenu, MF_STRING, IDM_NOTIFY_EXIT, TEXT("退出"));

	//提示
	szHandle[HANDLE_TOOLTIP_WND] = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hWnd,
		NULL,
		hInst,
		NULL
	);

	WCHAR wFirstRun[MAX_LOADSTRING];
	if (GetPrivateProfileStringLocal(TEXT("DEC"), TEXT("first_run"), TEXT("Y"), wFirstRun, MAX_LOADSTRING)) {
		if (lstrcmp(wFirstRun, TEXT("Y")) == 0) {
			szForm.first_run = TRUE;
			WritePrivateProfileStringLocal(TEXT("DEC"), TEXT("first_run"), TEXT("N"));
			if (!szResTip[0]) {
				sTmpPid = szResTip[0] = (PSTB_IMAGE_DATA)malloc(sizeof(STB_IMAGE_DATA));
				ZeroMemory(sTmpPid, sizeof(STB_IMAGE_DATA));
				if ((iSize = LoadResourceFromZip(&szResZip.zip, "res/tip.png", &resBuff))) {
					sTmpPid->i = GetTickCount();
					sTmpPid->channel = 4;
					sTmpPid->data = stbi_load_from_memory((BYTE*)resBuff, iSize, &sTmpPid->width, &sTmpPid->height, &sTmpPid->channel, sTmpPid->channel);
					sTmpPid->len = sTmpPid->width * sTmpPid->height * sTmpPid->channel;
					sTmpPid->d = 6000;
					sTmpPid->o = NULL;
					if (GetBitmapFromRes(sTmpPid)) {
						sTmpPid->x = 0;
						sTmpPid->y = 0;
					}
					else {
						stbi_image_free(sTmpPid->data);
						free(sTmpPid);
						sTmpPid = szResTip[0] = NULL;
					}
					free(resBuff);
					resBuff = NULL;
				}
			}
		}
	}
	else {
		szForm.first_run = TRUE;
		WritePrivateProfileStringLocal(TEXT("DEC"), TEXT("first_run"), TEXT("N"));
	}
	curl_global_init(CURL_GLOBAL_ALL);
	if (!szForm.first_run) {
		WCHAR wVer[MAX_LOADSTRING] = {0};
		if (GetPrivateProfileStringLocal(TEXT("DEC"), TEXT("version"), TEXT_CURRENT_VER, wVer, MAX_LOADSTRING)) {
			if (lstrcmp(wVer, TEXT_CURRENT_VER) != 0) {
				szNeedUpdate = TRUE;
			}
		}
	}
	TrySubmitThreadpoolCallback(CheckUpdateVerAdv, NULL, NULL);
	return TRUE;
}

DWORD WINAPI DecEventProcEx(LPVOID lpParameter) {
	INT iSize = 0;
	RECT sLoadingRect = { 0,0,0,0 };
	WCHAR* sMsg = NULL;
	SIZE sMsgSize = { 0,0 };
	LPRECT lPRect = &sContentRect;

	HDC hdc = (HDC)lpParameter;
	HDC hCompareDc = CreateCompatibleDC(hdc);
	HBITMAP hCompatibleBitmap = CreateCompatibleBitmap(hdc, lPRect->right, lPRect->bottom);
	HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
	HGDIOBJ hOldGdiObj = SelectObject(hCompareDc, hCompatibleBitmap);
	FillRect(hCompareDc, lPRect, hBackgroundBrush);
	SetBkMode(hCompareDc, TRANSPARENT);
	DeleteObject(SelectObject(hCompareDc, szForm.hFont[0]));
	ReleaseDC(szHandle[HANDLE_MAIN_WND], hdc);
	while (!szForm.stop_loading) {
		FillRect(hCompareDc, lPRect, hBackgroundBrush);
		sMsg = szForm.dec_devs[0] ? szReadyMsg : szErrMsg;
		GetTextExtentPoint32(hCompareDc, sMsg, lstrlen(sMsg), &sMsgSize);
		PSTB_IMAGE_DATA pTmpImage = szResGif[0];
		if (szResZip.init) {
			//图标所用的临时DC
			LPVOID pTmpGifEncBuff = NULL;
			if (pTmpImage == NULL) {
				//1
				if ((iSize = LoadResourceFromZip(&szResZip.zip, "res/loading.gif", &pTmpGifEncBuff))) {
					pTmpImage = szResGif[0] = (PSTB_IMAGE_DATA)malloc(sizeof(STB_IMAGE_DATA));
					ZeroMemory(pTmpImage, sizeof(STB_IMAGE_DATA));
					INT* iDelays = 0;
					pTmpImage->channel = 4;
					pTmpImage->i = 0;
					pTmpImage->data = stbi_load_gif_from_memory((stbi_uc*)pTmpGifEncBuff, iSize, &iDelays, &pTmpImage->width, &pTmpImage->height, &pTmpImage->z, &pTmpImage->comp, pTmpImage->channel);
					pTmpImage->len = pTmpImage->width * pTmpImage->height * pTmpImage->channel;
					pTmpImage->d = *iDelays;
					pTmpImage->o = hCompareDc;
					if (GetBitmapFromRes(pTmpImage)) {
						pTmpImage->x = (lPRect->right >> 1) - (pTmpImage->width >> 1);
						pTmpImage->y = (lPRect->bottom >> 1) - (pTmpImage->height >> 1);
						sLoadingRect.left = pTmpImage->x;
						sLoadingRect.top = pTmpImage->y;
						sLoadingRect.right = sLoadingRect.left + pTmpImage->width;
						sLoadingRect.bottom = sLoadingRect.top + pTmpImage->height;
						UpdateDeviceContextEx(pTmpImage->o, pTmpImage->hBitmap, &sLoadingRect, 0);
						//第一次刷新区域
						CopyRect(&sLoadingRect, lPRect);
					}
					else {
						stbi_image_free(pTmpImage->data);
						DeleteDC(pTmpImage->o);
						free(pTmpImage);
						pTmpImage = szResGif[0] = NULL;
						break;
					}
				}
				free(pTmpGifEncBuff);
				pTmpGifEncBuff = NULL;
			}
			else {
				//2
				INT iOldIndex = pTmpImage->i;
				InterlockedExchange(&pTmpImage->i, (pTmpImage->i + 1) % pTmpImage->z);
				if (iOldIndex != pTmpImage->i) {
					//避免反复创建句柄，因为同组gif每帧大小都是一样的
					BITMAPINFO bmi;
					ZeroMemory(&bmi, sizeof(BITMAPINFO));
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = pTmpImage->width;
					bmi.bmiHeader.biHeight = -pTmpImage->height;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 32;
					bmi.bmiHeader.biCompression = BI_RGB;
					bmi.bmiHeader.biSizeImage = pTmpImage->width * pTmpImage->height * pTmpImage->channel;
					LPBYTE pUpdateBits = pTmpImage->data + (pTmpImage->i * pTmpImage->width * pTmpImage->height * pTmpImage->channel);
					if (SetDIBits(pTmpImage->o, pTmpImage->hBitmap, 0, pTmpImage->height, pUpdateBits, &bmi, DIB_RGB_COLORS)) {
						sLoadingRect.left = pTmpImage->x;
						sLoadingRect.top = pTmpImage->y;
						sLoadingRect.right = sLoadingRect.left + pTmpImage->width;
						sLoadingRect.bottom = sLoadingRect.top + pTmpImage->height;
						UpdateDeviceContextEx(pTmpImage->o, pTmpImage->hBitmap, &sLoadingRect, 0);
					}
					else {
						LOG_TRACE("SetDIBits Failture");
					}
				}
			}
			TextOut(hCompareDc, (lPRect->right >> 1) - (sMsgSize.cx >> 1), sLoadingRect.bottom, sMsg, lstrlen(sMsg));
		}
		//完成计算
		if (szForm.stop_loading) break;
		InterlockedExchange(&pTmpImage->u, TRUE);
		InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], &sLoadingRect, FALSE);
		if (szForm.stop_loading) break;
		if (pTmpImage->d < 5) {
			WaitForSingleObject(szHandle[HANDLE_LOAD_GIF_EVENT], 100);
		}
		else {
			WaitForSingleObject(szHandle[HANDLE_LOAD_GIF_EVENT], pTmpImage->d);
		}
		if (szForm.stop_loading) break;
	}
	InterlockedExchange(&szForm.stop_loading, TRUE);
	DeleteObject(hBackgroundBrush);
	if (hOldGdiObj) {
		DeleteObject(SelectObject(hCompareDc, hOldGdiObj));
	}
	DeleteDC(hCompareDc);
	CloseHandle(szHandle[HANDLE_LOAD_GIF_EVENT]);
	_endthreadex(0);
	szHandle[HANDLE_LOAD_GIF_EVENT] = szHandle[THREAD_LOAD_GIF_EVENT] = NULL;
	ClearGifMemory();
	return TRUE;
}

BOOL InitOpenGlWin() {
	if (szHandle[OPENGL_WND]) {
		return TRUE;
	}
	else {
		if (RegisterGlCls(OpenGLProc, hInst)) {
			szHandle[OPENGL_WND] = CreateWindowEx(
				WS_EX_ACCEPTFILES,
				TEXT("OpenGL"),
				TEXT("OpenGLWin"),
				WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE,
				sAnalysisRect.left,
				sAnalysisRect.top,
				sAnalysisRect.right,
				sAnalysisRect.bottom,
				szHandle[HANDLE_MAIN_WND],
				NULL,
				hInst,
				NULL
			);
			if (szHandle[OPENGL_WND] != NULL) {
				if (InitGlWindow(szHandle[OPENGL_WND], 32, sAnalysisRect.right, sAnalysisRect.bottom, &szResZip.zip) == TRUE) {
					return TRUE;
				}
				else {
					LOG_TRACE("OpenGL初始化失败 %d", GetLastError());
				}
			}
			else {
				LOG_TRACE("OpenGL窗口初始化失败 %d", GetLastError());
			}
		}
		else {
			LOG_TRACE("OpenGL窗口注册失败 %d", GetLastError());
		}
	}
	return FALSE;
}

//资源回收
BOOL UnPreProcessCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int iSize = 0;
	if (InterlockedExchange(&szForm.stop_loading, TRUE) == FALSE) {
		if (szHandle[HANDLE_LOAD_GIF_EVENT]) {
			if (SetEvent(szHandle[HANDLE_LOAD_GIF_EVENT])) {
				if (szHandle[THREAD_LOAD_GIF_EVENT] != NULL) {
					WaitForSingleObject(szHandle[THREAD_LOAD_GIF_EVENT], 3000);
				}
			}
		}
	}
	if (Audio_StopDec()) {
		InterlockedExchange(&szForm.start, FALSE);
		if (szRecEnc) {
			InterlockedExchange(&szRecEnc->start, FALSE);
		}
		//析构图标资源
		for (int w = 0; w < 7; w++) {
			for (int h = 0; h < 2; h++) {
				PSTB_IMAGE_DATA pId = szRes[w][h];
				if (pId != NULL) {
					if (pId->hBitmap) {
						DeleteObject(pId->hBitmap);
					}
					_aligned_free(pId);
				}
			}
		}
		for (int i = 0; i < 2; i++) {
			if (szResAbout[i]) {
				if (szResAbout[i]->hBitmap) {
					DeleteObject(szResAbout[i]->hBitmap);
				}
				_aligned_free(szResAbout[i]);
				szResAbout[i] = NULL;
			}
		}
		iSize = sizeof(szForm.hFont) / sizeof(HFONT);
		iSize -= 1;
		while (iSize > 0) {
			if (szForm.hFont[iSize]) {
				DeleteObject(szForm.hFont[iSize]);
			}
			iSize--;
		}

		iSize = sizeof(szForm.hPen) / sizeof(HPEN);
		iSize -= 1;
		while (iSize > 0) {
			if (szForm.hPen[iSize]) {
				DeleteObject(szForm.hPen[iSize]);
			}
			iSize--;
		}

		iSize = sizeof(szForm.hBrush) / sizeof(HBRUSH);
		iSize -= 1;
		while (iSize > 0) {
			if (szForm.hBrush[iSize]) {
				DeleteObject(szForm.hBrush[iSize]);
			}
			iSize--;
		}
		//释放资源(可能未被释放)
		if (szResGif[0]) {
			stbi_image_free(szResGif[0]->data);
			szResGif[0]->data = NULL;
			if (szResGif[0]->hBitmap) {
				DeleteObject(szResGif[0]->hBitmap);
				szResGif[0]->hBitmap = NULL;
			}
			free(szResGif[0]);
			szResGif[0] = NULL;
		}
		if (szResTip[0]) {
			stbi_image_free(szResTip[0]->data);
			szResTip[0]->data = NULL;
			if (szResTip[0]->hBitmap) {
				DeleteObject(szResTip[0]->hBitmap);
				szResTip[0]->hBitmap = NULL;
			}
			free(szResTip[0]);
			szResTip[0] = NULL;
		}

		//仅允许运行1个实例
		if (szHandle[MUTEXT_ONLY_INSTANCE]) {
			CloseHandle(szHandle[MUTEXT_ONLY_INSTANCE]);
			szHandle[MUTEXT_ONLY_INSTANCE] = NULL;
		}

		if (szForm.dec_devs[1]) {
			Audio_StopDec();
			szForm.dec_devs[0] = szForm.dec_devs[1] = 0;
		}

		//关闭锁
		if (szHandle[MUTEXT_RECORDER]) {
			CloseHandle(szHandle[MUTEXT_RECORDER]);
			szHandle[MUTEXT_RECORDER] = NULL;
		}

		if (szHandle[HANDLE_TOOLTIP_WND]) {
			DestroyWindow(szHandle[HANDLE_TOOLTIP_WND]);
			szHandle[HANDLE_TOOLTIP_WND] = NULL;
		}

		if (szHandle[THREAD_SHINE] != NULL) {
			WakeConditionVariable(&szRwCond[SHINE_COND_CUS]);
			WaitForSingleObject(szHandle[THREAD_SHINE], INFINITE);
			if (szHandle[ENC_OUT_CREATE_FILE]) {
				CloseHandle(szHandle[ENC_OUT_CREATE_FILE]);
				szHandle[ENC_OUT_CREATE_FILE] = NULL;
			}
			CloseHandle(szHandle[THREAD_SHINE]);
			szHandle[THREAD_SHINE] = NULL;
		}
		//释放虚拟内存
		if (szRing.mem != NULL) {
			VirtualFree(szRing.mem, szRing.length, MEM_DECOMMIT | MEM_RELEASE);
		}
		Destory_Encoder(szRecEnc);
		szRecEnc = NULL;
		if (szHandle[OPENGL_WND]) {
			UnInitGlWindow();
		}
		if (szNotifyMenu) {
			DestroyMenu(szNotifyMenu);
			szNotifyMenu = NULL;
		}
		if (szMemDc) {
			DeleteDC(szMemDc);
			szMemDc = NULL;
		}
		if (szPngMemDc) {
			DeleteDC(szPngMemDc);
			szPngMemDc = NULL;
		}
		if (szCompatibleBitmap) {
			DeleteObject(szCompatibleBitmap);
			szCompatibleBitmap = NULL;
		}
		Shell_NotifyIcon(NIM_DELETE, &szNotify);
		//压缩资源
		if (szResZip.init) {
			mz_zip_reader_end(&szResZip.zip);
			if (szResZip.data != NULL) {
				free(szResZip.data);
			}
		}
		if (szHandle[HANDLE_DEF_FONT] != NULL) {
			RemoveFontMemResourceEx(szHandle[HANDLE_DEF_FONT]);
		}
		if (szLog != NULL) {
			fflush(szLog);
			fclose(szLog);
			szLog = NULL;
		}
		curl_global_cleanup();
	}
	CoUninitialize();
	return TRUE;
}

//清理内存
VOID ClearGifMemory() {
	//释放资源
	if (szResGif[0]) {
		stbi_image_free(szResGif[0]->data);
		if (szResGif[0]->hBitmap) {
			DeleteObject(szResGif[0]->hBitmap);
		}
		free(szResGif[0]);
		szResGif[0] = NULL;
	}
}

//回调（处理音频数据,计算分贝和振幅)
void DecibelWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	static LARGE_INTEGER liPerfStart = { 0 };
	static LARGE_INTEGER liPerfFreq = { 0 };
	static INT iPreTick[2] = { 0,0 };
	static INT16 test_s_val = 0;
	static kiss_fft_cpx kfc = { 0,0 };
	LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;
	if (szForm.dec_devs[1]) {
		InterlockedExchange(&szForm.dec_devs[2], TRUE);
		if (!szForm.start) {
			InterlockedExchange(&szForm.start, TRUE);
			InterlockedExchange(&szForm.stop_loading, TRUE);
			InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], &sDecValRect, FALSE);
		}
		BOOL bContine = (iPreTick[0] == 0) ? TRUE : FALSE;
		if (bContine) {
			QueryPerformanceCounter(&liPerfStart);
			QueryPerformanceFrequency(&liPerfFreq);
			InterlockedExchange(&iPreTick[0], 1);
		}
		else {
			LARGE_INTEGER liPerfNow = { 0 };
			QueryPerformanceCounter(&liPerfNow);
			int iTmp = (((liPerfNow.QuadPart - liPerfStart.QuadPart) * 1000) / liPerfFreq.QuadPart);
			if (MIN(iTmp - iPreTick[0], 100) == 100) {
				//0.1刷新一次分贝数据
				InterlockedExchange(&iPreTick[0], iTmp);
				bContine = TRUE;
			}
			if (MIN(iTmp - iPreTick[1], 500) == 500) {
				//刷新区域(0.5秒刷新一次波形图)
				if (TryAcquireSRWLockExclusive(&szRwLock[DEC_RW_LOCK])) {
					RECT sTmpRect = { 0 };
					CopyRect(&sTmpRect, &sDecValRect);
					if (szForm.about) {
						sTmpRect.right = WS_ABOUT_CONTENT_L;
						InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], &sTmpRect, FALSE);
					}
					else {
						//操作按钮排除(不要刷新)
						if (!szForm.first_run) {
							sTmpRect.top += 50;
						}
						sTmpRect.right = sDecValRect.right + 1;
						sTmpRect.left = sDecValRect.left - 1;
						sTmpRect.bottom = sDecValRect.bottom + 1;
						InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], &sTmpRect, FALSE);
					}
					if (!SleepConditionVariableSRW(&szRwCond[DEC_COND_PRO], &szRwLock[DEC_RW_LOCK], 100, 0)) {
						bContine = FALSE;
					}
					InterlockedExchange(&iPreTick[1], iTmp);
					ReleaseSRWLockExclusive(&szRwLock[DEC_RW_LOCK]);
				}
			}
		}

		if (bContine) {
			const signed short* pInput = pwh->lpData;
			int shortCount = pwh->dwBytesRecorded >> 1, sumVal = 0, iSize = 0;
			INT16 shortVal = 0;
			while (shortCount > 0) {
				shortVal = *pInput;
				if (szHandle[OPENGL_WND] && TryAcquireSRWLockExclusive(&szRwLock[OPENGL_RW_LOCK])) {
					InvalidateRectOnce(szHandle[OPENGL_WND], NULL, FALSE);
					if (PaUtil_GetRingBufferWriteAvailable(szRing.ring[RING_IND_OPENGL_EFFECT])) {
						kfc.r = shortVal;
						kfc.i = 0.0f;
						PaUtil_WriteRingBuffer(szRing.ring[RING_IND_OPENGL_EFFECT], &kfc, 1);
					}
					ReleaseSRWLockExclusive(&szRwLock[OPENGL_RW_LOCK]);
				}
				sumVal += abs(shortVal);
				pInput++;
				shortCount--;
			}
			//计算分贝
			if (sumVal > 0) {
				shortVal = sumVal / (pwh->dwBytesRecorded >> 1);
				if (shortVal > 0) {
					InterlockedExchange(&szForm.current_db, 20 * log10(shortVal));
				}
			}
			if (TryAcquireSRWLockExclusive(&szRwLock[DEC_RW_LOCK])) {
				if (PaUtil_GetRingBufferWriteAvailable(szRing.ring[RING_IND_DEC_EFFECT])) {
					PaUtil_WriteRingBuffer(szRing.ring[RING_IND_DEC_EFFECT], &szForm.current_db, 1);
				}
				ReleaseSRWLockExclusive(&szRwLock[DEC_RW_LOCK]);
			}
		}

		//编码mp3
		if (szRecEnc != NULL) {
			if (szRecEnc->start) {
				//调用解码数据
				if (WaitForSingleObject(szHandle[MUTEXT_RECORDER], 100) == WAIT_OBJECT_0) {
					if (TryAcquireSRWLockExclusive(&szRwLock[SHINE_RW_LOCK])) {
						//编码mp3
						if (PaUtil_GetRingBufferWriteAvailable(szRing.ring[RING_IND_ENC_OUT]) < pwh->dwBytesRecorded) {
							WakeConditionVariable(&szRwCond[SHINE_COND_CUS]);
							if (SleepConditionVariableSRW(&szRwCond[SHINE_COND_PRO], &szRwLock[SHINE_RW_LOCK], 100, 0)) {
								PaUtil_WriteRingBuffer(szRing.ring[RING_IND_ENC_OUT], pwh->lpData, pwh->dwBytesRecorded);
							}
						}
						else {
							PaUtil_WriteRingBuffer(szRing.ring[RING_IND_ENC_OUT], pwh->lpData, pwh->dwBytesRecorded);
						}
						ReleaseSRWLockExclusive(&szRwLock[SHINE_RW_LOCK]);
					}
					ReleaseMutex(szHandle[MUTEXT_RECORDER]);
				}
			}
		}
	}
	else {
		szForm.dec_devs[2] = 0;
	}
}

//居中窗口
VOID CenterWindow(HWND hWnd) {
	HWND hParentOrOwner;
	RECT rc, rc2;
	int x, y;
	if ((hParentOrOwner = GetParent(hWnd)) == NULL) {
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
	}
	else {
		GetClientRect(hParentOrOwner, &rc);
	}
	GetWindowRect(hWnd, &rc2);
	x = ((rc.right - rc.left) - (rc2.right - rc2.left)) / 2 + rc.left;
	y = ((rc.bottom - rc.top) - (rc2.bottom - rc2.top)) / 2 + rc.top;
	SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}

BOOL	RenderBitmap(HDC hdc, HDC hMemdc, HBITMAP hBitmap, RECT * pRect) {
	BOOL result = FALSE;
	HGDIOBJ hOldGdiObj = SelectObject(hMemdc, hBitmap);
	result = BitBlt(
		hdc,
		pRect->left,
		pRect->top,
		pRect->right - pRect->left,
		pRect->bottom - pRect->top,
		hMemdc,
		0,
		0,
		SRCCOPY
	);
	if (hOldGdiObj) SelectObject(hMemdc, hOldGdiObj);
	return result;
}

BOOL RenderAlphaBitmap(HDC hdc, HDC hMemdc, HBITMAP hBitmap, RECT * pRect, DWORD dBackground) {
	BOOL result = FALSE;
	HGDIOBJ hOldGdiObj = SelectObject(hMemdc, hBitmap);
	BLENDFUNCTION ftn;
	HBRUSH hBackgroundBrush = NULL;
	ftn.BlendOp = AC_SRC_OVER;
	ftn.AlphaFormat = AC_SRC_ALPHA;
	ftn.BlendFlags = 0;
	ftn.SourceConstantAlpha = 0xFF;
	if (dBackground != 0) {
		hBackgroundBrush = CreateSolidBrush(dBackground);
		FillRect(hdc, pRect, hBackgroundBrush);
	}
	result = AlphaBitBlt(
		hdc,
		pRect->left,
		pRect->top,
		pRect->right - pRect->left,
		pRect->bottom - pRect->top,
		hMemdc,
		0,
		0,
		pRect->right - pRect->left,
		pRect->bottom - pRect->top,
		ftn
	);
	if (hOldGdiObj) SelectObject(hMemdc, hOldGdiObj);
	if (hBackgroundBrush != NULL) DeleteObject(hBackgroundBrush);
	return result;
}

BOOL RenderDecErr(HDC hdc, HDC hMemdc, const LPRECT lPRect) {
	PSTB_IMAGE_DATA pTmpImage = szResGif[0];
	BOOL ret = FALSE;
	if (pTmpImage) {
		if (InterlockedExchange(&pTmpImage->u, FALSE) == TRUE) {
			ret = BitBlt(hMemdc, lPRect->left, lPRect->top, lPRect->right, lPRect->bottom, pTmpImage->o, lPRect->left, lPRect->top, SRCCOPY);
		}
	}
	return ret;
}

HRESULT UpdateDeviceContextEx(HDC hdc, HBITMAP hBitmap, LPRECT lPRect) {
	HRESULT res = S_FALSE;
	PICTDESC pd;
	pd.cbSizeofstruct = sizeof(PICTDESC);
	pd.picType = PICTYPE_BITMAP;
	pd.bmp.hbitmap = hBitmap;
	IPicture* pNewPic = NULL;
	OLE_XSIZE_HIMETRIC hWidth = 0, hHeight = 0;
	if (SUCCEEDED(res = OleCreatePictureIndirect(&pd, (const IID * const)& IID_IPicture, FALSE, (LPVOID*)& pNewPic))) {
		pNewPic->lpVtbl->get_Width(pNewPic, &hWidth);
		pNewPic->lpVtbl->get_Height(pNewPic, &hHeight);
		if (FAILED(res = pNewPic->lpVtbl->Render(pNewPic, hdc, lPRect->left, lPRect->top, (lPRect->right - lPRect->left), (lPRect->bottom - lPRect->top), 0, hHeight, hWidth, -hHeight, NULL))) {
			LOG_TRACE("IPicture渲染:%d (E_POINTER:%d CTL_E_INVALIDPROPERTYVALUE:%d)", res, E_POINTER, CTL_E_INVALIDPROPERTYVALUE);
		}
		pNewPic->lpVtbl->Release(pNewPic);
		pNewPic = NULL;
	}
	return res;
}

//关于
BOOL RenderAbout(HDC hdc, HDC hMemdc, const LPRECT lPRect) {
	INT iSize = 0;
	HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(48, 45, 45));
	FillRect(hMemdc, lPRect, hBackgroundBrush);
	CHAR* tarRes[2] = { "res/me.png" ,"res/aliyun_adv.png" };
	LPVOID resBuff = NULL;
	for (int i = 0; i < 2; i++) {
		if (!szResAbout[i]) {
			if ((iSize = LoadResourceFromZip(&szResZip.zip, tarRes[i], &resBuff))) {
				szResAbout[i] = (PSTB_IMAGE_DATA)_aligned_malloc(sizeof(STB_IMAGE_DATA), szAlignBlock);
				memset(szResAbout[i], 0, sizeof(STB_IMAGE_DATA));
				szResAbout[i]->channel = 4;
				szResAbout[i]->data = NULL;
				szResAbout[i]->len = iSize;
				szResAbout[i]->z = 0;
				szResAbout[i]->d = 0;
				szResAbout[i]->i = 0;
				szResAbout[i]->u = TRUE;
				//stb解码图片资源
				szResAbout[i]->data = stbi_load_from_memory(
					(stbi_uc*)resBuff,
					iSize,
					&szResAbout[i]->width,
					&szResAbout[i]->height,
					&szResAbout[i]->comp,
					szResAbout[i]->channel
				);
				if (szResAbout[i]->data != NULL) {
					if (GetBitmapFromRes(szResAbout[i]) == TRUE) {
						stbi_image_free(szResAbout[i]->data);
					}
				}
				free(resBuff);
				resBuff = NULL;
			}
		}
	}
	if (!szAboutBody) {
		//加载帮助文本（加载资源文件）
		if (!(iSize = LoadResourceFromZip(&szResZip.zip, "res/about.txt", &szAboutBody))) {
			LOG_TRACE("获取about.txt失败");
		}
	}
	PSTB_IMAGE_DATA sTmpPid = szResAbout[0], sTmpAdv = szResAbout[1];
	RECT sTmpRect = { lPRect->left + (((lPRect->right - lPRect->left) >> 1) - (sTmpPid->width >> 1)),lPRect->top + 10,0,0 };
	RECT sTmpAdvRect = { lPRect->left + (((lPRect->right - lPRect->left) >> 1) - (sTmpAdv->width >> 1)),lPRect->top + 15 + sTmpPid->height,0,0 };
	sTmpRect.right = sTmpRect.left + sTmpPid->width;
	sTmpRect.bottom = sTmpRect.top + sTmpPid->height;
	sTmpAdvRect.right = sTmpAdvRect.left + sTmpAdv->width;
	sTmpAdvRect.bottom = sTmpAdvRect.top + sTmpAdv->height;
	if (szAboutBody) {
		SetTextColor(hMemdc, RGB(0xFF, 0xFF, 0xFF));
		WCHAR wAboutBody[1024];
		INT iTextSize = Char2WChar(szAboutBody, &wAboutBody);
		if (iTextSize) {
			WCHAR* next_token = NULL;
			WCHAR* token = NULL;
			SIZE  bodySize = { 0 };
			INT   iCount = 0;
			token = wcstok_s(wAboutBody, TEXT("\n"), &next_token);
			while (token != NULL) {
				iTextSize = lstrlen(token);
				if (GetTextExtentPoint32(hMemdc, token, iTextSize, &bodySize) == TRUE) {
					TextOut(hMemdc, sTmpAdvRect.left, (sTmpAdvRect.bottom + (bodySize.cy >> 1)) + (iCount * bodySize.cy), token, iTextSize);
				}
				token = wcstok_s(NULL, TEXT("\n"), &next_token);
				iCount++;
			}
		}
	}
	UpdateDeviceContextEx(hMemdc, sTmpPid->hBitmap, &sTmpRect);
	UpdateDeviceContextEx(hMemdc, sTmpAdv->hBitmap, &sTmpAdvRect);
	DeleteObject(hBackgroundBrush);
	return TRUE;
}

//渲染波形数据
BOOL RenderDecFrame(HDC hdc, HDC hMemdc, const LPRECT lPRect) {
	BOOL result = FALSE;
	INT iSize = 0, iDc = 0;
	HGDIOBJ hOldPen = NULL;
	HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
	if (szForm.about) {
		RECT sTmpRect = { lPRect->left,lPRect->top,WS_ABOUT_CONTENT_L,lPRect->bottom };
		FillRect(hMemdc, &sTmpRect, hBackgroundBrush);
	}
	else {
		FillRect(hMemdc, lPRect, hBackgroundBrush);
	}
	//h(纵向 偏移5px)
	POINT oldPoint = { 0,0 }, decPoint = { 0,0 }, areaPoint[20];;
	MoveToEx(hMemdc, lPRect->left + 5, lPRect->top, &oldPoint);
	INT hPrecentVal = WS_DEC_HEIGHT / 100, vPrecentVal = 0;
	//v（横向）
	vPrecentVal = (int)(sClientRect.right / 20);
	if (szForm.dec_devs[1] && szRing.ring[RING_IND_DEC_EFFECT]) {
		if (TryAcquireSRWLockShared(&szRwLock[DEC_RW_LOCK])) {
			areaPoint[0].x = lPRect->left + vPrecentVal;
			areaPoint[0].y = lPRect->bottom - 6;
			int iDecVal, iIndex = szRing.ring[RING_IND_DEC_EFFECT]->readIndex, iRead = 0, iCount = 0;
			iDc = SaveDC(hMemdc);
			HGDIOBJ hOldTmp = NULL;
			if (szForm.dec_type == 2) {
				hOldTmp = SelectObject(hMemdc, szForm.hPen[1]);
			}
			iSize = PaUtil_GetRingBufferReadAvailable(szRing.ring[RING_IND_DEC_EFFECT]);
			iRead = MIN(iSize, 20);
			if (iSize > 20) {
				PaUtil_AdvanceRingBufferReadIndex(szRing.ring[RING_IND_DEC_EFFECT], iSize - 20);
				iIndex = szRing.ring[RING_IND_DEC_EFFECT]->readIndex;
				iSize -= (iSize - 20);
			}
			while (iRead > 0) {
				if (PaUtil_ReadRingBuffer(szRing.ring[RING_IND_DEC_EFFECT], &iDecVal, 1)) {
					iDecVal *= hPrecentVal;
					decPoint.x = (iCount + 1) * vPrecentVal;
					decPoint.y = lPRect->bottom - iDecVal;
					//绘图模式（线条）
					if (szForm.dec_type == 1) {
						hOldTmp = SelectObject(hMemdc, szForm.hPen[0]);
						if (!iCount) {
							MoveToEx(hMemdc, decPoint.x, decPoint.y, NULL);
						}
						else {
							LineTo(hMemdc, decPoint.x, decPoint.y);
							MoveToEx(hMemdc, decPoint.x, decPoint.y, NULL);
						}
					}
					else {
						areaPoint[iCount + 1].x = decPoint.x;
						areaPoint[iCount + 1].y = decPoint.y;
					}
					iCount++;
				}
				iRead--;
			}
			if (iCount && PaUtil_GetRingBufferReadAvailable(szRing.ring[RING_IND_DEC_EFFECT]) < 30) {
				szRing.ring[RING_IND_DEC_EFFECT]->readIndex = iIndex - 1;
				PaUtil_AdvanceRingBufferReadIndex(szRing.ring[RING_IND_DEC_EFFECT], 1);
			}
			if (hOldTmp) {
				SelectObject(hMemdc, hOldTmp);
			}
			//多边形
			if (iCount && szForm.dec_type == 2) {
				hOldTmp = SelectObject(hMemdc, szForm.hBrush[0]);
				areaPoint[iCount + 1].x = lPRect->right;
				areaPoint[iCount + 1].y = lPRect->bottom - 6;
				Polygon(hMemdc, areaPoint, iCount + 2);
				SelectObject(hMemdc, hOldTmp);
			}
			RestoreDC(hMemdc, iDc);
			ReleaseSRWLockShared(&szRwLock[DEC_RW_LOCK]);
			WakeConditionVariable(&szRwCond[DEC_COND_PRO]);
		}
	}

	//竖
	INT hvCalc = 0, hvVal = 90;
	WCHAR hvLab[16] = { 0 };
	for (int i = 1; i <= 100; i++) {
		hvCalc = i * hPrecentVal + lPRect->top;
		MoveToEx(hMemdc, lPRect->left, hvCalc, NULL);
		if ((i % 10) == 0 && i) {
			swprintf_s(hvLab, sizeof(hvLab), TEXT("%d"), hvVal);
			LineTo(hMemdc, lPRect->left + 3, hvCalc);
			if (i < 100) {
				if (szForm.dec_type == 1) {
					hOldPen = SelectObject(hMemdc, szForm.hPen[2]);
					MoveToEx(hMemdc, lPRect->left + 24, hvCalc, NULL);
					LineTo(hMemdc, lPRect->right, hvCalc);
					SelectObject(hMemdc, hOldPen);
				}
			}
			if (hvVal) {
				SetTextColor(hMemdc, (hvVal < 70) ? (hvVal <= 30) ? RGB(0x58, 0xAF, 0x10) : RGB(0, 0, 0) : RGB(0xFF, 0x00, 0x00));
				szForm.hFont[3] = SelectObject(hMemdc, szForm.hFont[2]);
				TextOut(hMemdc, lPRect->left + 5, hvCalc - 8, hvLab, lstrlen(hvLab));
				SelectObject(hMemdc, szForm.hFont[3]);
			}
			hvVal -= 10;
		}
		else {
			LineTo(hMemdc, lPRect->left + 1, hvCalc);
		}
	}

	iDc = SaveDC(hMemdc);
	SetTextColor(hMemdc, (szForm.dec_type == 1) ? RGB(0, 0, 0) : RGB(0xFF, 0xFF, 0xFF));
	for (int i = 1; i <= 20; i++) {
		MoveToEx(hMemdc, lPRect->left + i * vPrecentVal, lPRect->bottom, NULL);
		if (i == 10) {
			LineTo(hMemdc, lPRect->left + i * vPrecentVal, lPRect->bottom - 6);
			TextOut(hMemdc, lPRect->left + i * vPrecentVal - 12, lPRect->bottom - 24, TEXT("1秒"), 4);
		}
		else {
			LineTo(hMemdc, lPRect->left + i * vPrecentVal, lPRect->bottom - 4);
		}
	}

	WCHAR hvValData[32] = { 0 };
	swprintf_s(hvValData, sizeof(hvValData), TEXT("%.2d/分贝"), szForm.current_db);
	//lab(输出分贝值)
	TextOut(hMemdc, lPRect->right - 65, lPRect->bottom - 24, hvValData, lstrlen(hvValData));
	RestoreDC(hMemdc, iDc);
	//还原原坐标
	MoveToEx(hMemdc, oldPoint.x, oldPoint.y, NULL);

	//填充一个渐变条
	TRIVERTEX vert[2];
	vert[0].x = sDecValRect.right - 1;
	vert[0].y = sDecValRect.top;
	vert[0].Red = 0xFF00;
	vert[0].Green = 0x0000;
	vert[0].Blue = 0x0000;
	vert[0].Alpha = 0x0000;

	vert[1].x = sDecValRect.right + 1;
	vert[1].y = sDecValRect.bottom;
	vert[1].Red = 0x4900;
	vert[1].Green = 0xDC00;
	vert[1].Blue = 0x8700;
	vert[1].Alpha = 0x0000;
	GRADIENT_RECT gRect = { 0,1 };
	GradientFill(hMemdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
	DeleteObject(hBackgroundBrush);
	InterlockedExchange(&szForm.dec_refresh, FALSE);
	//信息更新
	if (szMsgTip.msg != NULL && szMsgTip.display) {
		if ((szMsgTip.delay == 0 && szMsgTip.start) || (GetTickCount() - szMsgTip.start) <= szMsgTip.delay) {
			//绘制msg
			if (!szMsgTip.size.cx || !szMsgTip.size.cy) {
				GetTextExtentPoint32(hMemdc, szMsgTip.msg, lstrlen(szMsgTip.msg), &szMsgTip.size);
			}
			if (szMsgTip.size.cx && szMsgTip.size.cy) {
				iDc = SaveDC(hMemdc);
				HBRUSH hMsgTipBrush = CreateSolidBrush(szMsgTip.background);
				RECT msgRect = { sDecValRect.left - 1,sDecValRect.bottom - hPrecentVal * 20,sDecValRect.right + 1,sDecValRect.bottom + 1 };
				FillRect(hMemdc, &msgRect, hMsgTipBrush);
				SetTextColor(hMemdc, szMsgTip.color);
				TextOut(hMemdc, (sDecValRect.right >> 1) - (szMsgTip.size.cx >> 1), msgRect.top + (hPrecentVal * 10) - (szMsgTip.size.cy >> 1), szMsgTip.msg, lstrlen(szMsgTip.msg));
				DeleteObject(hMsgTipBrush);
				RestoreDC(hMemdc, iDc);
			}
		}
		else {
			ZeroMemory(&szMsgTip, sizeof(MSG_TIP));
		}
	}
	//教学信息
	if (szForm.first_run) {
		if (szResTip[0]) {
			if ((szResTip[0]->d == 0 && szResTip[0]->i) || (GetTickCount() - szResTip[0]->i) <= szResTip[0]->d) {
				UpdateDeviceContextEx(hMemdc, szResTip[0]->hBitmap, &sClientRect);
			}
			else {
				szForm.first_run = FALSE;
				stbi_image_free(szResTip[0]->data);
				szResTip[0]->data = NULL;
				if (szResTip[0]->hBitmap) {
					DeleteObject(szResTip[0]->hBitmap);
					szResTip[0]->hBitmap = NULL;
				}
				free(szResTip[0]);
				szResTip[0] = NULL;
			}
		}
	}
	return result;
}

LRESULT CALLBACK OpenGLProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_ERASEBKGND:
		return TRUE;
	case WM_MOUSEMOVE:
		if (szOpenglEnv) {
			szOpenglEnv->point.x = LOWORD(lParam);
			szOpenglEnv->point.y = HIWORD(lParam);
			if (!PtInRect(&sAnalysisRect, szOpenglEnv->point)) {
				szOpenglEnv->point.x = 0;
				szOpenglEnv->point.y = 0;
			}
		}
		break;
	case WM_PAINT:
		if (szOpenglEnv != NULL) {
			RenderGl(szOpenglEnv);
		}
		//获取WM_PAINT消息即可
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DROPFILES:
		HDROP hDrop = (HDROP)wParam;
		WCHAR wDropFileName[MAX_LOADSTRING] = { 0 }, wDropFileExt[MAX_LOADSTRING] = { 0 };
		CHAR cDropFileName[MAX_LOADSTRING] = { 0 };
		DWORD dwAttribute;
		if (DragQueryFile(hDrop, 0, wDropFileName, sizeof(wDropFileName))) {
			dwAttribute = GetFileAttributes(wDropFileName);
			if (!(dwAttribute & FILE_ATTRIBUTE_DIRECTORY)) {
				//验证文件名后缀
				_wsplitpath_s(wDropFileName, NULL, 0, NULL, 0, NULL, 0, wDropFileExt, MAX_LOADSTRING);
				WCHAR* wUperName = CharUpperW(wDropFileExt);
				if (lstrcmp(wUperName, TEXT(".PNG")) == 0 || lstrcmp(wUperName, TEXT(".JPG")) == 0 || lstrcmp(wUperName, TEXT(".JPEG")) == 0) {
					WChar2Char(wDropFileName, cDropFileName);
					GlUpdateBackgroundFromFile(cDropFileName);
				}
				else {
					WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0x00, 0x00), TEXT("文件类型不合法，仅允许 .png .jpg .jpeg"), FALSE);
				}
			}
			else {
				WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0x00, 0x00), TEXT("文件类型不合法，仅允许 .png .jpg .jpeg"), FALSE);
			}
		}
		DragFinish(hDrop);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

/** 截图*/
BOOL	CaptureForm(HWND hWnd, LPBYTE pOut, INT * iSize, INT * pWidth, INT * pHeight, BOOL bClip) {
	BOOL ret = FALSE;
	HDC hdcWindow = NULL;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	INT iOpenGlBytes = ALIGN_OF(sAnalysisRect.right * sAnalysisRect.bottom << 2, 4);
	hdcWindow = GetDC(hWnd);
	RECT targetClientRect = { sClientRect.left,sClientRect.top,sClientRect.right,sClientRect.bottom };
	hdcMemDC = CreateCompatibleDC(hdcWindow);
	if (!hdcMemDC) {
		goto done;
	}
	SetStretchBltMode(hdcWindow, HALFTONE);
	hbmScreen = CreateCompatibleBitmap(hdcWindow, targetClientRect.right - targetClientRect.left, targetClientRect.bottom - targetClientRect.top);
	if (!hbmScreen) {
		goto done;
	}
	SelectObject(hdcMemDC, hbmScreen);
	if (!BitBlt(hdcMemDC, 0, 0, targetClientRect.right - targetClientRect.left, targetClientRect.bottom - targetClientRect.top, hdcWindow, 0, 0, SRCCOPY)) {
		goto done;
	}
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);
	BITMAPINFOHEADER   bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = -bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	bi.biSizeImage = bmpScreen.bmWidth * bmpScreen.bmHeight << 2;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize << 1);
	UINT8 * lpbitmap = (UINT8*)GlobalLock(hDIB);
	GetDIBits(hdcWindow, hbmScreen, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO*)& bi, DIB_RGB_COLORS);
	BGRA2RGBA(lpbitmap, lpbitmap + dwBmpSize, bmpScreen.bmWidth, bmpScreen.bmHeight, FALSE);
	if (szHandle[OPENGL_WND]) {
		//从显卡中获取像素数据(并替换先前处理的内存数据RGBA)
		GlReadPixels(0, 0, sAnalysisRect.right, sAnalysisRect.bottom, GL_RGBA, GL_UNSIGNED_BYTE, lpbitmap + dwBmpSize + (sMenuBarRect.bottom * sMenuBarRect.right << 2));
	}
	ret = TRUE;
	CopyMemory(pOut, lpbitmap + dwBmpSize, dwBmpSize);
	*pWidth = bmpScreen.bmWidth;
	*pHeight = bmpScreen.bmHeight;
	*iSize = dwBmpSize;
	//写入到剪贴板
	if (bClip) {
		if (OpenClipboard(hWnd)) {
			//转换RGBA2BGRA(BGRA2RGBA可逆函数)
			LPVOID pTmpBuffer = malloc(dwBmpSize);
			if (pTmpBuffer != NULL) {
				BGRA2RGBA(pOut, pTmpBuffer, bmpScreen.bmWidth, bmpScreen.bmHeight, FALSE);
				if (SetDIBits(hdcMemDC, hbmScreen, 0, bmpScreen.bmHeight, pTmpBuffer, &bi, DIB_RGB_COLORS)) {
					if (EmptyClipboard()) {
						SetClipboardData(CF_BITMAP, hbmScreen);
					}
				}
				free(pTmpBuffer);
				pTmpBuffer = NULL;
			}
			CloseClipboard();
		}
	}
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
done:
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(hWnd, hdcWindow);
	return ret;
}

/**
截屏
*/
BOOL CaptureAndBuildPng(HWND hWnd, CONST WCHAR * pName) {
	INT iWidth = 0, iHeight = 0, iSize = 0;
	BYTE* pTmpBuffer = malloc(((WS_WIDTH * 32 + 31) / 32) * 4 * WS_HEIGHT);
	if (CaptureForm(hWnd, pTmpBuffer, &iSize, &iWidth, &iHeight, TRUE)) {
		stbi_write_png(pName, iWidth, iHeight, 4, pTmpBuffer, iWidth << 2);
		free(pTmpBuffer);
		return TRUE;
	}
	free(pTmpBuffer);
	return FALSE;
}

BOOL	InvalidateRectOnce(HWND hWnd, CONST RECT* lpRect, BOOL bErase) {
	if (szAllowValidate) {
		return InvalidateRect(hWnd, lpRect, bErase);
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static POINT point = { 0,0 };
	static HRGN hAbtRgn = NULL;
	static COLORREF cHoverColor = RGB(63, 63, 65);
	INT iBytes = 0, iDc = 0;
	PSTB_IMAGE_DATA sTmpPid = NULL;
	switch (message) {
	case WM_CREATE:
		PreProcessCreate(hWnd, message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		return FALSE;
	case WM_PAINT:
		PAINTSTRUCT ps;
		POINT oldPoint = { 0 };
		InterlockedExchange(&szAllowValidate, FALSE);
		HDC hdc = BeginPaint(hWnd, &ps);
		SetBkMode(hdc, TRANSPARENT);
		//双缓冲
		if (!szMemDc) {
			szMemDc = CreateCompatibleDC(hdc);
			szCompatibleBitmap = CreateCompatibleBitmap(hdc, sClientRect.right, sClientRect.bottom);
			SelectObject(szMemDc, szCompatibleBitmap);
			SetBkMode(szMemDc, TRANSPARENT);
		}
		if (!szPngMemDc) {
			szPngMemDc = CreateCompatibleDC(hdc);
			SetBkMode(szPngMemDc, TRANSPARENT);
		}
		//绘制工具栏
		Rectangle(szMemDc, sClientRect.left, sClientRect.top, sClientRect.right, sClientRect.bottom);
		FillRect(szMemDc, &sMenuBarRect, szForm.hBrush[1]);
		COLORREF oldColor = SetTextColor(szMemDc, RGB(0xFF, 0xFF, 0xFF));
		szForm.hFont[1] = SelectObject(szMemDc, szForm.hFont[0]);
		DrawIcon(szMemDc, 3, 6, szIcon[ICON_IDB_DEC_SMALL]);
		TextOut(szMemDc, 32, 12, szTitle, lstrlen(szTitle));
		MoveToEx(szMemDc, 0, 40, &oldPoint);
		LineTo(szMemDc, sClientRect.right, 40);
		MoveToEx(szMemDc, oldPoint.x, oldPoint.y, NULL);

		//绘制按钮
		sTmpPid = (szForm.close_hover == TRUE) ? szRes[0][1] : szRes[0][0];
		if (sTmpPid->hBitmap) {
			RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sCloseRect, (szForm.close_hover == TRUE) ? cHoverColor : 0);
		}
		sTmpPid = (szForm.dec_resize_hover == TRUE) ? szRes[1][1] : szRes[1][0];
		if (sTmpPid->hBitmap) {
			RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sResizeRect, (szForm.dec_resize_hover == TRUE) ? cHoverColor : 0);
		}
		sTmpPid = (szForm.dec_min_hover == TRUE) ? szRes[2][1] : szRes[2][0];
		if (sTmpPid->hBitmap) {
			RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sMinRect, (szForm.dec_min_hover == TRUE) ? cHoverColor : 0);
		}
		sTmpPid = (szForm.dec_about_hover == TRUE) ? szRes[3][1] : szRes[3][0];
		if (sTmpPid->hBitmap) {
			RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sAboutRect, (szForm.dec_about_hover == TRUE) ? cHoverColor : 0);
		}
		if (szNeedUpdate) {
			sTmpPid = (szForm.dec_update_hover == TRUE) ? szRes[6][1] : szRes[6][0];
			if (sTmpPid->hBitmap) {
				RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sUpdateRect, (szForm.dec_update_hover == TRUE) ? cHoverColor : 0);
			}
		}
		
		if (szForm.dec_devs[1]) {
			if (szForm.dec_devs[2]) {
				//数据准备
				if (szForm.about) {
					//设置绘画区域
					if (!hAbtRgn) {
						hAbtRgn = CreateRectRgn(sDecValRect.left, sDecValRect.top, sAboutContentRect.left, sDecValRect.bottom);
					}
					SelectClipRgn(szMemDc, hAbtRgn);
					RenderDecFrame(hdc, szMemDc, &sDecValRect);
					SelectClipRgn(szMemDc, NULL);
					RenderAbout(hdc, szMemDc, &sAboutContentRect);
				}
				else {
					RenderDecFrame(hdc, szMemDc, &sDecValRect);
					sTmpPid = (szForm.dec_record_hover == TRUE) ? szRes[4][1] : szRes[4][0];
					if (sTmpPid->hBitmap) {
						RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sRecordRect, 0);
					}
					sTmpPid = (szForm.dec_cut_hover == TRUE) ? szRes[5][1] : szRes[5][0];
					if (sTmpPid->hBitmap) {
						RenderAlphaBitmap(szMemDc, szPngMemDc, sTmpPid->hBitmap, &sCutRect, 0);
					}
				}
			}
			else {
				RenderDecErr(hdc, szMemDc, &sContentRect);
			}
		}
		else {
			RenderDecErr(hdc, szMemDc, &sContentRect);
		}
		//1次性复制
		BitBlt(hdc, 0, 0, sClientRect.right, sClientRect.bottom, szMemDc, 0, 0, SRCCOPY);
		SetTextColor(szMemDc, oldColor);
		if (szForm.hFont[1]) {
			SelectObject(szMemDc, szForm.hFont[1]);
		}
		EndPaint(hWnd, &ps);
		InterlockedExchange(&szAllowValidate, TRUE);
		break;
	case WM_DESTROY:
		if (hAbtRgn) {
			DeleteObject(hAbtRgn);
			hAbtRgn = NULL;
		}
		if (UnPreProcessCreate(hWnd, message, wParam, lParam)) {
			PostQuitMessage(0);
		}
		break;
	case WM_MOUSEMOVE:
		szMouse.x = LOWORD(lParam);
		szMouse.y = HIWORD(lParam);
		point.x = szMouse.x;
		point.y = szMouse.y;
		if (szMouse.l_button_down == TRUE) {
			//移动窗口
			SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
		//关闭
		if (PtInRect(&sCloseRect, point) == TRUE) {
			if (szForm.close_hover == FALSE) {
				szForm.close_hover = TRUE;
				UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sCloseRect, TEXT("关闭"));
				InvalidateRectOnce(hWnd, &sMenuBarRect, FALSE);
			}
		}
		else {
			if (szForm.close_hover == TRUE) {
				szForm.close_hover = FALSE;
				InvalidateRectOnce(hWnd, &sMenuBarRect, FALSE);
			}
		}
		//最小化
		if (PtInRect(&sMinRect, point) == TRUE) {
			if (szForm.dec_min_hover == FALSE) {
				szForm.dec_min_hover = TRUE;
				UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sMinRect, TEXT("最小化"));
				InvalidateRectOnce(hWnd, &sMinRect, FALSE);
			}
		}
		else {
			if (szForm.dec_min_hover == TRUE) {
				szForm.dec_min_hover = FALSE;
				InvalidateRectOnce(hWnd, &sMinRect, FALSE);
			}
		}
		//调整
		if (PtInRect(&sResizeRect, point) == TRUE) {
			if (szForm.dec_resize_hover == FALSE) {
				szForm.dec_resize_hover = TRUE;
				UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sResizeRect, TEXT("调整"));
				InvalidateRectOnce(hWnd, &sResizeRect, FALSE);
			}
		}
		else {
			if (szForm.dec_resize_hover == TRUE) {
				szForm.dec_resize_hover = FALSE;
				InvalidateRectOnce(hWnd, &sResizeRect, FALSE);
			}
		}
		//内容
		if (PtInRect(&sDecValRect, point) == TRUE) {
			if (szForm.dec_hover == FALSE) {
				szForm.dec_hover = TRUE;
			}
		}
		else {
			if (szForm.dec_hover == TRUE) {
				szForm.dec_hover = FALSE;
			}
		}
		//截图
		if (PtInRect(&sCutRect, point) == TRUE) {
			if (szForm.dec_cut_hover == FALSE) {
				szForm.dec_cut_hover = TRUE;
				UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sCutRect, TEXT("截图"));
				InvalidateRectOnce(hWnd, &sCutRect, FALSE);
			}
		}
		else {
			if (szForm.dec_cut_hover == TRUE) {
				szForm.dec_cut_hover = FALSE;
				InvalidateRectOnce(hWnd, &sCutRect, FALSE);
			}
		}
		//关于
		if (PtInRect(&sAboutRect, point) == TRUE) {
			if (szForm.dec_about_hover == FALSE) {
				szForm.dec_about_hover = TRUE;
				UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sAboutRect, TEXT("关于我"));
				InvalidateRectOnce(hWnd, &sAboutRect, FALSE);
			}
		}
		else {
			if (szForm.dec_about_hover == TRUE) {
				szForm.dec_about_hover = FALSE;
				InvalidateRectOnce(hWnd, &sAboutRect, FALSE);
			}
		}
		if (szNeedUpdate) {
			if (PtInRect(&sUpdateRect, point) == TRUE) {
				if (szForm.dec_update_hover == FALSE) {
					szForm.dec_update_hover = TRUE;
					UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sUpdateRect, TEXT("更新"));
					InvalidateRectOnce(hWnd, &sUpdateRect, FALSE);
				}
			}
			else {
				if (szForm.dec_update_hover == TRUE) {
					szForm.dec_update_hover = FALSE;
					InvalidateRectOnce(hWnd, &sUpdateRect, FALSE);
				}
			}
		}
		if (PtInRect(&sRecordRect, point) == TRUE) {
			UpdateTooltip(szHandle[HANDLE_TOOLTIP_WND], hWnd, &sRecordRect, TEXT("录音"));
		}
		break;
	case WM_LBUTTONDOWN:
		szMouse.l_button_down = TRUE;
		szMouse.c_x = LOWORD(lParam);
		szMouse.c_y = HIWORD(lParam);
		point.x = szMouse.c_x;
		point.y = szMouse.c_y;
		szMsgTip.display = FALSE;
		if (InterlockedExchange(&szMsgTip.display, FALSE) == TRUE || InterlockedExchange(&szForm.first_run, FALSE) == TRUE) {
			InvalidateRectOnce(hWnd, NULL, FALSE);
			break;
		}
		if (szForm.close_hover) {
			//关闭
			PostMessage(hWnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
		}
		else if (szForm.dec_min_hover) {
			//小化
			InterlockedExchange(&szForm.minimize, TRUE);
			if (szOpenglEnv) {
				szOpenglEnv->iMinizeTicket[0] = GetTickCount();
			}
			PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		else if (szForm.dec_about_hover) {
			//关于
			if (szForm.dec_devs[2]) {
				if (!szForm.about) {
					InterlockedExchange(&szForm.about, TRUE);
					InvalidateRectOnce(hWnd, &sAboutContentRect, FALSE);
				}
				else {
					InterlockedExchange(&szForm.about, FALSE);
					InvalidateRectOnce(hWnd, &sAboutContentRect, FALSE);
				}
				return TRUE;
			}
		}
		else if (szForm.dec_update_hover && szNeedUpdate) {
			//更新版本
			ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/mengdj/c-lang/releases"), NULL, NULL, SW_SHOWNORMAL);
			return TRUE;
		}
		else if (szForm.dec_resize_hover) {
			//调整
			if (szForm.dec_devs[2]) {
				if (!szForm.resize) {
					SetWindowPos(
						szHandle[HANDLE_MAIN_WND],
						HWND_TOP,
						0,
						0,
						WS_WIDTH + WS_RESIZE_WIDTH,
						sClientRect.bottom,
						SWP_NOMOVE
					);
					SetWindowPos(
						szHandle[OPENGL_WND],
						HWND_TOP,
						0,
						0,
						sAnalysisRect.right + WS_RESIZE_WIDTH,
						sAnalysisRect.bottom,
						SWP_NOMOVE
					);
					ReSizeGl(0, 0, sAnalysisRect.right + WS_RESIZE_WIDTH, sAnalysisRect.bottom);
					InitAllRect(szHandle[HANDLE_MAIN_WND]);
					InterlockedExchange(&szForm.resize, TRUE);
				}
				else {
					SetWindowPos(
						szHandle[HANDLE_MAIN_WND],
						HWND_TOP,
						0,
						0,
						WS_WIDTH,
						sClientRect.bottom,
						SWP_NOMOVE
					);
					SetWindowPos(
						szHandle[OPENGL_WND],
						HWND_TOP,
						0,
						0,
						sAnalysisRect.right - WS_RESIZE_WIDTH,
						sAnalysisRect.bottom,
						SWP_NOMOVE
					);
					ReSizeGl(0, 0, sAnalysisRect.right - WS_RESIZE_WIDTH, sAnalysisRect.bottom);
					InitAllRect(szHandle[HANDLE_MAIN_WND]);
					InterlockedExchange(&szForm.resize, FALSE);
				}
				if (szCompatibleBitmap && szMemDc) {
					HDC hCurrentDC = GetDC(hWnd);
					szCompatibleBitmap = CreateCompatibleBitmap(hCurrentDC, sClientRect.right, sClientRect.bottom);
					DeleteObject(SelectObject(szMemDc, szCompatibleBitmap));
					ReleaseDC(hWnd, hCurrentDC);
				}
			}
			if (hAbtRgn) {
				DeleteObject(hAbtRgn);
				hAbtRgn = NULL;
			}
		}
		if (!szForm.about) {
			if (PtInRect(&sCutRect, point) == TRUE) {
				//截图
				SYSTEMTIME sysTime;
				GetLocalTime(&sysTime);
				CHAR cName[MAX_LOADSTRING];
				sprintf_s(
					cName,
					MAX_LOADSTRING,
					"%d%.2d%.2d%.2d%.2d%.2d.png",
					sysTime.wYear,
					sysTime.wMonth,
					sysTime.wDay,
					sysTime.wHour,
					sysTime.wMinute,
					sysTime.wSecond
				);
				CaptureAndBuildPng(szHandle[HANDLE_MAIN_WND], cName);
				WCHAR wName[MAX_LOADSTRING];
				if (Char2WChar(cName, wName)) {
					WCHAR msgBuff[MAX_LOADSTRING];
					swprintf_s(msgBuff, MAX_LOADSTRING, TEXT("%s %s"), TEXT("截图"), wName);
					WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0x49, 0xA8, 0x14), msgBuff, FALSE);
				}
			}
			else if (PtInRect(&sRecordRect, point) == TRUE) {
				//录音
				if (InterlockedExchange(&szForm.dec_record_hover, FALSE) == TRUE) {
					if (InterlockedExchange(&szRecEnc->start, FALSE) == TRUE) {
						if (szRecEnc != NULL) {
							//关闭解码器和采样器
							if (WaitForSingleObject(szHandle[MUTEXT_RECORDER], INFINITE) == WAIT_OBJECT_0) {
								WakeConditionVariable(&szRwCond[SHINE_COND_CUS]);
								WaitForSingleObject(szHandle[THREAD_SHINE], INFINITE);
								InterlockedExchange(&szForm.dec_record_hover, FALSE);
								//析构资源
								if (szHandle[ENC_OUT_CREATE_FILE]) {
									CloseHandle(szHandle[ENC_OUT_CREATE_FILE]);
									szHandle[ENC_OUT_CREATE_FILE] = NULL;
								}
								CloseHandle(szHandle[THREAD_SHINE]);
								szHandle[THREAD_SHINE] = NULL;
								ReleaseMutex(szHandle[MUTEXT_RECORDER]);
							}
							Audio_Reset();
						}
					}
					WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0x7A, 0x00), TEXT("结束录音"), FALSE);
				}
				else {
					//start
					if (szOpenglEnv) {
						szOpenglEnv->iMinizeTicket[0] = szOpenglEnv->iMinizeTicket[1] = 0;
						szOpenglEnv->bRestart = TRUE;
					}
					if (!szHandle[MUTEXT_RECORDER]) {
						szHandle[MUTEXT_RECORDER] = CreateMutex(NULL, FALSE, TEXT("MUTEXT_RECORDER"));
					}
					if (!szHandle[ENC_OUT_CREATE_FILE]) {
						SYSTEMTIME sysTime;
						GetLocalTime(&sysTime);
						swprintf_s(
							szRecEncName,
							MAX_LOADSTRING,
							TEXT("%d%.2d%.2d%.2d%.2d%.2d.mp3"),
							sysTime.wYear,
							sysTime.wMonth,
							sysTime.wDay,
							sysTime.wHour,
							sysTime.wMinute,
							sysTime.wSecond
						);
						szHandle[ENC_OUT_CREATE_FILE] = CreateFile(
							szRecEncName,
							GENERIC_WRITE | GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL
						);
					}
					if (WaitForSingleObject(szHandle[MUTEXT_RECORDER], INFINITE) == WAIT_OBJECT_0) {
						if (szRecEnc != NULL) {
							InterlockedExchange(&szForm.dec_record_hover, TRUE);
							InterlockedExchange(&szRecEnc->start, TRUE);
							StartAnalysis_Encoder(szRecEnc, szHandle[ENC_OUT_CREATE_FILE]);
							szHandle[THREAD_SHINE] = _beginthreadex(NULL, 0, Mp3EncodeProc, NULL, 0, NULL);
						}
						ReleaseMutex(szHandle[MUTEXT_RECORDER]);
					}
					WCHAR msgBuff[MAX_LOADSTRING];
					swprintf_s(msgBuff, MAX_LOADSTRING, TEXT("%s %s"), TEXT("录音"), szRecEncName);
					WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0x49, 0xA8, 0x14), msgBuff, TRUE);
				}
				//刷新
				InvalidateRectOnce(hWnd, &sRecordRect, FALSE);
			}
			else if (szForm.dec_hover) {
				//切换绘图类型
				szForm.dec_type = (szForm.dec_type == 1) ? 2 : 1;
			}
		}
		else {
			//打开默认广告
			if (PtInRect(&sAboutContentRect, point)) {
				ShellExecute(NULL, TEXT("open"), TEXT("http://s.click.taobao.com/t?e=m%3D2%26s%3DnuEbcp773t8cQipKwQzePCperVdZeJviEViQ0P1Vf2kguMN8XjClAkpW6R4gtXUbVIUOwLrt%2BYaX8w009aTaouWxeTcCpXvTV6inrJGdDjEs2bQHfoAA7%2Bdn1BbglxZYxUhy8exlzcpAFEHVckI7b3VyxRO0gvF4QxJtmCgOmCLXl8Q7TEjBF%2FXb8tq2dBVYdZ3HtmtNobWiZ%2BQMlGz6FQ%3D%3D"), NULL, NULL, SW_SHOWNORMAL);
			}
			else {
				InterlockedExchange(&szForm.about, FALSE);
				InvalidateRectOnce(hWnd, &sAboutContentRect, FALSE);
			}
		}
		break;
	case WM_QUERYOPEN:
		//窗口最小化恢复时
		InterlockedExchange(&szForm.minimize, FALSE);
		if (szOpenglEnv) {
			szOpenglEnv->iMinizeTicket[1] = GetTickCount();
		}
		InvalidateRectOnce(szHandle[OPENGL_WND], NULL, FALSE);
		break;
	case WM_QUERYENDSESSION:
		//关机
		WriteMsgContext(0, 1500, RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0x7A, 0x00), TEXT("关闭"), FALSE);
		PostMessage(hWnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
		break;
	case WM_COMPACTING:
		//内存不足
		ClearGifMemory();
		break;
	case WM_LBUTTONUP:
		szMouse.l_button_down = FALSE;
		break;
	case WM_DROPFILES:
		if (szHandle[OPENGL_WND]) {
			SendMessage(szHandle[OPENGL_WND], WM_DROPFILES, wParam, lParam);
		}
		break;
	case WM_USER:
		//托盘菜单
		if (lParam == WM_RBUTTONDOWN) {
			GetCursorPos(&point);
			SetForegroundWindow(hWnd);
			INT mId = TrackPopupMenu(szNotifyMenu, TPM_RETURNCMD | TPM_CENTERALIGN, point.x, point.y, NULL, hWnd, NULL);
			if (mId == IDM_NOTIFY_EXIT) {
				PostMessage(hWnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
			}
			else if (mId == IDM_NOTIFY_VERSION) {
				ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/mengdj/c-lang/releases"), NULL, NULL, SW_SHOWNORMAL);
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return TRUE;
}

VOID	WriteMsgContext(INT start, INT delay, COLORREF color, COLORREF background, CONST CHAR * msg, BOOL refresh) {
	szMsgTip.background = background;
	szMsgTip.color = color;
	szMsgTip.delay = delay;
	szMsgTip.start = start ? start : GetTickCount();
	szMsgTip.size.cx = szMsgTip.size.cy = 0;
	szMsgTip.display = TRUE;
	lstrcpy(szMsgTip.msg, msg);
	if (refresh) {
		InvalidateRectOnce(szHandle[HANDLE_MAIN_WND], NULL, FALSE);
	}
}

//日志
VOID DecLog(int line, const char* format, ...) {
	if (szLog == NULL) {
		fopen_s(&szLog, "decibel.log", "w+");
	}
	va_list arg;
	int done;
	va_start(arg, format);
	time_t now = time(NULL);
	struct tm time_log;
	localtime_s(&time_log, &now);
	fprintf(
		szLog,
		"%04d-%02d-%02d %02d:%02d:%02d 行数:%d 线程id:%d ",
		time_log.tm_year + 1900,
		time_log.tm_mon + 1,
		time_log.tm_mday,
		time_log.tm_hour,
		time_log.tm_min,
		time_log.tm_sec,
		line,
		(int)GetCurrentThreadId()
	);
	done = vfprintf(szLog, format, arg);
	va_end(arg);
	fprintf(szLog, "\n");
	fflush(szLog);
}