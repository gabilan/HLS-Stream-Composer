// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HLSSTREAMCOMPOSERNATIVE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HLSSTREAMCOMPOSERNATIVE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef HLSSTREAMCOMPOSERNATIVE_EXPORTS
#define HLSSTREAMCOMPOSERNATIVE_API extern "C" __declspec(dllexport) int __cdecl
#else
#define HLSSTREAMCOMPOSERNATIVE_API __declspec(dllimport)
#endif

HLSSTREAMCOMPOSERNATIVE_API Initialize(const char* status_log_path, const char* error_log_path);
HLSSTREAMCOMPOSERNATIVE_API RunTranscoder(void* lpParameter, int len);
HLSSTREAMCOMPOSERNATIVE_API RunSegmenter(void* lpParameter, int len);
HLSSTREAMCOMPOSERNATIVE_API RunProbe(void* lpParameter, int len);
HLSSTREAMCOMPOSERNATIVE_API StopTranscoder();

