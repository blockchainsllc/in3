using System;
using System.Runtime.InteropServices;
using System.Text;
using In3.Crypto;
using In3.Error;
using In3.Utils;

namespace In3.Native
{
    internal static class NativeWallet
    {
        public static string Sign(string pk, string data, DigestType digestType)
        {
            string prefixedData = DataTypeConverter.TrimZeroPrefix(data);
            IntPtr dataBytes = Utils.hex_to_new_bytes(prefixedData, prefixedData.Length);
            object dataPtr = Marshal.PtrToStructure(dataBytes, typeof(bytes_t));
            if (dataPtr == null) throw new ContextException("Message to sign is null");
            bytes_t dataStruct = (bytes_t) dataPtr;

            string prefixedPk = DataTypeConverter.TrimZeroPrefix(pk);
            IntPtr pkBytes = Utils.hex_to_new_bytes(prefixedPk, prefixedPk.Length);
            object pkPtr = Marshal.PtrToStructure(pkBytes, typeof(bytes_t));
            if (pkPtr == null) throw new ContextException("Message to sign is null");
            bytes_t pkStruct = (bytes_t) pkPtr;

            bytes_t signature = sign_with_pk(pkStruct.data, dataStruct, digestType);

            byte[] managedSignature = new byte[signature.len];
            Marshal.Copy(signature.data, managedSignature, 0, signature.len);
            string res = DataTypeConverter.BytesToHexString(managedSignature, signature.len);

            Utils._free_(signature.data);
            Utils._free_(dataStruct.data);
            Utils._free_(pkStruct.data);
            Utils._free_(dataBytes);
            Utils._free_(pkBytes);

            return res;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern bytes_t sign_with_pk(IntPtr pk, bytes_t data, DigestType type);
    }
}