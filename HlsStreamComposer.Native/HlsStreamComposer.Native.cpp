// HlsStreamComposer.Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "HlsStreamComposer.Native.h"
#include "NamedPipeProtocol.h"

#pragma pack(2)

int initialized = 0;

HLSSTREAMCOMPOSERNATIVE_API int Initialize(void)
{		
	if(initialized)
		return 1;
	else
		initialized = 1;

	av_register_all();	
	avcodec_register_all();
	avfilter_register_all();

	register_protocol(&namedp_protocol);

	return 0;
}
