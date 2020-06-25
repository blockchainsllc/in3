using System;
using System.Threading;
using In3;
using In3.Crypto;
using In3.Eth1;

namespace SendTransaction
{
    public class Program
    {
        static void Main()
        {
            IN3 goerliClient = IN3.ForChain(Chain.Goerli);

            string myPrivateKey = "0x0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
            string receivingAddress = "0x6FA33809667A99A805b610C49EE2042863b1bb83";

            // Get the wallet, which is the default signer.
            SimpleWallet myAccountWallet = (SimpleWallet)goerliClient.Signer;

            string myAccount = myAccountWallet.AddRawKey(myPrivateKey);

            // Create the transaction request
            TransactionRequest transferWei = new TransactionRequest();
            transferWei.To = receivingAddress;
            transferWei.From = myAccount;
            transferWei.Value = 300;

            // Get the current gas prices
            long currentGasPrice = goerliClient.Eth1.GetGasPrice();
            transferWei.GasPrice = currentGasPrice;

            long estimatedSpentGas = goerliClient.Eth1.EstimateGas(transferWei, BlockParameter.Latest);
            Console.Out.WriteLine($"Estimated gas to spend: {estimatedSpentGas}");

            string transactionHash = goerliClient.Eth1.SendTransaction(transferWei);
            Console.Out.WriteLine($"Transaction {transactionHash} sent.");
            Thread.Sleep(30000);

            TransactionReceipt receipt = goerliClient.Eth1.GetTransactionReceipt(transactionHash);
            Console.Out.WriteLine($"Transaction {transactionHash} mined on block {receipt.BlockNumber}.");
        }
    }
}