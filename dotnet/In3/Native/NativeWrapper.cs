using System;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Serialization;
using SystemTextJsonSamples;
using In3.Configuration;
using In3.Rpc;

namespace In3.Native
{
    internal class NativeWrapper
    {
        public IntPtr NativeClientPointer;
        public IN3 Client { get; }

        public NativeWrapper(IN3 in3, Chain chainId)
        {
            Client = in3;
            new NativeTransportHandler(this).RegisterNativeHandler();
            new NativeStorageHandler(this).RegisterNativeHandler();
            NativeClientPointer = in3_for_chain_auto_init(chainId);
        }

        public string Send(string jsonRpcRequest)
        {
            IntPtr res = in3_client_exec_req(NativeClientPointer, jsonRpcRequest);
            string str = Marshal.PtrToStringAuto(res);
            _free_(res);
            return str;

        }

        public ClientConfiguration ReadConfiguration()
        {
            return NativeConfiguration.Read(NativeClientPointer);
        }

        public void ApplyConfiguration(ClientConfiguration configuration)
        {
            string error = NativeConfiguration.SetConfig(NativeClientPointer, configuration.ToJson());
            if (!String.IsNullOrEmpty(error))
            {
                throw new ArgumentException(error, configuration.GetType().Name);
            }
            configuration.MarkSynced();
        }

        ~NativeWrapper() => in3_free(NativeClientPointer);

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_free(IntPtr ptr);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void _free_(IntPtr ptr);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_for_chain_auto_init(Chain chainId);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_client_exec_req(IntPtr ptr, string rpc);
    }
}