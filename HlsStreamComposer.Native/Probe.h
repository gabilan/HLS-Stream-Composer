#pragma once

#include "stdafx.h" 
#include "cmdutils.h"
#include <inttypes.h>

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/opt.h"
#include "libavutil/pixdesc.h"
#include "libavdevice/avdevice.h"
}

#define PRId64       "I64d"
#define AV_OPT_FLAG_ENCODING_PARAM 1
#define AV_OPT_FLAG_DECODING_PARAM 2

static const char* binary_unit_prefixes[]  = { "", "Ki", "Mi", "Gi", "Ti", "Pi" };
static const char* decimal_unit_prefixes[] = { "", "K" , "M" , "G" , "T" , "P"  };

class CProbe
{
	char* program_name;
	int program_birth_year;

	int do_show_format;
	int do_show_packets;
	int do_show_streams;

	int convert_tags;
	int show_value_unit;
	int use_value_prefix;
	int use_byte_value_binary_prefix;
	int use_value_sexagesimal_format;

	/* FFprobe context */
	char *input_filename;
	char *output_filename;	
	FILE *pOutFile;
	AVInputFormat *iformat;

	char *unit_second_str;
	char *unit_hertz_str;
	char *unit_byte_str;
	char *unit_bit_per_second_str;

	char **opt_names;
	char **opt_values;
	int opt_name_count;
	AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
	AVFormatContext *avformat_opts;
	struct SwsContext *sws_opts;

	char *value_string(char *buf, int buf_size, double val, const char *unit)
	{
		if (unit == unit_second_str && use_value_sexagesimal_format) {
			double secs;
			int hours, mins;
			secs  = val;
			mins  = (int)secs / 60;
			secs  = secs - mins * 60;
			hours = mins / 60;
			mins %= 60;
			snprintf(buf, buf_size, "%d:%02d:%09.6f", hours, mins, secs);
		} else if (use_value_prefix) {
			const char *prefix_string;
			int index;

			if (unit == unit_byte_str && use_byte_value_binary_prefix) {
				index = (int) (log(val)/log(2.0f)) / 10;
				index = av_clip(index, 0, FF_ARRAY_ELEMS(binary_unit_prefixes) -1);
				val /= pow(2.0f, index*10);
				prefix_string = binary_unit_prefixes[index];
			} else {
				index = (int) (log10(val)) / 3;
				index = av_clip(index, 0, FF_ARRAY_ELEMS(decimal_unit_prefixes) -1);
				val /= pow(10.0f, index*3);
				prefix_string = decimal_unit_prefixes[index];
			}

			snprintf(buf, buf_size, "%.3f %s%s", val, prefix_string, show_value_unit ? unit : "");
		} else {
			snprintf(buf, buf_size, "%f %s", val, show_value_unit ? unit : "");
		}

		return buf;
	}

	char *time_value_string(char *buf, int buf_size, int64_t val, const AVRational *time_base)
	{
		if (val == AV_NOPTS_VALUE) {
			snprintf(buf, buf_size, "N/A");
		} else {
			value_string(buf, buf_size, val * av_q2d(*time_base), unit_second_str);
		}

		return buf;
	}

	char *ts_value_string (char *buf, int buf_size, int64_t ts)
	{
		if (ts == AV_NOPTS_VALUE) {
			snprintf(buf, buf_size, "N/A");
		} else {
			snprintf(buf, buf_size, "%" PRId64, ts);
		}

		return buf;
	}

	const char *media_type_string(enum AVMediaType media_type)
	{
		switch (media_type) {
	case AVMEDIA_TYPE_VIDEO:      return "video";
	case AVMEDIA_TYPE_AUDIO:      return "audio";
	case AVMEDIA_TYPE_DATA:       return "data";
	case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
	case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
	default:                      return "unknown";
		}
	}

	void show_packet(AVFormatContext *fmt_ctx, AVPacket *pkt)
	{
		char val_str[128];
		AVStream *st = fmt_ctx->streams[pkt->stream_index];

		fprintf(pOutFile, "[PACKET]\n");
		fprintf(pOutFile, "codec_type=%s\n"   , media_type_string(st->codec->codec_type));
		fprintf(pOutFile, "stream_index=%d\n" , pkt->stream_index);
		fprintf(pOutFile, "pts=%s\n"          , ts_value_string  (val_str, sizeof(val_str), pkt->pts));
		fprintf(pOutFile, "pts_time=%s\n"     , time_value_string(val_str, sizeof(val_str), pkt->pts, &st->time_base));
		fprintf(pOutFile, "dts=%s\n"          , ts_value_string  (val_str, sizeof(val_str), pkt->dts));
		fprintf(pOutFile, "dts_time=%s\n"     , time_value_string(val_str, sizeof(val_str), pkt->dts, &st->time_base));
		fprintf(pOutFile, "duration=%s\n"     , ts_value_string  (val_str, sizeof(val_str), pkt->duration));
		fprintf(pOutFile, "duration_time=%s\n", time_value_string(val_str, sizeof(val_str), pkt->duration, &st->time_base));
		fprintf(pOutFile, "size=%s\n"         , value_string     (val_str, sizeof(val_str), pkt->size, unit_byte_str));
		fprintf(pOutFile, "pos=%"PRId64"\n"   , pkt->pos);
		fprintf(pOutFile, "flags=%c\n"        , pkt->flags & AV_PKT_FLAG_KEY ? 'K' : '_');
		fprintf(pOutFile, "[/PACKET]\n");
	}

	void show_packets(AVFormatContext *fmt_ctx)
	{
		AVPacket pkt;

		av_init_packet(&pkt);

		while (!av_read_frame(fmt_ctx, &pkt))
			show_packet(fmt_ctx, &pkt);
	}

	void show_stream(AVFormatContext *fmt_ctx, int stream_idx)
	{
		AVStream *stream = fmt_ctx->streams[stream_idx];
		AVCodecContext *dec_ctx;
		AVCodec *dec;
		char val_str[128];
		AVMetadataTag *tag = NULL;
		AVRational display_aspect_ratio;

		fprintf(pOutFile, "[STREAM]\n");

		fprintf(pOutFile, "index=%d\n",        stream->index);

		if ((dec_ctx = stream->codec)) {
			if ((dec = dec_ctx->codec)) {
				fprintf(pOutFile, "codec_name=%s\n",         dec->name);
				fprintf(pOutFile, "codec_long_name=%s\n",    dec->long_name);
			} else {
				fprintf(pOutFile, "codec_name=unknown\n");
			}

			fprintf(pOutFile, "codec_type=%s\n",         media_type_string(dec_ctx->codec_type));
			fprintf(pOutFile, "codec_time_base=%d/%d\n", dec_ctx->time_base.num, dec_ctx->time_base.den);

			/* print AVI/FourCC tag */
			av_get_codec_tag_string(val_str, sizeof(val_str), dec_ctx->codec_tag);
			fprintf(pOutFile, "codec_tag_string=%s\n", val_str);
			fprintf(pOutFile, "codec_tag=0x%04x\n", dec_ctx->codec_tag);

			switch (dec_ctx->codec_type) {
	case AVMEDIA_TYPE_VIDEO:
		fprintf(pOutFile, "width=%d\n",                   dec_ctx->width);
		fprintf(pOutFile, "height=%d\n",                  dec_ctx->height);
		fprintf(pOutFile, "has_b_frames=%d\n",            dec_ctx->has_b_frames);
		if (dec_ctx->sample_aspect_ratio.num) {
			fprintf(pOutFile, "sample_aspect_ratio=%d:%d\n", dec_ctx->sample_aspect_ratio.num,
				dec_ctx->sample_aspect_ratio.den);
			av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
				dec_ctx->width  * dec_ctx->sample_aspect_ratio.num,
				dec_ctx->height * dec_ctx->sample_aspect_ratio.den,
				1024*1024);
			fprintf(pOutFile, "display_aspect_ratio=%d:%d\n", display_aspect_ratio.num,
				display_aspect_ratio.den);
		}
		/*fprintf(pOutFile, "pix_fmt=%s\n",                 dec_ctx->pix_fmt != PIX_FMT_NONE ?
		av_pix_fmt_descriptors[dec_ctx->pix_fmt].name : "unknown");*/
		break;

	case AVMEDIA_TYPE_AUDIO:
		fprintf(pOutFile, "sample_rate=%s\n",             value_string(val_str, sizeof(val_str),
			dec_ctx->sample_rate,
			unit_hertz_str));
		fprintf(pOutFile, "channels=%d\n",                dec_ctx->channels);
		fprintf(pOutFile, "bits_per_sample=%d\n",         av_get_bits_per_sample(dec_ctx->codec_id));
		break;
			}
		} else {
			fprintf(pOutFile, "codec_type=unknown\n");
		}

		if (fmt_ctx->iformat->flags & AVFMT_SHOW_IDS)
			fprintf(pOutFile, "id=0x%x\n", stream->id);
		fprintf(pOutFile, "r_frame_rate=%d/%d\n",         stream->r_frame_rate.num,   stream->r_frame_rate.den);
		fprintf(pOutFile, "avg_frame_rate=%d/%d\n",       stream->avg_frame_rate.num, stream->avg_frame_rate.den);
		fprintf(pOutFile, "time_base=%d/%d\n",            stream->time_base.num,      stream->time_base.den);
		if (stream->language[0])
			fprintf(pOutFile, "language=%s\n",            stream->language);
		fprintf(pOutFile, "start_time=%s\n",   time_value_string(val_str, sizeof(val_str), stream->start_time,
			&stream->time_base));
		fprintf(pOutFile, "duration=%s\n",     time_value_string(val_str, sizeof(val_str), stream->duration,
			&stream->time_base));
		if (stream->nb_frames)
			fprintf(pOutFile, "nb_frames=%"PRId64"\n",    stream->nb_frames);

		while ((tag = av_metadata_get(stream->metadata, "", tag, AV_METADATA_IGNORE_SUFFIX)))
			fprintf(pOutFile, "TAG:%s=%s\n", tag->key, tag->value);

		fprintf(pOutFile, "[/STREAM]\n");
	}

	void show_format(AVFormatContext *fmt_ctx)
	{
		AVMetadataTag *tag = NULL;
		char val_str[128];

		fprintf(pOutFile, "[FORMAT]\n");

		fprintf(pOutFile, "filename=%s\n",         fmt_ctx->filename);
		fprintf(pOutFile, "nb_streams=%d\n",       fmt_ctx->nb_streams);
		fprintf(pOutFile, "format_name=%s\n",      fmt_ctx->iformat->name);
		fprintf(pOutFile, "format_long_name=%s\n", fmt_ctx->iformat->long_name);

		AVRational time_base = {1, AV_TIME_BASE};
		fprintf(pOutFile, "start_time=%s\n",       time_value_string(val_str, sizeof(val_str), fmt_ctx->start_time, &time_base));

		AVRational time_base1 = {1, AV_TIME_BASE};
		fprintf(pOutFile, "duration=%s\n",         time_value_string(val_str, sizeof(val_str), fmt_ctx->duration, &time_base1));

		fprintf(pOutFile, "size=%s\n",             value_string(val_str, sizeof(val_str), fmt_ctx->file_size, unit_byte_str));
		fprintf(pOutFile, "bit_rate=%s\n",         value_string(val_str, sizeof(val_str), fmt_ctx->bit_rate, unit_bit_per_second_str));

		if (convert_tags)
			av_metadata_conv(fmt_ctx, NULL, fmt_ctx->iformat->metadata_conv);
		while ((tag = av_metadata_get(fmt_ctx->metadata, "", tag, AV_METADATA_IGNORE_SUFFIX)))
			fprintf(pOutFile, "TAG:%s=%s\n", tag->key, tag->value);

		fprintf(pOutFile, "[/FORMAT]\n");
	}

	void set_context_opts(void *ctx, void *opts_ctx, int flags, AVCodec *codec)
	{
		int i;
		void *priv_ctx=NULL;

		if(!strcmp("AVCodecContext", (*(AVClass**)ctx)->class_name)){
			AVCodecContext *avctx = (AVCodecContext *)ctx;
			if(codec && codec->priv_class && avctx->priv_data){
				priv_ctx= avctx->priv_data;
			}
		}

		for(i=0; i<opt_name_count; i++){
			char buf[256];
			const AVOption *opt;
			const char *str= av_get_string(opts_ctx, opt_names[i], &opt, buf, sizeof(buf));
			/* if an option with name opt_names[i] is present in opts_ctx then str is non-NULL */
			if(str && ((opt->flags & flags) == flags))
				av_set_string3(ctx, opt_names[i], str, 1, NULL);
			/* We need to use a differnt system to pass options to the private context because
			it is not known which codec and thus context kind that will be when parsing options
			we thus use opt_values directly instead of opts_ctx */
			if(!str && priv_ctx && av_get_string(priv_ctx, opt_names[i], &opt, buf, sizeof(buf))){
				av_set_string3(priv_ctx, opt_names[i], opt_values[i], 1, NULL);
			}
		}
	}

	int open_input_file(AVFormatContext **fmt_ctx_ptr, const char *filename)
	{
		int err, i;
		AVFormatContext *fmt_ctx;

		fmt_ctx = avformat_alloc_context();
		set_context_opts(fmt_ctx, avformat_opts, AV_OPT_FLAG_DECODING_PARAM, NULL);

		if ((err = av_open_input_file(&fmt_ctx, filename, iformat, 0, NULL)) < 0) {
			print_error(filename, err);
			return err;
		}

		/* fill the streams in the format context */
		if ((err = av_find_stream_info(fmt_ctx)) < 0) {
			print_error(filename, err);
			return err;
		}

		dump_format(fmt_ctx, 0, filename, 0);

		/* bind a decoder to each input stream */
		for (i = 0; i < fmt_ctx->nb_streams; i++) {
			AVStream *stream = fmt_ctx->streams[i];
			AVCodec *codec;

			if (!(codec = avcodec_find_decoder(stream->codec->codec_id))) {
				fprintf(stderr, "Unsupported codec (id=%d) for input stream %d\n",
					stream->codec->codec_id, stream->index);
			} else if (avcodec_open(stream->codec, codec) < 0) {
				fprintf(stderr, "Error while opening codec for input stream %d\n",
					stream->index);
			}
		}

		*fmt_ctx_ptr = fmt_ctx;
		return 0;
	}

	int probe_file(const char *filename)
	{
		AVFormatContext *fmt_ctx;
		int ret, i;

		if(!(pOutFile = fopen(output_filename, "w")))
			return -1;

		if ((ret = open_input_file(&fmt_ctx, filename)))
			return ret;

		if (do_show_packets)
			show_packets(fmt_ctx);

		if (do_show_streams)
			for (i = 0; i < fmt_ctx->nb_streams; i++)
				show_stream(fmt_ctx, i);

		if (do_show_format)
			show_format(fmt_ctx);

		_fclose_nolock(pOutFile);
		av_close_input_file(fmt_ctx);
		return 0;
	}

	void show_usage(void)
	{
		fprintf(pOutFile, "Simple multimedia streams analyzer\n");
		fprintf(pOutFile, "usage: ffprobe [OPTIONS] [INPUT_FILE]\n");
		fprintf(pOutFile, "\n");
	}

	void opt_format(const char *arg)
	{
		iformat = av_find_input_format(arg);
		if (!iformat) {
			fprintf(stderr, "Unknown input format: %s\n", arg);
			__exit(1);
		}
	}

	void opt_input_file(const char *arg)
	{
		if (input_filename) {
			fprintf(stderr, "Argument '%s' provided as input filename, but '%s' was already specified.\n",
				arg, input_filename);
			__exit(1);
		}

		if (!strcmp(arg, "-"))
			arg = "pipe:";

		input_filename = (char*)arg;
	}	

	void opt_pretty(void)
	{
		show_value_unit              = 1;
		use_value_prefix             = 1;
		use_byte_value_binary_prefix = 1;
		use_value_sexagesimal_format = 1;
		do_show_format = 1;
		do_show_streams = 1;
	}

	void __exit(int exitCode)
	{
		ExitThread(exitCode);
	}

public:

	CProbe(void)
	{
		opt_names = NULL;
		opt_values = NULL;
		opt_name_count = 0;
		avformat_opts = NULL;
		sws_opts = NULL;

		program_name = "FFprobe";
		program_birth_year = 2007;

		do_show_format  = 0;
		do_show_packets = 0;
		do_show_streams = 0;

		convert_tags                 = 0;
		show_value_unit              = 0;
		use_value_prefix             = 0;
		use_byte_value_binary_prefix = 0;
		use_value_sexagesimal_format = 0;

		iformat = NULL;

		unit_second_str          = "s"    ;
		unit_hertz_str           = "Hz"   ;
		unit_byte_str            = "byte" ;
		unit_bit_per_second_str  = "bit/s";

		opt_pretty();
	}

	~CProbe(void)
	{
	}

	int Run(char* infile, char* outfile)
	{
		int ret = -1;

		try
		{
			input_filename = infile;
			output_filename = outfile; 
			avformat_opts = avformat_alloc_context();

			ret = probe_file(input_filename);
		}catch(...)
		{
		}

		if(avformat_opts){
			av_free(avformat_opts);
			avformat_opts = NULL;
		}

		return ret;
	}
};
