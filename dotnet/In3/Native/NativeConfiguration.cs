using System;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text.Json;
using In3.Configuration;
using In3.Utils;

namespace In3.Native
{
    internal class NativeConfiguration
    {
        public static ClientConfiguration Read(IntPtr client)
        {
            IntPtr jsonPointer = in3_get_config(client);
            string jsonConfig = Marshal.PtrToStringUTF8(jsonPointer);
            Marshal.FreeHGlobal(jsonPointer);
                
            ClientConfiguration clientConf = JsonSerializer.Deserialize<ClientConfiguration>(jsonConfig);
            clientConf.MarkSynced();
            return clientConf;
        }

        internal static string SetConfig(IntPtr client, string val)
        {
            IntPtr jsonPointer = in3_configure(client, val);
            if (jsonPointer != IntPtr.Zero)
            {
                string error = Marshal.PtrToStringUTF8(jsonPointer);
                Marshal.FreeHGlobal(jsonPointer);
                return error;
            }

            return null;
        }

        // Why this is an IntPtr and not a string: https://stackoverflow.com/questions/7322503/pinvoke-how-to-free-a-mallocd-string
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_get_config(IntPtr client);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_configure(IntPtr client, string val);
    }
}