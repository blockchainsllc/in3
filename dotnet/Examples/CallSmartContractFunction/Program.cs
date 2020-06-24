using System;
using System.Numerics;
using In3;
using In3.Eth1;

namespace CallSmartContractFunction
{
    public class Program
    {
        public static void Main()
        {
            // Set it to mainnet
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            ClientConfiguration cfg = mainnetClient.Configuration;
            cfg.Proof = Proof.None;

            string contractAddress = "0x2736D225f85740f42D17987100dc8d58e9e16252";

            // Create the query transaction
            TransactionRequest serverCountQuery = new TransactionRequest();
            serverCountQuery.To = contractAddress;

            // Define the function and the parameters to query the total in3 servers
            serverCountQuery.Function = "totalServers():uint256";
            serverCountQuery.Params = new object[0];

            string[] serverCountResult = (string[])mainnetClient.Eth1.Call(serverCountQuery, BlockParameter.Latest);
            BigInteger servers = DataTypeConverter.HexStringToBigint(serverCountResult[0]);

            for (int i = 0; i < servers; i++)
            {
                TransactionRequest serverDetailQuery = new TransactionRequest();
                serverDetailQuery.To = contractAddress;

                // Define the function and the parameters to query the in3 servers detail
                serverDetailQuery.Function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";
                serverDetailQuery.Params = new object[] { i }; // index of the server (uint256) as per solidity function signature

                string[] serverDetailResult = (string[])mainnetClient.Eth1.Call(serverDetailQuery, BlockParameter.Latest);
                Console.Out.WriteLine($"Server url: {serverDetailResult[0]}");
            }
        }
    }