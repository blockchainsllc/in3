using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal interface IState
    {
        public Task PostExecuteAction(Context ctx);
        public bool IsFinal();
        public string GetResult();
    }
}