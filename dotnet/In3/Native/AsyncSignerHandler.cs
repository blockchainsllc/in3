using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using In3.Crypto;
using In3.Utils;

namespace In3.Native
{
    internal class AsyncSignerHandler
    {
        private readonly NativeClient _wrapper;

        private GCHandle SignerGcHandle { get; set; }
        private IntPtr SignerPtr { get; set; }

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
            Marshal.Copy(account.data, accountBytes, 0, account.len);
            string accountStr = DataTypeConverter.BytesToHexString(accountBytes, account.len);

            byte[] messageBytes = new byte[message.len];
            Marshal.Copy(message.data, messageBytes, 0, message.len);
            string messageStr = DataTypeConverter.BytesToHexString(messageBytes, message.len);

            uint digestType = in3_sign_ctx_get_digest_type(ctx);
            uint payloadType = in3_sign_ctx_get_payload_type(ctx);
            uint curveType = in3_sign_ctx_get_curve_type(ctx);

            string signedData = await _wrapper.Client.Signer.Sign(messageStr, accountStr, (DigestType) digestType, (PayloadType) payloadType, (CurveType) curveType);
            in3_sign_ctx_set_signature_hex(signCtx, signedData);
            in3_sign_ctx_set_signature(ctx, signCtx);
            Utils._free_(signCtx);
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr create_sign_ctx(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_sign_ctx_set_signature(IntPtr ctx, IntPtr signCtx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_sign_ctx_set_signature_hex(IntPtr ctx, String sig);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_message(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t in3_sign_ctx_get_account(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern uint in3_sign_ctx_get_digest_type(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern uint in3_sign_ctx_get_payload_type(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern uint in3_sign_ctx_get_curve_type(IntPtr ctx);
    }
}