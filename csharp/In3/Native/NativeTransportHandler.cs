using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class NativeTransportHandler
    {
        private static TransportHandler TransportDel { get; set; }
        private DefaultNativeWrapper wrapper { get; set; }

        // structs
        enum ErrorCode : int
        {
            IN3_OK = 0,
            IN3_ERPC = -11
        }
        [StructLayout(LayoutKind.Sequential)] private struct sb_t
        {
            [MarshalAs(UnmanagedType.LPStr)] private string data;
            private long allocated;
            private long len;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct in3_response_t
        {
            [MarshalAs(UnmanagedType.Struct)] public sb_t error;
            [MarshalAs(UnmanagedType.Struct)] public sb_t result;
        }

        [StructLayout(LayoutKind.Sequential)] private ref struct in3_request_t
        {
            [MarshalAs(UnmanagedType.LPStr)] public string payload;
            // This esoteric thing came from here: https://docs.microsoft.com/en-us/dotnet/framework/interop/default-marshaling-for-arrays
            [MarshalAs(UnmanagedType.SysInt)] public IntPtr urls;
            public int urls_len;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.SysInt)] public IntPtr[] results;
            public uint timeout;
            public IntPtr times;
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)] private delegate int TransportHandler(ref in3_request_t ptr1);

        public NativeTransportHandler(DefaultNativeWrapper wrapper)
        {
            this.wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            TransportDel = HandleRequest;
            in3_set_default_transport(TransportDel);
        }

        private int HandleRequest(ref in3_request_t req)
        {
            ErrorCode err = ErrorCode.IN3_OK;
            string[] urls = GetAllStrings(req.urls, req.urls_len);
            for (int i = 0; i < req.urls_len; i++)
            {
                in3_response_t rsp = Marshal.PtrToStructure<in3_response_t>(req.results[i]);
                try
                {
                    string result = wrapper.client.Transport.Handle(urls[i], req.payload);
                    sb_add_chars(ref rsp.result, result);
                }
                catch (Exception ex)
                {
                    sb_add_chars(ref rsp.error, ex.Message);
                    err = ErrorCode.IN3_ERPC;
                }
                Marshal.StructureToPtr(rsp, req.results[i], false);
            }
            return (int) err;
        }

        // Heavily inspired by: https://stackoverflow.com/questions/1498931/marshalling-array-of-strings-to-char-in-c-sharp
        private string[] GetAllStrings(IntPtr ptr, int size) {
            string[] list = new string[size];
            for ( int i = 0; i < size; i++ ) {
                var strPtr = (IntPtr)Marshal.PtrToStructure(ptr, typeof(IntPtr));
                list[i] = Marshal.PtrToStringUTF8(strPtr);
                ptr = new IntPtr(ptr.ToInt64()+IntPtr.Size);
            }
            return list;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr sb_add_chars(ref sb_t sbt, string chars);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_set_default_transport(TransportHandler transport);

    }
}