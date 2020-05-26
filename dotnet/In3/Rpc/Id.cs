namespace In3.Rpc
{
    internal class Id
    {
        private string _value;

        public Id(string id)
        {
            _value = id;
        }

        public Id(long id)
        {
            _value = id.ToString();
        }

        public long AsLong()
        {
            long result;
            long.TryParse(_value, out result);
            return result;
        }

        public string AsString()
        {
            return _value;
        }
    }
}