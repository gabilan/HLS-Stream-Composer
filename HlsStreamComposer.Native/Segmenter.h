#pragma once

extern "C"{
#include "libavformat/avformat.h"
}

struct config_info
{
	char input_filename[1024];
	int segment_length;
	char temp_directory[1024];
	char filename_prefix[1024];
	int start_index;
	double start_time;
	char segmentIndexFileLocation[1024];
};

struct file_index_update_context
{
	void*	thisPtr;
	char	old_filename[1024];
	char	new_filename[1024];
	char	index_entry[2048];
};


DWORD __stdcall RenameFileAndUpdateIndex(LPVOID p);

class CSegmenter
{
public:
	FILE* segmentIndexFile;

private:	

	AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream) 
	{
		AVCodecContext *input_codec_context;
		AVCodecContext *output_codec_context;
		AVStream *output_stream;

		output_stream = av_new_stream(output_format_context, 0);
		if (!output_stream) 
		{
			fprintf(stderr, "Segmenter error: Could not allocate stream\n");
			__exit(1);
		}

		input_codec_context = input_stream->codec;
		output_codec_context = output_stream->codec;

		output_codec_context->codec_id = input_codec_context->codec_id;
		output_codec_context->codec_type = input_codec_context->codec_type;
		output_codec_context->codec_tag = input_codec_context->codec_tag;
		output_codec_context->bit_rate = input_codec_context->bit_rate;
		output_codec_context->extradata = input_codec_context->extradata;
		output_codec_context->extradata_size = input_codec_context->extradata_size;

		if(av_q2d(input_codec_context->time_base) * input_codec_context->ticks_per_frame > av_q2d(input_stream->time_base) && av_q2d(input_stream->time_base) < 1.0/1000) 
		{
			output_codec_context->time_base = input_codec_context->time_base;
			output_codec_context->time_base.num *= input_codec_context->ticks_per_frame;
		}
		else 
		{
			output_codec_context->time_base = input_stream->time_base;
		}

		switch (input_codec_context->codec_type) 
		{
		case CODEC_TYPE_AUDIO:
			output_codec_context->channel_layout = input_codec_context->channel_layout;
			output_codec_context->sample_rate = input_codec_context->sample_rate;
			output_codec_context->channels = input_codec_context->channels;
			output_codec_context->frame_size = input_codec_context->frame_size;
			if ((input_codec_context->block_align == 1 && input_codec_context->codec_id == CODEC_ID_MP3) || input_codec_context->codec_id == CODEC_ID_AC3) 
			{
				output_codec_context->block_align = 0;
			}
			else 
			{
				output_codec_context->block_align = input_codec_context->block_align;
			}
			break;
		case CODEC_TYPE_VIDEO:
			output_codec_context->pix_fmt = input_codec_context->pix_fmt;
			output_codec_context->width = input_codec_context->width;
			output_codec_context->height = input_codec_context->height;
			output_codec_context->has_b_frames = input_codec_context->has_b_frames;

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) 
			{
				output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			break;
		default:
			break;
		}

		return output_stream;
	}

	void __exit(int exitCode)
	{
		if(segmentIndexFile)
		{
			_fclose_nolock(segmentIndexFile);
			segmentIndexFile = NULL;
		}

		ExitThread(exitCode);
	}	

public:

	CSegmenter(void)
	{
		segmentIndexFile = NULL;
	}

	~CSegmenter(void)
	{
		if(segmentIndexFile)
		{
			_fclose_nolock(segmentIndexFile);
			segmentIndexFile = NULL;
		}
	}

	int Run(int argc, char** argv)
	{
		try{
			if(argc < 4)
			{
				fprintf(stderr, "Usage: %s <input filename> <segment length> <output location> <filename prefix> <start segment> <start time>\n", "segmenter.exe");
				return 1;
			}

			struct config_info config;

			memset(&config, 0, sizeof(struct config_info));

			strcpy(config.input_filename, argv[0]);
			config.segment_length = atoi(argv[1]); 
			strcpy(config.temp_directory, argv[2]);
			strcpy(config.filename_prefix, argv[3]);

			if(argc > 5)
			{
				config.start_index = atoi(argv[4]);
				config.start_time = atof(argv[5]);
			}
			else
			{
				config.start_index = 0;
				config.start_time = 0;
			}

			//setup the segment index file
			snprintf(config.segmentIndexFileLocation, strlen(config.temp_directory) + 10, "%s\\%s", config.temp_directory, "index.xml");		

			char output_filename[1024];
			char new_output_filename[1024];			

			segmentIndexFile = fopen(config.segmentIndexFileLocation, config.start_index ? "a" : "w");
			if(segmentIndexFile == NULL)
			{
				fprintf(stderr, "Segmenter error: Could not find open index file for writing\n");
				__exit(1);
			}

			AVInputFormat *input_format = av_find_input_format("mpegts");
			if (!input_format) 
			{
				fprintf(stderr, "Segmenter error: Could not find MPEG-TS demuxer\n");
				__exit(1);
			}

			AVFormatContext *input_context = NULL;
			int ret = av_open_input_file(&input_context, config.input_filename, input_format, 0, NULL);
			if (ret != 0) 
			{
				fprintf(stderr, "Segmenter error: Could not open input file '%s', make sure it is an mpegts file: %d\n", config.input_filename, ret);
				__exit(1);
			}

			if (av_find_stream_info(input_context) < 0) 
			{
				fprintf(stderr, "Segmenter error: Could not read stream information\n");
				__exit(1);
			}

#if LIBAVFORMAT_VERSION_MAJOR >= 52 && LIBAVFORMAT_VERSION_MINOR >= 45
			AVOutputFormat *output_format = av_guess_format("mpegts", NULL, NULL);
#else
			AVOutputFormat *output_format = guess_format("mpegts", NULL, NULL);
#endif
			if (!output_format) 
			{
				fprintf(stderr, "Segmenter error: Could not find MPEG-TS muxer\n");
				__exit(1);
			}

			AVFormatContext *output_context = avformat_alloc_context();
			if (!output_context) 
			{
				fprintf(stderr, "Segmenter error: Could not allocated output context");
				__exit(1);
			}
			output_context->oformat = output_format;

			int video_index = -1;
			int audio_index = -1;

			AVStream *video_stream;
			AVStream *audio_stream;

			int i;

			for (i = 0; i < input_context->nb_streams && (video_index < 0 || audio_index < 0); i++) 
			{
				switch (input_context->streams[i]->codec->codec_type) {
				case CODEC_TYPE_VIDEO:
					video_index = i;
					input_context->streams[i]->discard = AVDISCARD_NONE;
					video_stream = add_output_stream(output_context, input_context->streams[i]);
					break;
				case CODEC_TYPE_AUDIO:
					audio_index = i;
					input_context->streams[i]->discard = AVDISCARD_NONE;
					audio_stream = add_output_stream(output_context, input_context->streams[i]);
					break;
				default:
					input_context->streams[i]->discard = AVDISCARD_ALL;
					break;
				}
			}

			if (av_set_parameters(output_context, NULL) < 0) 
			{
				fprintf(stderr, "Segmenter error: Invalid output format parameters\n");
				__exit(1);
			}

			dump_format(output_context, 0, config.filename_prefix, 1);

			AVCodec *codec = NULL;
			AVCodecContext *codecCtx = NULL;
			if(video_index >= 0)
			{
				codec = avcodec_find_decoder(video_stream->codec->codec_id);
				if (!codec) 
				{
					fprintf(stderr, "Segmenter error: Could not find video decoder, key frames will not be honored\n");
				}

				if (avcodec_open(video_stream->codec, codec) < 0) 
				{
					fprintf(stderr, "Segmenter error: Could not open video decoder, key frames will not be honored\n");
				}

				codecCtx = video_stream->codec;
			}

			unsigned int output_index = config.start_index ? config.start_time ? 99999 : config.start_index  + 1 : 1;

			snprintf(output_filename, strlen(config.temp_directory) + 1 + strlen(config.filename_prefix) + 11, "%s\\%s-%05u.ts_", config.temp_directory, config.filename_prefix, output_index);
			snprintf(new_output_filename, strlen(config.temp_directory) + 1 + strlen(config.filename_prefix) + 10, "%s\\%s-%05u.ts", config.temp_directory, config.filename_prefix, output_index++);

			if (url_fopen(&output_context->pb, output_filename, URL_WRONLY) < 0) 
			{
				fprintf(stderr, "Segmenter error: Could not open '%s'\n", output_filename);
				__exit(1);
			}

			if (av_write_header(output_context)) 
			{
				fprintf(stderr, "Segmenter error: Could not write mpegts header to first output file\n");
				__exit(1);
			}

			unsigned int first_segment = 1;
			unsigned int last_segment = 0;
			unsigned int videoFrameCount = 0;
			int64_t lastValidPts = AV_NOPTS_VALUE;
			int lastDuration = 0;
			unsigned int first_video_packet = 1;
			AVFrac old_video_pts;

			double prev_segment_time = 0;
			int decode_done;

			do 
			{
				double segment_time = 0;
				AVPacket packet;

				decode_done = av_read_frame(input_context, &packet);
				if (decode_done < 0) 
				{
					break;
				}

				if (av_dup_packet(&packet) < 0) 
				{
					fprintf(stderr, "Segmenter error: Could not duplicate packet");
					av_free_packet(&packet);
					break;
				}

				if (packet.stream_index == video_index)
				{
					if((packet.flags & AV_PKT_FLAG_KEY)) 
					{				
						if(video_stream->pts.val)
							segment_time = (double)video_stream->pts.val * video_stream->time_base.num / video_stream->time_base.den;			
					}
					else
					{
						segment_time = prev_segment_time;
					}

					if(packet.pts == AV_NOPTS_VALUE)
					{
						if(packet.duration > 0)
						{
							lastDuration = packet.duration;
							lastValidPts = packet.pts;
						}
					}
					else
					{
						lastDuration = packet.duration;
						lastValidPts = packet.pts;
					}

					first_video_packet = 0;
					++videoFrameCount;
				}
				else if (video_index < 0) 
				{
					segment_time = (double)audio_stream->pts.val * audio_stream->time_base.num / audio_stream->time_base.den;
				}
				else 
				{
					segment_time = prev_segment_time;
				}

				//we need to skip a certain amount of time before creating the segment files
				if(config.start_time)
				{
					//are we lower than skip time
					if(segment_time < config.start_time)
					{
						av_interleaved_write_frame(output_context, &packet);
						goto __free_av_packet; //just free the packet
					}
					else
					{
						config.start_time = 0; // set the skip time to zero; write packet and proceed normally
						output_index = config.start_index - 1;
					}
				}

				// done writing the current file?
				if ((int)(segment_time - prev_segment_time) >= config.segment_length) 
				{
					old_video_pts = video_stream->pts;

					put_flush_packet(output_context->pb);
					//av_write_trailer(output_context);
					url_fclose(output_context->pb);

					{
						file_index_update_context* updateCtx = (file_index_update_context*)malloc(sizeof(file_index_update_context));
						memset(updateCtx, 0, sizeof(file_index_update_context));
						updateCtx->thisPtr = this;

						memcpy(updateCtx->old_filename, output_filename, 1024);
						memcpy(updateCtx->new_filename, new_output_filename, 1024);

						snprintf(updateCtx->index_entry, 1024, "<segment index=\"%d\" file=\"%s\" duration=\"%f\" />\n", output_index - 1, output_filename, (float)(segment_time - prev_segment_time));
						CreateThread(NULL, 0, RenameFileAndUpdateIndex, updateCtx, 0, NULL);
					}

					snprintf(output_filename, strlen(config.temp_directory) + 1 + strlen(config.filename_prefix) + 11, "%s\\%s-%05u.ts_", config.temp_directory, config.filename_prefix, output_index);
					snprintf(new_output_filename, strlen(config.temp_directory) + 1 + strlen(config.filename_prefix) + 10, "%s\\%s-%05u.ts", config.temp_directory, config.filename_prefix, output_index++);
					if (url_fopen(&output_context->pb, output_filename, URL_WRONLY) < 0) 
					{
						fprintf(stderr, "Segmenter error: Could not open '%s'\n", output_filename);
						break;
					}
					/*else if (av_write_header(output_context)) 
					{
					fprintf(stderr, "Segmenter error: Could not write mpegts header to output file\n");
					__exit(1);
					}*/		

					prev_segment_time = segment_time;
				}				

				ret = av_interleaved_write_frame(output_context, &packet);		
				if (ret < 0) 
				{
					fprintf(stderr, "Segmenter error: Could not write frame of stream: %d\n", ret);
				}
				else if (ret > 0) 
				{
					fprintf(stderr, "Segmenter info: End of stream requested\n");
					av_free_packet(&packet);
					break;
				}	

__free_av_packet:
				av_free_packet(&packet);
			} while (!decode_done);

			av_write_trailer(output_context);		

			if (video_index >= 0) 
			{
				avcodec_close(video_stream->codec);
			}

			for(i = 0; i < output_context->nb_streams; i++) 
			{
				av_freep(&output_context->streams[i]->codec);
				av_freep(&output_context->streams[i]);
			}

			url_fclose(output_context->pb);
			av_free(output_context);	
			rename(output_filename, new_output_filename);
		}catch(...){
		}

		return 0;
	}
};

DWORD __stdcall RenameFileAndUpdateIndex(LPVOID p)
{
	file_index_update_context* ctx = (file_index_update_context*)p;
	CSegmenter* c = NULL;
	if(!ctx)
		return -1;
	else
		c = (CSegmenter*)ctx->thisPtr;

	if(!c)
		return -1;

	//update the index
	if(c->segmentIndexFile){
		fprintf(c->segmentIndexFile, ctx->index_entry);
		fflush(c->segmentIndexFile);
	}

	//rename the file
	rename(ctx->old_filename, ctx->new_filename);

	return 0;
}