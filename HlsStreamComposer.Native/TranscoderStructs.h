#pragma once
//#include "stdafx.h" 

/* select an input stream for an output stream */
typedef struct AVStreamMap {
	int file_index;
	int stream_index;
	int sync_file_index;
	int sync_stream_index;
} AVStreamMap;

/**
* select an input file for an output file
*/
typedef struct AVMetaDataMap {
	int out_file;
	int in_file;
} AVMetaDataMap;

#define DEFAULT_PASS_LOGFILENAME_PREFIX "ffmpeg2pass"

struct AVInputStream;

typedef struct AVOutputStream {
	int file_index;          /* file index */
	int index;               /* stream index in the output file */
	int source_index;        /* AVInputStream index */
	AVStream *st;            /* stream in the output file */
	int encoding_needed;     /* true if encoding needed for this stream */
	int frame_number;
	/* input pts and corresponding output pts
	for A/V sync */
	//double sync_ipts;        /* dts from the AVPacket of the demuxer in second units */
	AVInputStream *sync_ist; /* input stream to sync against */
	int64_t sync_opts;       /* output frame counter, could be changed to some true timestamp */ //FIXME look at frame_number
	/* video only */
	int video_resample;
	AVFrame pict_tmp;      /* temporary image for resampling */
	SwsContext *img_resample_ctx; /* for image resampling */
	int resample_height;
	int resample_width;
	int resample_pix_fmt;

	/* full frame size of first frame */
	int original_height;
	int original_width;

	/* cropping area sizes */
	int video_crop;
	int topBand;
	int bottomBand;
	int leftBand;
	int rightBand;

	/* cropping area of first frame */
	int original_topBand;
	int original_bottomBand;
	int original_leftBand;
	int original_rightBand;

	/* audio only */
	int audio_resample;
	ReSampleContext *resample; /* for audio resampling */
	int reformat_pair;
	AVAudioConvert *reformat_ctx;
	AVFifoBuffer *fifo;     /* for compression: one audio fifo per codec */
	FILE *logfile;
} AVOutputStream;

typedef struct AVInputStream {
	int file_index;
	int index;
	AVStream *st;
	int discard;             /* true if stream data should be discarded */
	int decoding_needed;     /* true if the packets must be decoded in 'raw_fifo' */
	int64_t sample_index;      /* current sample */

	int64_t       start;     /* time when read started */
	int64_t       next_pts;  /* synthetic pts for cases where pkt.pts
							 is not defined */
	int64_t       pts;       /* current pts */
	int is_start;            /* is 1 at the start and after a discontinuity */
	int showed_multi_packet_warning;
	int is_past_recording_time;
#if CONFIG_AVFILTER
	AVFilterContext *output_video_filter;
	AVFilterContext *input_video_filter;
	AVFrame *filter_frame;
	int has_filter_frame;
	AVFilterBufferRef *picref;
#endif
} AVInputStream;

typedef struct AVInputFile {
	int eof_reached;      /* true if eof reached */
	int ist_index;        /* index of first stream in ist_table */
	int buffer_size;      /* current total buffer size */
	int nb_streams;       /* nb streams we are aware of */
} AVInputFile;

#if HAVE_TERMIOS_H

/* init terminal so that we can grab keys */
static struct termios oldtty;
#endif

#define av_assert0(cond) do {                                           \
    if (!(cond)) {                                                      \
        av_log(NULL, AV_LOG_FATAL, "Assertion %s failed at %s:%d\n",    \
               AV_STRINGIFY(cond), __FILE__, __LINE__);                 \
        abort();                                                        \
    }                                                                   \
} while (0)

#include <emmintrin.h> 
static inline long lrintf(float f) { 
        return _mm_cvtss_si32(_mm_load_ss(&f)); 
} 