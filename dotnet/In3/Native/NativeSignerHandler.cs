using System;
using System.Runtime.InteropServices;
using In3.Utils;

namespace In3.Native
{
    internal class NativeSignerHandler : NativeHandler
    {
        private NativeWrapper Wrapper;

        private GCHandle SignerGcHandle { get; set; }
        private IntPtr SignerPtr { get; set; }

        [StructLayout(LayoutKind.Sequential)]
        private struct bytes_t
        {
            public IntPtr data;
            public int len;
        };

        private unsafe delegate uint sign(void* ctx, uint type, bytes_t content, bytes_t account, IntPtr dst);

        public NativeSignerHandler(NativeWrapper wrapper)
        {
            Wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            unsafe
            {
                sign signDel = Sign;
                SignerGcHandle = GCHandle.Alloc(signDel);
                IntPtr signPtr = Marshal.GetFunctionPointerForDelegate(signDel);
                SignerPtr = in3_set_signer(Wrapper.NativeClientPointer, signPtr.ToPointer(), null, null);
            }
        }

        public void UnregisterNativeHandler()
        {
            NativeUtils._free_(SignerPtr);
            SignerGcHandle.Free();
        }

        private unsafe uint Sign(void* ctx, uint type, [MarshalAs(UnmanagedType.Struct)] bytes_t message, [MarshalAs(UnmanagedType.Struct)] bytes_t account, IntPtr dst)
        {
            byte[] accountBytes = new byte[account.len];
            Marshal.Copy(account.data, accountBytes, 0, account.len);
            string accountStr = TypesMatcher.BytesToHexString(accountBytes, account.len);

            byte[] messageBytes = new byte[message.len];
            Marshal.Copy(message.data, messageBytes, 0, message.len);
            string messageStr = TypesMatcher.BytesToHexString(messageBytes, message.len);

            String signedData = Wrapper.Client.Signer.Sign(messageStr, accountStr);

            NativeUtils.hex_to_bytes(signedData, -1, dst, 65);
            return 0;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern unsafe IntPtr in3_set_signer(IntPtr client, void* sign, void* prepare_tx, void* wallet);
    }
}