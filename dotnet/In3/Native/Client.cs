using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using In3.Configuration;

namespace In3.Native
{
    internal class NativeClient
    {
        public IntPtr Pointer;
        public IN3 Client { get; }
        private List<IRegistrableHandler> NativeHandlers { get; } = new List<IRegistrableHandler>();

        public NativeClient(IN3 in3, Chain chainId)
        {
            Client = in3;
            NativeHandlers.Add(new RegistrableStorageHandler(this));

            Pointer = in3_for_chain_auto_init(chainId);
            NativeHandlers.ForEach(handler => handler.RegisterNativeHandler());
        }

        public ClientConfiguration ReadConfiguration()
        {
            return Configuration.Read(Pointer);
        }

        public void ApplyConfiguration(ClientConfiguration configuration)
        {
            string error = Configuration.SetConfig(Pointer, configuration.ToJson());
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
            if (Pointer != IntPtr.Zero)
            {
                in3_free(Pointer);
            }
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_free(IntPtr ptr);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_for_chain_auto_init(Chain chainId);
    }
}