using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class DefaultNativeWrapper : NativeWrapper
    {
        public IntPtr Ptr;
        public IN3 client { get; }

        public DefaultNativeWrapper(IN3 in3, Chain chainId)
        {
            client = in3;
            new NativeTransportHandler(this).RegisterNativeHandler();
            new NativeStorageHandler(this).RegisterNativeHandler();
            Ptr = in3_for_chain_auto_init(chainId);
        }

        public string Send(string jsonRpcRequest)
        {
            return in3_client_exec_req(Ptr, jsonRpcRequest);
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern int in3_cache_init(IntPtr ptr);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_register_eth_full();
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_register_eth_api();
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_for_chain_auto_init(Chain chainId);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern string in3_client_exec_req(IntPtr ptr, string rpc);
    }
}