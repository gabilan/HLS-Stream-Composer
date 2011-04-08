/*
* Various utilities for command line tools
* copyright (c) 2003 Fabrice Bellard
*
* This file is part of FFmpeg.
*
* FFmpeg is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* FFmpeg is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef FFMPEG_CMDUTILS_H
#define FFMPEG_CMDUTILS_H

#include "stdafx.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <vector>
#if CONFIG_NETWORK
#include "libavformat/network.h"
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

using namespace std;

class TFunctor
{
public:

	const char *name;
	int flags;
	void* ptr_arg;
	const char *help;
	const char *argname;

	virtual void Func(const char *) = 0;
	virtual int Func2(const char *, const char *) = 0;

#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_GRAB   0x0040
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_FUNC2  0x0400
#define OPT_INT64  0x0800
#define OPT_EXIT   0x1000
};


// derived template class
template <class TClass> class OptionDef2 : public TFunctor
{
private:
	void (TClass::*fpt)(const char*);					// pointer to member function
	int (TClass::*fpt2)(const char *, const char *);	// pointer to member function
	TClass* pt2Object;								// pointer to object

public:

	// constructor - takes pointer to an object and pointer to a member and stores
	// them in two private variables
	OptionDef2(TClass* _pt2Object, 
		const char* name,
		int flags,
		void* ptr_arg,
		void (TClass::*_fpt)(const char *),
		int (TClass::*_fpt2)(const char *, const char *),
		const char* help,
		const char* argname = NULL)
	{
		pt2Object = _pt2Object; 
		fpt = _fpt; 
		fpt2 = _fpt2; 
		this->name = name;
		this->flags = flags;
		this->ptr_arg = ptr_arg;
		this->help = help;
		this-> argname = argname;
	}

	// override function "Call"
	virtual void Func(const char* string)
	{ (*pt2Object.*fpt)(string);};             // execute member function

	virtual int Func2(const char* string, const char* arg)
	{ return (*pt2Object.*fpt2)(string, arg);};             // execute member function
};

typedef struct {
	int64_t num_faulty_pts; /// Number of incorrect PTS values so far
	int64_t num_faulty_dts; /// Number of incorrect DTS values so far
	int64_t last_pts;       /// PTS of the last frame
	int64_t last_dts;       /// DTS of the last frame
} PtsCorrectionContext;

const int this_year = 2010;

void __cmd_exit(int exitCode)
{
	HANDLE hThread;

	hThread = GetCurrentThread();
	TerminateThread(hThread, exitCode);
}

void log_callback_help(void* ptr, int level, const char* fmt, va_list vl)
{
	vfprintf(stdout, fmt, vl);
}

double parse_number_or_die(const char *context, const char *numstr, int type, double min, double max)
{
	char *tail;
	const char *error;
	double d = av_strtod(numstr, &tail);
	if (*tail)
		error= "Expected number for %s but found: %s\n";
	else if (d < min || d > max)
		error= "The value for %s was %s which is not within %f - %f\n";
	else if(type == OPT_INT64 && (int64_t)d != d)
		error= "Expected int64 for %s but found %s\n";
	else
		return d;
	fprintf(stderr, error, context, numstr, min, max);
	__cmd_exit(1);
}

int64_t parse_time_or_die(const char *context, const char *timestr, int is_duration)
{
	int64_t us = parse_date(timestr, is_duration);
	if (us == INT64_MIN) {
		fprintf(stderr, "Invalid %s specification for %s: %s\n",
			is_duration ? "duration" : "date", context, timestr);
		__cmd_exit(1);
	}
	return us;
}

void show_help_options(const TFunctor *options, const char *msg, int mask, int value)
{
	const TFunctor *po;
	int first;

	first = 1;
	for(po = options; po->name != NULL; po++) {
		char buf[64];
		if ((po->flags & mask) == value) {
			if (first) {
				printf("%s", msg);
				first = 0;
			}
			av_strlcpy(buf, po->name, sizeof(buf));
			if (po->flags & HAS_ARG) {
				av_strlcat(buf, " ", sizeof(buf));
				av_strlcat(buf, po->argname, sizeof(buf));
			}
			printf("-%-17s  %s\n", buf, po->help);
		}
	}
}

template<class T> static const OptionDef2<T>* find_option(const OptionDef2<T> **po, const char *name){
	while ((*po) && (*po)->name != NULL) {
		if (!strcmp(name, (*po)->name))
			break;
		po++;
	}
	return *po;
}

template<class T> void parse_options(int argc, char **argv, const OptionDef2<T> **options,
									 void (* parse_arg_function)(const char*))
{
	const char *opt, *arg;
	int optindex, handleoptions=1;
	OptionDef2<T> *po;

	/* parse options */
	optindex = 0;
	while (optindex < argc) {
		opt = argv[optindex++];

		if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
			int bool_val = 1;
			if (opt[1] == '-' && opt[2] == '\0') {
				handleoptions = 0;
				continue;
			}
			opt++;
			po= (OptionDef2<T>*)find_option(options, opt);
			if (!po->name && opt[0] == 'n' && opt[1] == 'o') {
				/* handle 'no' bool option */
				po = (OptionDef2<T>*)find_option(options, opt + 2);
				if (!(po->name && (po->flags & OPT_BOOL)))
					goto unknown_opt;
				bool_val = 0;
			}
			if (!po->name)
				po= (OptionDef2<T>*)find_option(options, "default");
			if (!po->name) {
unknown_opt:
				fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], opt);
				__cmd_exit(1);
			}
			arg = NULL;
			if (po->flags & HAS_ARG) {
				arg = argv[optindex++];
				if (!arg) {
					fprintf(stderr, "%s: missing argument for option '%s'\n", argv[0], opt);
					__cmd_exit(1);
				}
			}
			if (po->flags & OPT_STRING) {
				char *str;
				str = av_strdup(arg);

				po->ptr_arg = malloc(sizeof(char) * strlen(str));
				strcpy((char*)po->ptr_arg, str);
			} else if (po->flags & OPT_BOOL) {
				//po->ptr_arg = malloc(sizeof(int));
				*((int*)(po->ptr_arg)) = bool_val;
			} else if (po->flags & OPT_INT) {
				//po->ptr_arg = malloc(sizeof(int));
				*((int*)(po->ptr_arg)) = (int)parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);
			} else if (po->flags & OPT_INT64) {
				//po->ptr_arg = malloc(sizeof(__int64));
				*((__int64*)(po->ptr_arg)) = (__int64)parse_number_or_die(opt, arg, OPT_INT64, INT64_MIN, INT64_MAX);
			} else if (po->flags & OPT_FLOAT) {
				*((float*)po->ptr_arg) = (float)parse_number_or_die(opt, arg, OPT_FLOAT, 
						FLT_MIN, 
						FLT_MAX);
			} else if (po->flags & OPT_FUNC2) {
				if (po->Func2(opt, arg) < 0) {
					fprintf(stderr, "%s: failed to set value '%s' for option '%s'\n", argv[0], arg, opt);
					__cmd_exit(1);
				}
			} else {
				po->Func(arg);
			}
			if(po->flags & OPT_EXIT)
				__cmd_exit(0);
		} else {
			if (parse_arg_function)
				parse_arg_function(opt);
		}
	}
}

int opt_timelimit(const char *opt, const char *arg)
{
#if HAVE_SETRLIMIT
	int lim = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
	struct rlimit rl = { lim, lim + 1 };
	if (setrlimit(RLIMIT_CPU, &rl))
		perror("setrlimit");
#else
	fprintf(stderr, "Warning: -%s not implemented on this OS\n", opt);
#endif
	return 0;
}

void print_error(const char *filename, int err)
{
	char errbuf[128];
	const char *errbuf_ptr = errbuf;

	if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
		errbuf_ptr = strerror(AVUNERROR(err));
	fprintf(stderr, "%s: %s\n", filename, errbuf_ptr);
}

static int warned_cfg = 0;

#define INDENT        1
#define SHOW_VERSION  2
#define SHOW_CONFIG   4

#define PRINT_LIB_INFO(outstream,libname,LIBNAME,flags)                 \
	if (CONFIG_##LIBNAME) {                                             \
	const char *indent = flags & INDENT? "  " : "";                 \
	if (flags & SHOW_VERSION) {                                     \
	unsigned int version = libname##_version();                 \
	fprintf(outstream, "%slib%-10s %2d.%2d.%2d / %2d.%2d.%2d\n", \
	indent, #libname,                                   \
	LIB##LIBNAME##_VERSION_MAJOR,                       \
	LIB##LIBNAME##_VERSION_MINOR,                       \
	LIB##LIBNAME##_VERSION_MICRO,                       \
	version >> 16, version >> 8 & 0xff, version & 0xff); \
	}                                                               \
	if (flags & SHOW_CONFIG) {                                      \
	const char *cfg = libname##_configuration();                \
	if (strcmp(FFMPEG_CONFIGURATION, cfg)) {                    \
	if (!warned_cfg) {                                      \
	fprintf(outstream,                                  \
	"%sWARNING: library configuration mismatch\n", \
	indent);                                    \
	warned_cfg = 1;                                     \
	}                                                       \
	fprintf(stderr, "%s%-11s configuration: %s\n",          \
	indent, #libname, cfg);                         \
	}                                                           \
	}                                                               \
	}                                                                   \

void list_fmts(void (*get_fmt_string)(char *buf, int buf_size, int fmt), int nb_fmts)
{
	int i;
	char fmt_str[128];
	for (i=-1; i < nb_fmts; i++) {
		get_fmt_string (fmt_str, sizeof(fmt_str), i);
		fprintf(stdout, "%s\n", fmt_str);
	}
}

void show_formats(void)
{
	AVInputFormat *ifmt=NULL;
	AVOutputFormat *ofmt=NULL;
	const char *last_name;

	printf(
		"File formats:\n"
		" D. = Demuxing supported\n"
		" .E = Muxing supported\n"
		" --\n");
	last_name= "000";
	for(;;){
		int decode=0;
		int encode=0;
		const char *name=NULL;
		const char *long_name=NULL;

		while((ofmt= av_oformat_next(ofmt))) {
			if((name == NULL || strcmp(ofmt->name, name)<0) &&
				strcmp(ofmt->name, last_name)>0){
					name= ofmt->name;
					long_name= ofmt->long_name;
					encode=1;
			}
		}
		while((ifmt= av_iformat_next(ifmt))) {
			if((name == NULL || strcmp(ifmt->name, name)<0) &&
				strcmp(ifmt->name, last_name)>0){
					name= ifmt->name;
					long_name= ifmt->long_name;
					encode=0;
			}
			if(name && strcmp(ifmt->name, name)==0)
				decode=1;
		}
		if(name==NULL)
			break;
		last_name= name;

		printf(
			" %s%s %-15s %s\n",
			decode ? "D":" ",
			encode ? "E":" ",
			name,
			long_name ? long_name:" ");
	}
}

void show_codecs(void)
{
	AVCodec *p=NULL, *p2;
	const char *last_name;
	printf(
		"Codecs:\n"
		" D..... = Decoding supported\n"
		" .E.... = Encoding supported\n"
		" ..V... = Video codec\n"
		" ..A... = Audio codec\n"
		" ..S... = Subtitle codec\n"
		" ...S.. = Supports draw_horiz_band\n"
		" ....D. = Supports direct rendering method 1\n"
		" .....T = Supports weird frame truncation\n"
		" ------\n");
	last_name= "000";
	for(;;){
		int decode=0;
		int encode=0;
		int cap=0;
		const char *type_str;

		p2=NULL;
		while((p= av_codec_next(p))) {
			if((p2==NULL || strcmp(p->name, p2->name)<0) &&
				strcmp(p->name, last_name)>0){
					p2= p;
					decode= encode= cap=0;
			}
			if(p2 && strcmp(p->name, p2->name)==0){
				if(p->decode) decode=1;
				if(p->encode) encode=1;
				cap |= p->capabilities;
			}
		}
		if(p2==NULL)
			break;
		last_name= p2->name;

		switch(p2->type) {
case AVMEDIA_TYPE_VIDEO:
	type_str = "V";
	break;
case AVMEDIA_TYPE_AUDIO:
	type_str = "A";
	break;
case AVMEDIA_TYPE_SUBTITLE:
	type_str = "S";
	break;
default:
	type_str = "?";
	break;
		}
		printf(
			" %s%s%s%s%s%s %-15s %s",
			decode ? "D": (/*p2->decoder ? "d":*/" "),
			encode ? "E":" ",
			type_str,
			cap & CODEC_CAP_DRAW_HORIZ_BAND ? "S":" ",
			cap & CODEC_CAP_DR1 ? "D":" ",
			cap & CODEC_CAP_TRUNCATED ? "T":" ",
			p2->name,
			p2->long_name ? p2->long_name : "");
		/* if(p2->decoder && decode==0)
		printf(" use %s for decoding", p2->decoder->name);*/
		printf("\n");
	}
	printf("\n");
	printf(
		"Note, the names of encoders and decoders do not always match, so there are\n"
		"several cases where the above table shows encoder only or decoder only entries\n"
		"even though both encoding and decoding are supported. For example, the h263\n"
		"decoder corresponds to the h263 and h263p encoders, for file formats it is even\n"
		"worse.\n");
}

void show_bsfs(void)
{
	AVBitStreamFilter *bsf=NULL;

	printf("Bitstream filters:\n");
	while((bsf = av_bitstream_filter_next(bsf)))
		printf("%s\n", bsf->name);
	printf("\n");
}

void show_protocols(void)
{
	URLProtocol *up=NULL;

	printf("Supported file protocols:\n"
		"I.. = Input  supported\n"
		".O. = Output supported\n"
		"..S = Seek   supported\n"
		"FLAGS NAME\n"
		"----- \n");
	while((up = av_protocol_next(up)))
		printf("%c%c%c   %s\n",
		up->url_read  ? 'I' : '.',
		up->url_write ? 'O' : '.',
		up->url_seek  ? 'S' : '.',
		up->name);
}

void show_filters(void)
{
	AVFilter av_unused(**filter) = NULL;

	printf("Filters:\n");
#if CONFIG_AVFILTER
	while ((filter = av_filter_next(filter)) && *filter)
		printf("%-16s %s\n", (*filter)->name, (*filter)->description);
#endif
}

int read_yesno(void)
{
	int c = getchar();
	int yesno = (toupper(c) == 'Y');

	while (c != '\n' && c != EOF)
		c = getchar();

	return yesno;
}

int read_file(const char *filename, char **bufptr, size_t *size)
{
	FILE *f = fopen(filename, "rb");

	if (!f) {
		fprintf(stderr, "Cannot read file '%s': %s\n", filename, strerror(errno));
		return AVERROR(errno);
	}
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*bufptr = (char*)av_malloc(*size + 1);
	if (!*bufptr) {
		fprintf(stderr, "Could not allocate file buffer\n");
		fclose(f);
		return AVERROR(ENOMEM);
	}
	fread(*bufptr, 1, *size, f);
	(*bufptr)[*size++] = '\0';

	fclose(f);
	return 0;
}

void init_pts_correction(PtsCorrectionContext *ctx)
{
	ctx->num_faulty_pts = ctx->num_faulty_dts = 0;
	ctx->last_pts = ctx->last_dts = INT64_MIN;
}

int64_t guess_correct_pts(PtsCorrectionContext *ctx, int64_t reordered_pts, int64_t dts)
{
	int64_t pts = AV_NOPTS_VALUE;

	if (dts != AV_NOPTS_VALUE) {
		ctx->num_faulty_dts += dts <= ctx->last_dts;
		ctx->last_dts = dts;
	}
	if (reordered_pts != AV_NOPTS_VALUE) {
		ctx->num_faulty_pts += reordered_pts <= ctx->last_pts;
		ctx->last_pts = reordered_pts;
	}
	if ((ctx->num_faulty_pts<=ctx->num_faulty_dts || dts == AV_NOPTS_VALUE)
		&& reordered_pts != AV_NOPTS_VALUE)
		pts = reordered_pts;
	else
		pts = dts;

	return pts;
}

#endif /* FFMPEG_CMDUTILS_H */
