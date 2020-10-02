using In3.Error;
using In3.Utils;
using NUnit.Framework;

namespace Test.Utils
{
    public class RpcHandlerTest
    {
        [Test]
        public void From_ErrorObject()
        {
            string nonCompliantJsonRpc =
                "{\"id\":1,\"error\":\"getaddrinfo ENOTFOUND ipfs.iotlayer-deploy\",\"jsonrpc\":\"2.0\"}";
            Assert.Throws<RpcException>(() => RpcHandler.From<string>(nonCompliantJsonRpc));
        }

        [Test]
        public void From_ErrorString()
        {
            string nonCompliantJsonRpc =
                "{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-1,\"message\":\"The Stack is empty\"}}";
            Assert.Throws<RpcException>(() => RpcHandler.From<string>(nonCompliantJsonRpc));
        }

        [Test]
        public void From_String()
        {
            string nonCompliantJsonRpc =
                "{\"jsonrpc\":\"2.0\",\"result\":\"0x7d2b7500\",\"id\":1}";

            string result = RpcHandler.From<string>(nonCompliantJsonRpc);

            Assert.That(result, Is.EqualTo("0x7d2b7500"));
        }
    }
}