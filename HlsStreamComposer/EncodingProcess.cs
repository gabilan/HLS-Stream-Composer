using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace HlsStreamComposer
{
    class EncodingProcess
    {
        static readonly Dictionary<string, EncodingProcess> ProcessHistory = new Dictionary<string, EncodingProcess>();

        public string ProcessId = Guid.NewGuid().ToString();
        public string InputPath;
        public string OutputPath;
        public TranscodeOptions EncodingOptions;
        public int InputFileDurationInSeconds;
        public int SegmentDurationInSeconds;
        public double AmountOfTimeEncodedAndSegmented;

        public EncodingProcess()
        {
            ProcessHistory.Add(this.ProcessId, this);
            EncodingOptions = TranscodeOptions.MPEGTS;
        }

        public bool ReadyForEncode(ref  string errorMessage)
        {
            if (!File.Exists(InputPath))
            {
                errorMessage = "The specified input file does not exist.";
                return false;
            }

            if (!Directory.Exists(OutputPath))
            {
                errorMessage = "The specified output directory does not exist.";
                return false;
            }

            if (InputFileDurationInSeconds == 0)
            {
                errorMessage = "Unable to determine the length of the input file.";
                return false;
            }

            if (SegmentDurationInSeconds < 5)
            {
                errorMessage = "The segment duration must be at least 5 seconds.";
                return false;
            }

            if (SegmentDurationInSeconds > 30)
            {
                errorMessage = "The segment duration must be less than 30 seconds.";
                return false;
            }

            return true;
        }

        static void EncodingStatusUpdateCallback(string processId, int segmentNumber, double segmentLength)
        {
            if (!ProcessHistory.ContainsKey(processId))
                return;

            EncodingProcess proc = ProcessHistory[processId];
            proc.AmountOfTimeEncodedAndSegmented += segmentLength;
        }
    }
}
