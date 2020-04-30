using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class NativeTransportHandler
    {
        private static TransportHandler TransportDel { get; set; }
        private NativeWrapper Wrapper { get; set; }

        // structs
        enum ErrorCode : int
        {
            IN3_OK = 0,
            IN3_ERPC = -11
        }

        [StructLayout(LayoutKind.Sequential)] private ref struct in3_request_t
        {
            [MarshalAs(UnmanagedType.LPStr)] public string payload;
            // This esoteric thing came from here: https://docs.microsoft.com/en-us/dotnet/framework/interop/default-marshaling-for-arrays
            [MarshalAs(UnmanagedType.SysInt)] public IntPtr urls;
            public int urls_len;
            [MarshalAs(UnmanagedType.SysInt)] public IntPtr results;
            public uint timeout;
            public IntPtr times;
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)] private delegate int TransportHandler(ref in3_request_t ptr1);

        public NativeTransportHandler(NativeWrapper wrapper)
        {
            Wrapper = wrapper;
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
                try
                {
                    string result = Wrapper.Client.Transport.Handle(urls[i], req.payload);
                    in3_req_add_response(req.results, i, false, result, result.Length);
                }
                catch (Exception ex)
                {
                    in3_req_add_response(req.results, i, true, ex.Message, ex.Message.Length);
                    err = ErrorCode.IN3_ERPC;
                }
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

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_req_add_response(IntPtr res, int index, bool is_error, string data, int data_len);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_set_default_transport(TransportHandler transport);

    }
}