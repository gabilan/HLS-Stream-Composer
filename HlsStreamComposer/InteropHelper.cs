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
        [DllImport(@"Lib\HlsStreamComposer.Native.dll",
            EntryPoint = "Initialize", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern int InitializeHelper(string errorPath, string statusPath);

        [DllImport(@"Lib\HlsStreamComposer.Native.dll",
            EntryPoint = "RunTranscoder", CallingConvention = CallingConvention.Cdecl)]
        static extern int RunTranscoder(IntPtr message, Int32 length);

        [DllImport(@"Lib\HlsStreamComposer.Native.dll",
           EntryPoint = "RunSegmenter", CallingConvention = CallingConvention.Cdecl)]
        static extern int RunSegmenter(IntPtr message, Int32 length);

        static readonly object helperLock = new object();

        public static void Initialize()
        {
            try
            {
                string dirPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string envVariable = Path.Combine(dirPath, "Lib\\presets");
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
    }
}