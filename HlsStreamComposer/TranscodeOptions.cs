﻿#define HAVE_LIBFAAC

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HlsStreamComposer
{
    public class TranscodeOptions
    {
        public static readonly TranscodeOptions MPEGTS = new TranscodeOptions()
        {
            VerbosityLevel = 0,

#if HAVE_LIBFAAC
            AudioCodec = "libfaac",
#else
            AudioCodec = "aac",
            EarlyExtendedOptions = "-strict experimental",
#endif

            AudioSampleRate = 44100,
            AudioChannels = 2,
            AudioBitrate = "128k",
            VideoCodec = "libx264",
            VideoPreset = "lossless_ultrafast",
            VideoBitrate = "3M",
            LateExtendedOptions = "-r 2997/100 -muxrate 10000000 -threads 0",
            OutputFormat = "mpegts",
            OutputExtension = "mp4"
        };

        public static readonly TranscodeOptions MPEGTSNOVIDEO = new TranscodeOptions()
        {
            VerbosityLevel = 0,

#if HAVE_LIBFAAC
            AudioCodec = "libfaac",
#else
            AudioCodec = "aac",
            EarlyExtendedOptions = "-strict experimental",
#endif

            AudioSampleRate = 44100,
            AudioChannels = 2,
            AudioBitrate = "128k",
            ExcludeVideo = true,
            OutputFormat = "mpegts",
            OutputExtension = "mp4"
        };

        public string InputFile { get; set; }
        public string OutputFile { get; set; }
        public int VerbosityLevel { get; set; }
        public string AudioCodec { get; set; }
        public int AudioSampleRate { get; set; }
        public string AudioBitrate { get; set; }
        public int AudioChannels { get; set; }
        public string VideoCodec { get; set; }
        public string VideoBitrate { get; set; }
        public byte VideoQualityControl { get; set; }
        public string VideoPreset { get; set; }
        public int VideoWidth { get; set; }
        public int VideoHeight { get; set; }
        public float VideoFramerate { get; set; }
        public string OutputFormat { get; set; }
        public string OutputExtension { get; set; }
        public bool ExcludeAudio { get; set; }
        public bool ExcludeVideo { get; set; }
        public string EarlyExtendedOptions { get; set; }
        public string LateExtendedOptions { get; set; }
        public int SecondsToSkip { get; set; }
        public int VideoStreamIndex { get; set; }
        public int AudioStreamIndex { get; set; }

        public TranscodeOptions()
        {
            VideoWidth = -1;
            VideoHeight = -1;
            VideoStreamIndex = -1;
            AudioStreamIndex = -1;
        }

        public TranscodeOptions(TranscodeOptions options)
            : this()
        {
            this.InputFile = options.InputFile;
            this.OutputFile = options.OutputFile;
            this.VerbosityLevel = options.VerbosityLevel;
            this.AudioCodec = options.AudioCodec;
            this.AudioSampleRate = options.AudioSampleRate;
            this.AudioBitrate = options.AudioBitrate;
            this.AudioChannels = options.AudioChannels;
            this.VideoCodec = options.VideoCodec;
            this.VideoBitrate = options.VideoBitrate;
            this.VideoQualityControl = options.VideoQualityControl;
            this.VideoPreset = options.VideoPreset;
            this.VideoWidth = options.VideoWidth;
            this.VideoHeight = options.VideoHeight;
            this.OutputFormat = options.OutputFormat;
            this.OutputExtension = options.OutputExtension;
            this.ExcludeAudio = options.ExcludeAudio;
            this.ExcludeVideo = options.ExcludeVideo;
            this.EarlyExtendedOptions = options.EarlyExtendedOptions;
            this.LateExtendedOptions = options.LateExtendedOptions;
            this.SecondsToSkip = options.SecondsToSkip;
        }

        public override string ToString()
        {
            List<string> parameters = new List<string>();

            if (!string.IsNullOrEmpty(InputFile))
                parameters.Add(string.Format("-i \"{0}\"", InputFile));

            if (VerbosityLevel >= 0)
                parameters.Add(string.Format("-v {0}", VerbosityLevel));

            if (SecondsToSkip > 0)
            {
                TimeSpan skipDuration = TimeSpan.FromSeconds(SecondsToSkip);
                parameters.Add(string.Format("-ss {0}", string.Format("{0:hh\\:mm\\:ss\\.fff}", skipDuration)));
            }

            if (!string.IsNullOrEmpty(EarlyExtendedOptions))
                parameters.Add(EarlyExtendedOptions);

            if (!string.IsNullOrEmpty(OutputFormat))
                parameters.Add(string.Format("-f {0}", OutputFormat));

            if (ExcludeVideo)
                parameters.Add("-vn");
            else
            {
                if (!string.IsNullOrEmpty(VideoCodec))
                    parameters.Add(string.Format("-vcodec {0}", VideoCodec));

                if (VideoFramerate > 0)
                    parameters.Add(string.Format("-r {0}", VideoFramerate.ToString("F")));

                if (!string.IsNullOrEmpty(VideoBitrate))
                    parameters.Add(string.Format("-b {0}", VideoBitrate));

                if (VideoQualityControl > 0)
                    parameters.Add(string.Format("-crf {0}", VideoQualityControl));

                if (!string.IsNullOrEmpty(VideoPreset))
                    parameters.Add(string.Format("-vpre {0}", VideoPreset));

                if ((VideoWidth != -1 || VideoHeight != -1) && (VideoWidth != 0 || VideoHeight != 0))
                    parameters.Add(string.Format("-vf \"scale={0}:{1}\"", VideoWidth, VideoHeight));
            }

            if (ExcludeAudio)
                parameters.Add("-an");
            else
            {
                if (!string.IsNullOrEmpty(AudioCodec))
                    parameters.Add(string.Format("-acodec {0}", AudioCodec));

                if (AudioSampleRate > 0)
                    parameters.Add(string.Format("-ar {0}", AudioSampleRate));

                if (!string.IsNullOrEmpty(AudioBitrate))
                    parameters.Add(string.Format("-ab {0}", AudioBitrate));

                if (AudioChannels > 0)
                    parameters.Add(string.Format("-ac {0}", AudioChannels));
            }

            if (!string.IsNullOrEmpty(LateExtendedOptions))
                parameters.Add(LateExtendedOptions);

            if (VideoStreamIndex != -1)
                parameters.Add(string.Format("-map 0:{0}", VideoStreamIndex));

            if (AudioStreamIndex != -1)
                parameters.Add(string.Format("-map 0:{0}", AudioStreamIndex));

            if (!string.IsNullOrEmpty(OutputFile))
                parameters.Add(string.Format("\"{0}\"", OutputFile));

            return string.Join(" ", parameters.ToArray());
        }

        public string[] ToStringArray()
        {
            List<string> parameters = new List<string>();

            if (!string.IsNullOrEmpty(InputFile))
            {
                parameters.Add("-i");
                parameters.Add(InputFile);
            }

            if (VerbosityLevel >= 0)
            {
                parameters.Add("-v");
                parameters.Add(VerbosityLevel.ToString());
            }

            if (SecondsToSkip > 0)
            {
                TimeSpan skipDuration = TimeSpan.FromSeconds(SecondsToSkip);
                parameters.Add("-ss");
                parameters.Add(string.Format("{0:hh\\:mm\\:ss\\.fff}", skipDuration));
            }

            if (!string.IsNullOrEmpty(EarlyExtendedOptions))
                parameters.AddRange(EarlyExtendedOptions.Split(' '));

            if (!string.IsNullOrEmpty(OutputFormat))
            {
                parameters.Add("-f");
                parameters.Add(OutputFormat);
            }

            if (ExcludeVideo)
                parameters.Add("-vn");
            else
            {
                if (!string.IsNullOrEmpty(VideoCodec))
                {
                    parameters.Add("-vcodec");
                    parameters.Add(VideoCodec);
                }

                if (VideoFramerate > 0)
                {
                    parameters.Add("-r");
                    parameters.Add(VideoFramerate.ToString("F"));
                }

                if (!string.IsNullOrEmpty(VideoBitrate))
                {
                    parameters.Add("-b");
                    parameters.Add(VideoBitrate);
                }

                if (VideoQualityControl > 0)
                {
                    parameters.Add("-crf");
                    parameters.Add(VideoQualityControl.ToString());
                }

                if (!string.IsNullOrEmpty(VideoPreset))
                {
                    parameters.Add("-vpre");
                    parameters.Add(string.Format("{0}", VideoPreset));
                }

                if ((VideoWidth != -1 || VideoHeight != -1) && (VideoWidth != 0 || VideoHeight != 0))
                {
                    parameters.Add("-vf");
                    parameters.Add(string.Format("scale={0}:{1}", VideoWidth, VideoHeight));
                }
            }

            if (ExcludeAudio)
                parameters.Add("-an");
            else
            {
                if (!string.IsNullOrEmpty(AudioCodec))
                {
                    parameters.Add("-acodec");
                    parameters.Add(AudioCodec);
                }

                if (AudioSampleRate > 0)
                {
                    parameters.Add("-ar");
                    parameters.Add(AudioSampleRate.ToString());
                }

                if (!string.IsNullOrEmpty(AudioBitrate))
                {
                    parameters.Add("-ab");
                    parameters.Add(AudioBitrate);
                }

                if (AudioChannels > 0)
                {
                    parameters.Add("-ac");
                    parameters.Add(AudioChannels.ToString());
                }
            }

            if (!string.IsNullOrEmpty(LateExtendedOptions))
                parameters.AddRange(LateExtendedOptions.Split(' '));

            if (VideoStreamIndex != -1)
            {
                parameters.Add("-map");
                parameters.Add(string.Format("0:{0}", VideoStreamIndex));
            }

            if (AudioStreamIndex != -1)
            {
                parameters.Add("-map");
                parameters.Add(string.Format("0:{0}", AudioStreamIndex));
            }

            if (!string.IsNullOrEmpty(OutputFile))
                parameters.Add(string.Format("\"{0}\"", OutputFile));

            return parameters.ToArray();
        }
    }
}
