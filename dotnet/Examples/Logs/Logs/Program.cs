using System;
using System.Threading;
using System.Threading.Tasks;
using In3;
using In3.Eth1;

namespace Logs
{
    class Program
    {
        static async Task Main()
        {
            // Define an upper limit for poll since we dont want our application potentially running forever.
            int maxIterations = 500;
            int oneSecond = 1000; // in ms

            // Connect to mainnet.
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);

            // Create a filter object pointing, in this case, to an "eventful" contract address.
            LogFilter tetherUsFilter = new LogFilter {Address = "0xdAC17F958D2ee523a2206206994597C13D831ec7"};

            // Create the filter to be polled for logs.
            long filterId = await mainnetClient.Eth1.NewLogFilter(tetherUsFilter);

            // Loop to initiate the poll for the logs.
            for (int i = 0; i < maxIterations; i++)
            {
                // Query for the log events since the creation of the filter or the previous poll (this method in NOT idempotent as it retrieves a diff).
                Log[] tetherLogs = await mainnetClient.Eth1.GetFilterChangesFromLogs(filterId);
                if (tetherLogs.Length > 0)
                {
                    Console.Out.WriteLine("Logs found: " + tetherLogs.Length);
                    break;
                }

                // Wait before next query.
                Thread.Sleep(oneSecond);
            }
        }
    }
}
