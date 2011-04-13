using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;

namespace HlsStreamComposer
{
    public class EncodingProcess
    {
        static readonly Dictionary<string, EncodingProcess> ProcessHistory = new Dictionary<string, EncodingProcess>();
        public static EncodingProcess Current;
        public static EncodingProcess GetCurrent() { return Current; }

        public string ProcessId
        {
            get { return SegmentOptions.ProcessId; }
            set { SegmentOptions.ProcessId = value; }
        }

        public string InputPath
        {
            get
            {
                return EncodingOptions.InputFile;
            }
            set
            {
                if (EncodingOptions.InputFile != value)
                {
                    EncodingOptions.InputFile = value;
                    InputFileDurationInSeconds = 0;
                    GetFileDuration();
                }
            }
        }

        public string OutputPath
        {
            get { return SegmentOptions.OutputLocation; }
            set { SegmentOptions.OutputLocation = value; }
        }

        ThreadStart GenerateHlsStreamThread;
        ThreadStart SegmentHlsStreamThread;
        public TranscodeOptions EncodingOptions { get; set; }
        public SegmentationOptions SegmentOptions { get; set; }
        public int InputFileDurationInSeconds { get; set; }
        public double AmountOfTimeEncodedAndSegmented { get; set; }

        public EncodingProcess()
        {
            EncodingOptions = TranscodeOptions.MPEGTS;
            SegmentOptions = new SegmentationOptions();
            GenerateHlsStreamThread = new ThreadStart(GenerateHlsStream);
            SegmentHlsStreamThread = new ThreadStart(SegmentHlsStream);


            this.ProcessId = Guid.NewGuid().ToString();
            ProcessHistory.Add(this.ProcessId, this);
            Current = this;
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
                    if (string.IsNullOrEmpty(line))
                        continue;

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

            if (SegmentOptions.SegmentLength < 5)
            {
                errorMessage = "The segment duration must be at least 5 seconds.";
                return false;
            }

            if (SegmentOptions.SegmentLength > 30)
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

        public void Start()
        {
            Thread processThread = new Thread(delegate()
            {
                string namedPipe = string.Format("namedp://{0}.ts", ProcessId);
                this.EncodingOptions.OutputFile = namedPipe;
                this.SegmentOptions.InputFile = namedPipe;

                Thread t = new Thread(GenerateHlsStreamThread);
                t.IsBackground = true;
                t.Start();

                Thread t2 = new Thread(SegmentHlsStreamThread);
                t2.Priority = ThreadPriority.AboveNormal;
                t2.IsBackground = true;
                t2.Start();

                t2.Join();
            });

            processThread.IsBackground = true;
            processThread.Start();
            processThread.Join();
        }

        private void GenerateHlsStream()
        {
            var options = EncodingProcess.Current.EncodingOptions;
            InteropHelper.Transcode(options.ToStringArray());
        }

        private void SegmentHlsStream()
        {
            var options = EncodingProcess.Current.SegmentOptions;
            InteropHelper.Segment(options.ToStringArray());
        }

        internal static void EncodingStatusUpdateCallback(string processId, int segmentNumber, double segmentLength)
        {
            if (!ProcessHistory.ContainsKey(processId))
                return;

            EncodingProcess proc = ProcessHistory[processId];
            proc.AmountOfTimeEncodedAndSegmented += segmentLength;
        }
    }

    delegate void EncodingStatusUpdateDelegate(string processId, int segmentNumber, double segmentLength);
}
