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
            string jsonConfig = in3_get_config(client);
            ClientConfiguration clientConf = JsonSerializer.Deserialize<ClientConfiguration>(jsonConfig);
            return clientConf;
        }

        internal static string SetConfig(IntPtr client, string val)
        {
            return in3_configure(client, val);
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern string in3_get_config(IntPtr client);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern string in3_configure(IntPtr client, string val);
    }
}