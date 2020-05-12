using System.Text.Json.Serialization;

namespace In3.Eth1
{
    public class TransactionBlock : Block
    {
        [JsonPropertyName("transactions")] public Transaction[] Transactions { get; set; }
    }
}