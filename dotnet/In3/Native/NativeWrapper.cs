using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using In3.Configuration;

namespace In3.Native
{
    internal class NativeWrapper
    {
        public IntPtr NativeClientPointer;
        public IN3 Client { get; }
        private List<NativeHandler> NativeHandlers { get; } = new List<NativeHandler>();

        public NativeWrapper(IN3 in3, Chain chainId)
        {
            Client = in3;
            NativeHandlers.Add(new NativeTransportHandler(this));
            NativeHandlers.Add(new NativeSignerHandler(this));
            NativeHandlers.Add(new NativeStorageHandler(this));

            NativeClientPointer = in3_for_chain_auto_init(chainId);
            NativeHandlers.ForEach(handler => handler.RegisterNativeHandler());
        }

        public string Send(string jsonRpcRequest)
        {
            IntPtr res = in3_client_exec_req(NativeClientPointer, jsonRpcRequest);
            string str = Marshal.PtrToStringAnsi(res);
            NativeUtils._free_(res);
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

        public void Free()
        {
            NativeHandlers.ForEach(handler => handler.UnregisterNativeHandler());
            NativeHandlers.Clear();
            if (NativeClientPointer != IntPtr.Zero)
            {
                in3_free(NativeClientPointer);
            }
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_free(IntPtr ptr);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_for_chain_auto_init(Chain chainId);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_client_exec_req(IntPtr ptr, string rpc);
    }
}