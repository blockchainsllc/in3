using System;
using System.Text;
using System.Threading.Tasks;
using In3;
using NUnit.Framework;

namespace Test.Ipfs
{
    public class ApiTest
    {
        private IN3 _in3;

        [SetUp]
        public void Setup()
        {
            string[][] mockedResponses = {
                new[] {"ipfs_get", "ipfs_get.json"},
                new[] {"ipfs_put", "ipfs_put.json"}
            };

            ClientBuilder builder = new ClientBuilder(Chain.Ipfs); ;
            _in3 = builder.ConstructClient(mockedResponses);
        }

        [Test]
        public async Task Put_Success()
        {
            string content = "Lorem ipsum dolor sit amet";

            string multihash = await _in3.Ipfs.Put(content);

            Assert.NotNull(multihash);
            Assert.That(multihash, Is.EqualTo("QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8"));
        }

        [Test]
        public void Put_Failure()
        {
            string nullString = null;

            Assert.ThrowsAsync<ArgumentException>(async () => await _in3.Ipfs.Put(nullString));
        }

        [Test]
        public async Task Get_Success()
        {
            string multihash = "QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8";

            byte[] content = await _in3.Ipfs.Get(multihash);

            Assert.NotNull(content);
            Assert.That(content, Is.EqualTo(Encoding.UTF8.GetBytes("Lorem ipsum dolor sit amet")));
        }

        [Test]
        public void Get_Failure()
        {
            string nullString = null;

            Assert.ThrowsAsync<ArgumentException>(async () => await _in3.Ipfs.Get(nullString));
        }
    }
}