using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class NativeTransportHandler : NativeHandler
    {
        private NativeWrapper Wrapper { get; set; }

        private GCHandle TransportGcHandle { get; set; }
        private IntPtr TransportPtr { get; set; }

        // structs
        enum ErrorCode : int
        {
            IN3_OK = 0,
            IN3_ERPC = -11
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)] private delegate int TransportHandler(IntPtr ptr1);

        public NativeTransportHandler(NativeWrapper wrapper)
        {
            Wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            /*
             * The code bellow is a combionation of three solutions:
             * https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/367eeye0(v=vs.110)?redirectedfrom=MSDN
             * https://docs.microsoft.com/en-us/previous-versions/dotnet/netframework-4.0/44ey4b32(v=vs.100)
             * https://docs.microsoft.com/en-us/dotnet/framework/interop/marshaling-a-delegate-as-a-callback-method
             * The same logic SHOULD be applied to any function pointer stored in a struct (aka whose lifecycle goes beyond the higher order function invocation). In this context they are: transport, storage and signer.
             */
            unsafe
            {
                TransportHandler transportHandler = HandleRequest;
                TransportGcHandle = GCHandle.Alloc(transportHandler);
                IntPtr transportFunctionPtr = Marshal.GetFunctionPointerForDelegate(transportHandler);
                in3_set_transport(Wrapper.NativeClientPointer, transportFunctionPtr.ToPointer());
            }
        }

        public void UnregisterNativeHandler()
        {
            TransportGcHandle.Free();
        }

        private int HandleRequest(IntPtr reqPtr)
        {
            ErrorCode err = ErrorCode.IN3_OK;
            int urlsLength = in3_get_request_urls_len(reqPtr);
            string[] urls = NativeUtils.GetAllStrings(in3_get_request_urls(reqPtr), urlsLength);
            // This is marshaled in a non-declarative way to prevent double freeing of the string (sometimes necessary when the string is declared as a char*). 
            string payload = Marshal.PtrToStringAnsi(in3_get_request_payload(reqPtr));

            for (int i = 0; i < urlsLength; i++)
            {
                try
                {
                    string result = Wrapper.Client.Transport.Handle(urls[i], payload);
                    in3_req_add_response(reqPtr, i, false, result, result.Length);
                }
                catch (Exception ex)
                {
                    in3_req_add_response(reqPtr, i, true, ex.Message, ex.Message.Length);
                    err = ErrorCode.IN3_ERPC;
                }
            }
            return (int)err;
        }
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_get_request_payload(IntPtr request);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_get_request_urls(IntPtr request);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern int in3_get_request_urls_len(IntPtr request);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_req_add_response(IntPtr req, int index, bool is_error, string data, int data_len);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern unsafe void in3_set_transport(IntPtr client, void* transport);
    }
}