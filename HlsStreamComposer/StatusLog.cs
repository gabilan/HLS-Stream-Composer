using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HlsStreamComposer
{
    static class StatusLog
    {
        internal static void CreateExceptionEntry(Exception ex)
        {
            throw ex;
            throw new NotImplementedException();
        }
    }
}
