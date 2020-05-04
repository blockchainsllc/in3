using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class NativeUtils
    {
        // Heavily inspired by: https://stackoverflow.com/questions/1498931/marshalling-array-of-strings-to-char-in-c-sharp
        public static string[] GetAllStrings(IntPtr ptr, int size) {
            string[] list = new string[size];
            for ( int i = 0; i < size; i++ ) {
                var strPtr = (IntPtr)Marshal.PtrToStructure(ptr, typeof(IntPtr));
                list[i] = Marshal.PtrToStringUTF8(strPtr);
                ptr = new IntPtr(ptr.ToInt64()+IntPtr.Size);
            }
            return list;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] public static extern void _free_(IntPtr ptr);

        [DllImport("libin3", CharSet = CharSet.Ansi)] public static extern int hex_to_bytes(string buf, int len, IntPtr dst, int outbuf_size);
    }
}