using System.Text.Json.Serialization;

namespace In3.Eth1
{
    public class TransactionHashBlock: Block
    {
        [JsonPropertyName("transactions")] public string[] Transactions { get; set; }
    }
}