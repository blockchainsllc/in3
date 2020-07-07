using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class ErrorState : IState
    {
        private string _errorMessage;
        public async Task PostExecuteAction(Context ctx)
        {
            // Unfortunately needs to have this keep the interface consistent
            await Task.Run(() => _errorMessage = ctx.GetErrorMessage());
        }

        public bool IsFinal()
        {
            return true;
        }

        public string GetResult()
        {
            return _errorMessage;
        }
    }
}