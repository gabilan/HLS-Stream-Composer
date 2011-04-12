using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace HlsStreamComposer
{
    public class EncodingProcess
    {
        static readonly Dictionary<string, EncodingProcess> ProcessHistory = new Dictionary<string, EncodingProcess>();
        public static EncodingProcess Current;
        public static EncodingProcess GetCurrent()
        {
            return Current;
        }

        public string ProcessId = Guid.NewGuid().ToString();

        string inputPath;
        public string InputPath
        {
            get
            {
                return inputPath;
            }
            set
            {
                if (inputPath != value)
                {
                    inputPath = value;
                    InputFileDurationInSeconds = 0;
                    GetFileDuration();
                }
            }
        }

        public string OutputPath;
        public TranscodeOptions EncodingOptions;
        public int InputFileDurationInSeconds;
        public int SegmentDurationInSeconds = 10;
        public double AmountOfTimeEncodedAndSegmented;

        public EncodingProcess()
        {
            ProcessHistory.Add(this.ProcessId, this);
            Current = this;

            EncodingOptions = TranscodeOptions.MPEGTS;
        }

        public void GetFileDuration()
        {
            if (InputFileDurationInSeconds > 0)
                return;

            string tempFileName = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString() + ".txt");
            string[] args = { InputPath, tempFileName };

            int ret = InteropHelper.Probe(args);
            if (ret == 0)
            {
                Dictionary<string, List<Dictionary<string, string>>> probeInfo = new Dictionary<string, List<Dictionary<string, string>>>();
                string nodeName = null;

                foreach (var line in File.ReadAllLines(tempFileName))
                {
                    if (line[0] == '[')
                    {
                        if (nodeName != null)
                            nodeName = null;
                        else
                        {
                            nodeName = line.Substring(1, line.Length - 2);
                            if (!probeInfo.ContainsKey(nodeName))
                                probeInfo.Add(nodeName, new List<Dictionary<string, string>>());

                            probeInfo[nodeName].Add(new Dictionary<string, string>());
                        }
                    }
                    else
                    {
                        int eqlIdx = -1;
                        eqlIdx = line.IndexOf('=');

                        if (eqlIdx != -1)
                        {
                            string key = null, value = null;

                            key = line.Substring(0, eqlIdx);
                            value = line.Substring(eqlIdx + 1, line.Length - (eqlIdx + 1));

                            probeInfo[nodeName][probeInfo[nodeName].Count - 1].Add(key, value);
                        }
                    }
                }

                TimeSpan ts;
                if (!probeInfo.ContainsKey("FORMAT") || !probeInfo["FORMAT"][0].ContainsKey("duration") || !TimeSpan.TryParse(probeInfo["FORMAT"][0]["duration"], out ts))
                    throw new ApplicationException("Unable to probe input file format and retrieve its duration.");

                this.InputFileDurationInSeconds = (int)ts.TotalSeconds;
            }
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

            if (string.IsNullOrEmpty(EncodingOptions.VideoPreset))
            {
                errorMessage = "No video preset option has been specified.";
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
