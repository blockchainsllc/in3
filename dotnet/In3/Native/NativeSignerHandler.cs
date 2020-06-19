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
            public uint len;
        };

        private unsafe delegate uint in3_sign(IntPtr in3_sign_ctx);

        public NativeSignerHandler(NativeWrapper wrapper)
        {
            Wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            unsafe
            {
                in3_sign signDel = Sign;
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

        private uint Sign(IntPtr in3SignCtx)
        {
            IntPtr dst = in3_sign_ctx_get_signature(in3SignCtx);

            bytes_t message = in3_sign_ctx_get_message(in3SignCtx);
            bytes_t account = in3_sign_ctx_get_account(in3SignCtx);

            byte[] accountBytes = new byte[account.len];
            Marshal.Copy(account.data, accountBytes, 0, (int) account.len);
            string accountStr = DataTypeConverter.BytesToHexString(accountBytes, (int) account.len);

            byte[] messageBytes = new byte[message.len];
            Marshal.Copy(message.data, messageBytes, 0, (int) message.len);
            string messageStr = DataTypeConverter.BytesToHexString(messageBytes, (int) message.len);

            String signedData = Wrapper.Client.Signer.Sign(messageStr, accountStr);

            NativeUtils.hex_to_bytes(signedData, -1, dst, 65);
            return 0;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_sign_ctx_get_signature(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_message(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_account(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern unsafe IntPtr in3_set_signer(IntPtr client, void* sign, void* prepare_tx, void* wallet);
    }
}