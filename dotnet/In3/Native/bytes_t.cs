using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    [StructLayout(LayoutKind.Sequential)]
    public struct bytes_t
    {
        public readonly IntPtr data;
        public readonly int len;
    };
}