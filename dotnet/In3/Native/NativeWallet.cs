using System;
using System.Runtime.InteropServices;
using System.Text;

namespace In3.Native
{
    internal static class NativeWallet
    {
        public static string Sign(string pk, string data)
        {
            byte[] managedArray = new byte[132];
            IntPtr message = eth_wallet_sign(pk, data);
            Marshal.Copy(message, managedArray, 0, 132);
            string res = Encoding.UTF8.GetString(managedArray);
            NativeUtils._free_(message);
            return res;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr eth_wallet_sign(string key, string data);
    }
}