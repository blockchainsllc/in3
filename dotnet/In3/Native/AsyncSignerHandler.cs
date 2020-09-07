using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using In3.Utils;

namespace In3.Native
{
    internal class AsyncSignerHandler
    {
        private readonly NativeClient _wrapper;

        private GCHandle SignerGcHandle { get; set; }
        private IntPtr SignerPtr { get; set; }

        [StructLayout(LayoutKind.Sequential)]
        private struct bytes_t
        {
            public readonly IntPtr data;
            public readonly int len;
        };

        public AsyncSignerHandler(NativeClient wrapper)
        {
            _wrapper = wrapper;
        }

        public async Task Handle(IntPtr ctx)
        {
            IntPtr signCtx = create_sign_ctx(ctx);

            bytes_t message = in3_sign_ctx_get_message(signCtx);
            bytes_t account = in3_sign_ctx_get_account(signCtx);

            byte[] accountBytes = new byte[account.len];
            Marshal.Copy(account.data, accountBytes, 0, (int)account.len);
            string accountStr = DataTypeConverter.BytesToHexString(accountBytes, (int)account.len);

            byte[] messageBytes = new byte[message.len];
            Marshal.Copy(message.data, messageBytes, 0, (int)message.len);
            string messageStr = DataTypeConverter.BytesToHexString(messageBytes, (int)message.len);

            string signedData = await _wrapper.Client.Signer.Sign(messageStr, accountStr);
            in3_sign_ctx_set_signature_hex(signCtx, signedData);
            in3_sign_ctx_set_signature(ctx, signCtx);
            Utils._free_(signCtx);
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr create_sign_ctx(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_sign_ctx_set_signature(IntPtr ctx, IntPtr signCtx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_sign_ctx_set_signature_hex(IntPtr ctx, String sig);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_message(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_account(IntPtr ctx);
    }
}