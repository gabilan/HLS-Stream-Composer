// HlsStreamComposer.Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "HlsStreamComposer.Native.h"
#include "NamedPipeProtocol.h"
#include "Transcoder.h"
#include "Segmenter.h"
#include "Probe.h"

int initialized = 0;

HLSSTREAMCOMPOSERNATIVE_API Initialize(const char* status_log_path, const char* error_log_path)
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

HLSSTREAMCOMPOSERNATIVE_API RunTranscoder(void* lpParameter, int len)
{
	auto_ptr<char> buffer(((char*)lpParameter));
	int _argc = 0, _currentOffset = 0;
	char** _argv = NULL;	
		
	_argc = buffer.get()[0];
	_argv = new char*[_argc];
	_currentOffset = 1 + _argc;

	//stop any other client transcode threads if necessary
	{
	for(map<DWORD, CTranscoder*>::iterator it = transcoder_map.begin(); it != transcoder_map.end(); it++)
			(it->second)->sigterm_handler(SIGTERM);

		transcoder_map.clear();
	}

	for(int i = 0; i < _argc; ++i)
	{
		int argLen = (unsigned char)buffer.get()[1 + i];

		_argv[i] = (char*)malloc(argLen + 1);
		memcpy(_argv[i], &(buffer.get()[_currentOffset]), argLen);
		_argv[i][argLen] = '\0';

		_currentOffset += argLen;
	}

	auto_ptr<CTranscoder> t(new CTranscoder);
	int ret = t.get()->Run(_argc, _argv);

	t.reset();
	buffer.reset();
	return ret;
}

HLSSTREAMCOMPOSERNATIVE_API RunSegmenter(void* lpParameter, int len)
{
	auto_ptr<char> buffer(((char*)lpParameter));
	int _argc = 0, _currentOffset = 0;
	char** _argv = NULL;	
		
	_argc = buffer.get()[0];
	_argv = new char*[_argc];
	_currentOffset = 1 + _argc;

	for(int i = 0; i < _argc; ++i)
	{
		int argLen = (unsigned char)buffer.get()[1 + i];

		_argv[i] = (char*)malloc(argLen + 1);
		memcpy(_argv[i], &(buffer.get()[_currentOffset]), argLen);
		_argv[i][argLen] = '\0';

		_currentOffset += argLen;
	}

	auto_ptr<CSegmenter> t (new CSegmenter);
	int ret = t.get()->Run(_argc, _argv);

	t.reset();
	buffer.reset();
	return ret;
}

HLSSTREAMCOMPOSERNATIVE_API RunProbe(void* lpParameter, int len)
{
	auto_ptr<char> buffer(((char*)lpParameter));
	int _argc = 0, _currentOffset = 0, _clientId = 0;
	char** _argv = NULL;	
		
	_argc = buffer.get()[0];
	_argv = new char*[_argc];
	_currentOffset = 1 + _argc;

	for(int i = 0; i < _argc; ++i)
	{
		int argLen = (unsigned char)buffer.get()[1 + i];

		_argv[i] = (char*)malloc(argLen + 1);
		memcpy(_argv[i], &(buffer.get()[_currentOffset]), argLen);
		_argv[i][argLen] = '\0';

		_currentOffset += argLen;
	}

	auto_ptr<CProbe> t (new CProbe);
	int ret = t.get()->Run(_argv[0], _argv[1]);

	t.reset();
	buffer.reset();
	return ret;
}

HLSSTREAMCOMPOSERNATIVE_API StopTranscoder()
{
	for(map<DWORD, CTranscoder*>::iterator it = transcoder_map.begin(); it != transcoder_map.end(); it++)
		(it->second)->sigterm_handler(SIGTERM);		

	return 0;
}

