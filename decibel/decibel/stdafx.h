// stdafx.h: 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 项目特定的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdarg.h>
#include <time.h>

#pragma comment(lib,"msimg32.lib")

#include <gl/GL.h>
#include <gl/GLU.h>
#pragma comment(lib,"opengl32.lib")

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")  

#include <shellapi.h>
#pragma   comment(lib,"shell32.lib") 

#include <process.h>
#define NOUNCRYPT

#include "pa_ringbuffer.h"
#ifndef MIN
#define MIN(x,y)  (((x)<(y))?(x):(y))
#endif

#include "_kiss_fft_guts.h"

#include <ocidl.h>    
#include <olectl.h>   

#ifndef ALIGN_OF
#define ALIGN_OF(d,a) (((d) + (a - 1)) & ~(a - 1))
#endif

#define	ARRAY_LEN(x)	((int) (sizeof (x) / sizeof ((x) [0])))

