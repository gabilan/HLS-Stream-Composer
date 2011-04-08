// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <signal.h>
#include <sys/stat.h>

#if WIN32
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
//#pragma comment(lib, "avcore.lib")
#endif

#include <inttypes.h>
#include <limits>
#include <vector>

using namespace std;

#define __STDC_FORMAT_MACROS

#define snprintf _snprintf
#define usleep Sleep
#define UINT64_C(c) c ## ULL
#define INT64_C(c) c ## LL
#define INT64_MIN -0x8000000000000000LL
#define INT64_MAX 0x7fffffffffffffffLL
#define MAX_FILES 20

extern "C"{
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avstring.h"
#include "libavutil/pixdesc.h"
#include "libavutil/eval.h"
#include "libavcodec/opt.h"
//#include "libavcore/avcore.h"
}
