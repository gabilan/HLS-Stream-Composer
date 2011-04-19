using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using System.Reflection;

namespace HlsStreamComposer
{
    class InteropHelper
    {
        [DllImport(@"HlsStreamComposer.Native.dll",
            EntryPoint = "Initialize", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int InitializeHelper(string errorPath, string statusPath);

        [DllImport(@"HlsStreamComposer.Native.dll",
            EntryPoint = "RunTranscoder", CallingConvention = CallingConvention.Cdecl)]
        static extern int RunTranscoder(IntPtr message, Int32 length);

        [DllImport(@"HlsStreamComposer.Native.dll",
           EntryPoint = "RunSegmenter", CallingConvention = CallingConvention.Cdecl)]
        static extern int RunSegmenter(IntPtr message, Int32 length);

        [DllImport(@"HlsStreamComposer.Native.dll",
           EntryPoint = "RunProbe", CallingConvention = CallingConvention.Cdecl)]
        static extern int RunProbe(IntPtr message, Int32 length);

        [DllImport(@"HlsStreamComposer.Native.dll",
          EntryPoint = "StopTranscoder", CallingConvention = CallingConvention.Cdecl)]
        public static extern int StopTranscoder();

        static readonly object helperLock = new object();

        public static void Initialize()
        {
            try
            {
                string dirPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string envVariable = Path.Combine(dirPath, "presets");
                Environment.SetEnvironmentVariable("FFMPEG_DATADIR", envVariable);

                int ret = InitializeHelper(
                     Path.Combine(dirPath, "log_helper_error.txt"),
                     Path.Combine(dirPath, "log_helper_out.txt"));
            }
            catch (Exception ex)
            {
                StatusLog.CreateExceptionEntry(ex);
                throw;
            }
        }

        public static int Transcode(string[] arguments)
        {
            int size = 0, ret = -1;

            IntPtr args = RunProcess(arguments, out size);
            ret = RunTranscoder(args, size);
            Marshal.FreeHGlobal(args);

            return ret;
        }

        public static int Segment(string[] arguments)
        {
            int size = 0, ret = -1;

            IntPtr args = RunProcess(arguments, out size);
            ret = RunSegmenter(args, size);
            Marshal.FreeHGlobal(args);

            return ret;
        }

        public static int Probe(string[] arguments)
        {
            int size = 0, ret = -1;

            IntPtr args = RunProcess(arguments, out size);
            ret = RunProbe(args, size);
            Marshal.FreeHGlobal(args);

            return ret;
        }

        static IntPtr RunProcess(string[] args, out int size)
        {
            size = 0;

            try
            {
                using (MemoryStream ms = new MemoryStream())
                {
                    ms.WriteByte((byte)args.Length);

                    var convertedArgs = from x in args
                                        select x;

                    foreach (string s in convertedArgs)
                    {
                        ms.WriteByte((byte)s.Length);
                    }

                    foreach (string s in convertedArgs)
                    {
                        byte[] b = (from x in s.ToCharArray()
                                    select (byte)x).ToArray();

                        ms.Write(b, 0, b.Length);
                    }

                    ms.Position = 0;
                    byte[] buffer = ms.GetBuffer();
                    size = buffer.Length;

                    IntPtr ptr = Marshal.AllocHGlobal(buffer.Length);
                    Marshal.Copy(buffer, 0, ptr, buffer.Length);

                    return ptr;
                }
            }
            catch (Exception ex)
            {
                StatusLog.CreateExceptionEntry(ex);
                return IntPtr.Zero;
            }
        }
    }
}