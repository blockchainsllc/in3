namespace In3.Ipfs
{
    // This is named IpfsEncoding to avoid name colision (while there is a namespace for that, its just better to avoid trouble for now at least while the class is internal)
    internal class IpfsEncoding
    {
        private IpfsEncoding(string value)
        {
            Value = value;
        }

        public string Value { get; set; }

        public static IpfsEncoding Base64
        {
            get { return new IpfsEncoding("base64"); }
        }

        public static IpfsEncoding Hex
        {
            get { return new IpfsEncoding("hex"); }
        }

        public static IpfsEncoding Utf8
        {
            get { return new IpfsEncoding("utf8"); }
        }
    }
}