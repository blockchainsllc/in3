namespace In3.Transport
{
    public interface Transport
    {
        public string Handle(string url, string payload);
    }
}