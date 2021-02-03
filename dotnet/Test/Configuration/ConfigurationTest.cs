using System;
using System.Linq;
using System.Text.Json;
using In3;
using In3.Configuration;
using NUnit.Framework;

namespace Test.Configuration
{
    public class ConfigurationTest
    {
        private IN3 _client;

        [SetUp]
        public void CreateClient()
        {
            _client = IN3.ForChain(Chain.Mainnet);
        }

        [Test]
        public void ReadsDefaultConfiguration()
        {
            ClientConfiguration clientConfig = _client.Configuration;
            Assert.IsNotNull(clientConfig.NodeRegistry);
        }

        [Test]
        public void ObjectHierarchy()
        {
            string nodeConfigOutputKey = "nodeRegistry";

            ClientConfiguration clientConfig = _client.Configuration;

            NodeRegistryConfiguration nodeRegistryConfig = clientConfig.NodeRegistry;

            nodeRegistryConfig.NeedsUpdate = false;
            nodeRegistryConfig.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";
            nodeRegistryConfig.Contract = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab";
            nodeRegistryConfig.WhiteListContract = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab";

            string jsonConfiguration1 = clientConfig.ToJson();

            JsonDocument jsonObject1 = JsonDocument.Parse(jsonConfiguration1);
            var propertiesName = jsonObject1.RootElement.EnumerateObject().Select(prop => prop.Name);

            Assert.That(propertiesName.Contains(nodeConfigOutputKey));

            NodeConfiguration nodeListConfig = new NodeConfiguration(nodeRegistryConfig)
            {
                Props = 0x0,
                Url = "scheme://userinfo@host:port/path?query#fragment",
                Address = "0x0"
            };

            string jsonConfiguration2 = clientConfig.ToJson();
            JsonDocument jsonObject2 = JsonDocument.Parse(jsonConfiguration2);

            Assert.DoesNotThrow(() => jsonObject2
                .RootElement
                .GetProperty(nodeConfigOutputKey));
        }

        [Test]
        public void IsSynced()
        {
            ClientConfiguration config = _client.Configuration;
            Assert.That(!config.HasChanged());

            uint requestCount = 1;
            bool autoUpdateList = false;
            Proof proof = Proof.None;
            uint maxAttempts = 1;
            uint signatureCount = 0;

            config.RequestCount = requestCount;
            config.AutoUpdateList = autoUpdateList;
            config.Proof = proof;
            config.MaxAttempts = maxAttempts;
            config.SignatureCount = signatureCount;

            Assert.That(config.HasChanged());
            _client.Eth1.GetChainId();
            Assert.That(!config.HasChanged());
        }

        [Test]
        public void DispatchRequestInvalidConfig()
        {
            ClientConfiguration config = _client.Configuration;

            NodeRegistryConfiguration chainConfig = config.NodeRegistry;

            chainConfig.NeedsUpdate = false;
            chainConfig.RegistryId = "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb";
            chainConfig.Contract = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab";
            chainConfig.WhiteListContract = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab";
            chainConfig.WhiteList = new[]
            {
                "0x0123456789012345678901234567890123456789",
                "0x1234567890123456789012345678901234567890"
            };

            Assert.ThrowsAsync<ArgumentException>(() => _client.Eth1.GetGasPrice());
        }
    }
}