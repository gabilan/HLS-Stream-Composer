using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace HlsStreamComposer
{
    public class SegmentationOptions
    {
        public string InputFile { get; set; }
        public int SegmentLength { get; set; }
        public string OutputLocation { get; set; }
        public string FilenamePrefix { get; set; }
        public string PlaylistFilename { get; set; }
        public string ProcessId { get; set; }
        public Delegate CallbackMethod;

        public SegmentationOptions()
        {
            SegmentLength = 10;
            CallbackMethod = new EncodingStatusUpdateDelegate(EncodingProcess.EncodingStatusUpdateCallback);
            FilenamePrefix = "http://www.google.com/hls";
            PlaylistFilename = "playlist.m3u";
        }

        public string[] ToStringArray()
        {
            return new string[]
            {
                InputFile,
                SegmentLength.ToString(),
                OutputLocation,
                FilenamePrefix.EndsWith("/") ? FilenamePrefix : FilenamePrefix + "/",
                PlaylistFilename,
                ProcessId,
                Marshal.GetFunctionPointerForDelegate (CallbackMethod ).ToInt64().ToString()
            };
        }
    }
}
