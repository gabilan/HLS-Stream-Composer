#pragma once

char* named_pipe_semaphore_name = NULL;

struct NamedPipeProtocolContext
{
	HANDLE hPipe;
	char* pipeName;
	BOOL pipeOwner;
	BOOL pipeConnected;
};

char *replace_str(char *str, char *orig, char *rep)
{
	static char buffer[4096];
	char *p;

	if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
		return str;

	strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
	buffer[p-str] = '\0';

	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
	return buffer;
}

int namedp_open(URLContext *h, const char *url, int flags)
{
	HANDLE hSemaphore = NULL;
	NamedPipeProtocolContext* ctx = (NamedPipeProtocolContext*)malloc(sizeof(NamedPipeProtocolContext));	
	if(!ctx)
		return -1;
	else
		memset(ctx, 0, sizeof(NamedPipeProtocolContext));

	char* cUrl = replace_str((char*)url, "namedp://", "\\\\.\\pipe\\");	
	int strLen = strlen(cUrl);

	ctx->pipeName = (char*)malloc(strLen);
	strcpy(ctx->pipeName, cUrl);

	wchar_t* wUrl = (wchar_t*)malloc(sizeof(wchar_t) * (strLen + 1));

	mbstowcs(wUrl, cUrl, strLen);
	wUrl[strLen] = L'\0';

	if(flags == URL_RDONLY){
		printf("Connecting to pipe: '%s'\n", cUrl);
		if(WaitNamedPipe(wUrl, 5000) == 0)
		{
			printf("Error: Pipe '%s' connection timeout.\n", cUrl);
			return -1;
		}
		else
		{
			printf("Pipe '%s' was successfully connected.\n", ctx->pipeName);
		}

		ctx->hPipe = CreateFile( 
			wUrl,   // pipe name 
			GENERIC_READ|GENERIC_WRITE,  // read access
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 
	}
	else
	{
		ctx->hPipe = CreateNamedPipe(
			wUrl,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_WRITE_THROUGH,
			PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			PIPE_UNLIMITED_INSTANCES,
			32768,
			32768,
			5000,
			NULL);

		if(ctx->hPipe != INVALID_HANDLE_VALUE)
			ctx->pipeOwner = TRUE;
	}

	//error if the pipe handle is valid.  
	if (ctx->hPipe == INVALID_HANDLE_VALUE) 
	{
		printf("Error: Unable to open pipe '%s'.\n", wUrl);
		return -1;     
	}
	else
	{
		printf("Pipe '%s' was successfully opened.\n", ctx->pipeName);
	}

	if(!SetNamedPipeHandleState(ctx->hPipe, 0, NULL, NULL))
	{
		printf("Error: Unable to set pipe handle state on '%s'.\n", wUrl);
		return -1;
	}
	else
	{
		printf("State of pipe handle '%s' was successfully set.\n", ctx->pipeName);
	}

	h->priv_data = ctx;
	return 0;
}

int namedp_read(URLContext *h, unsigned char *buf, int size)
{
	if(!h->priv_data)
		return -1;

	NamedPipeProtocolContext *ctx = (NamedPipeProtocolContext*)h->priv_data;
	HANDLE hPipe = ctx->hPipe;
	if(!hPipe)
		return -1;
	else if(ctx->pipeOwner && !ctx->pipeConnected)
		ConnectNamedPipe(ctx->hPipe, NULL);

	//if(namedp_data_available(h, buf, size, 5000) < 0)
	//return -1;

	DWORD bytesRead = 0;
	DWORD readErrors = 0;

	do
	{
		BOOL result = ReadFile( 
			hPipe,    // pipe handle 
			buf,    // buffer to receive reply 
			size,  // size of buffer 
			&bytesRead,  // number of bytes read 
			NULL);    // not overlapped

		if(bytesRead == 0 && size > 0)
		{
			printf("Warning: failed to read bytes from the input stream. Pipe name: '%s'. Error #%d.\n", ctx->pipeName, ++readErrors);
			Sleep(10);
		}
	}while(bytesRead == 0 && readErrors < 10);

	return bytesRead;
}

int namedp_write(URLContext *h, const unsigned char *buf, int size)
{
	if(!h->priv_data)
		return -1;

	NamedPipeProtocolContext *ctx = (NamedPipeProtocolContext*)h->priv_data;
	HANDLE hPipe = ctx->hPipe;
	if(!hPipe)
		return -1;
	else if(ctx->pipeOwner && !ctx->pipeConnected)
		ConnectNamedPipe(ctx->hPipe, NULL);

	DWORD bytesWritten = 0;
	DWORD writeErrors = 0;

	do{
		WriteFile(
			hPipe,			//pipe handle
			buf,			//buffer to write
			size,			//buffer size
			&bytesWritten,	//number of bytes written
			NULL);			//not overlapped

		if(bytesWritten == 0  && size > 0)
		{
			printf("Warning: failed to write bytes to the output stream. Pipe name: '%s'. Error #%d.\n", ctx->pipeName, ++writeErrors);
			Sleep(10);
		}
	}while(bytesWritten == 0 && writeErrors < 10);

	return bytesWritten;
}

int64_t namedp_seek(URLContext *h, int64_t pos, int whence)
{
	return 0;
}

int namedp_close(URLContext *h)
{
	int ret = 0;
	if(!h->priv_data)
		return 0;

	NamedPipeProtocolContext *ctx = (NamedPipeProtocolContext*)h->priv_data;
	HANDLE hPipe = ctx->hPipe;
	if(!hPipe)
		return 0;

	if(ctx->pipeOwner)
		DisconnectNamedPipe(ctx->hPipe);

	ret = CloseHandle(hPipe);
	if(ctx->pipeName){
		free(ctx->pipeName);		
		ctx->pipeName = NULL;
	}
	
	free(ctx);
	h->priv_data = NULL;

	return ret;
}

URLProtocol namedp_protocol = {
	"namedp",
	namedp_open,
	namedp_read,
	namedp_write,
	NULL, //namedp_seek,
	namedp_close,
};