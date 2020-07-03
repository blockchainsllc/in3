using System.Threading.Tasks;
using In3.Context.Action;
using In3.Native;

namespace In3.Context
{
    internal class Runner
    {
        private readonly NativeClient _wrapper;

        internal Runner(NativeClient wrapper)
        {
            _wrapper = wrapper;
        }

        public Task<string> Run(string rpc)
        {
            return Task.Run(async () =>
            {
                using Context rpcContext = Context.FromRpc(_wrapper, rpc);
                return await ExecuteContextUntilResult(rpcContext);
            });
        }

        private async Task<string> ExecuteContextUntilResult(Context rpcContext)
        {
            while (true)
            {
                IState state = await rpcContext.Execute();
                if (state.IsFinal())
                {
                    return state.GetResult();
                }
            }
        }
    }
}