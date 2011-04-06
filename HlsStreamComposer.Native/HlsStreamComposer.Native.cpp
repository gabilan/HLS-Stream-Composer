// HlsStreamComposer.Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "HlsStreamComposer.Native.h"
#include "NamedPipeProtocol.h"

#pragma pack(2)

int initialized = 0;

HLSSTREAMCOMPOSERNATIVE_API int Initialize(const char* status_log_path, const char* error_log_path)
{		
	if(initialized)
		return 1;
	else
		initialized = 1;

	av_register_all();	
	avcodec_register_all();
	avfilter_register_all();

	register_protocol(&namedp_protocol);

	//set the log paths
	try{
		freopen(status_log_path, "w", stdout);
		freopen(error_log_path, "w", stderr);
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	}catch(...){
	}

	return 0;
}
