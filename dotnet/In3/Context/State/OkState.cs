using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class OkState : IState
    {
        private string _response;

        public async Task PostExecuteAction(Context ctx)
        {
            await Task.Run(() => _response = ctx.GetResponse());
        }

        public bool IsFinal()
        {
            return true;
        }

        public string GetResult()
        {
            return _response;
        }
    }
}