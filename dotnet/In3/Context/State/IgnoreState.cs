using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class IgnoreState : IState
    {
        public async Task PostExecuteAction(Context ctx)
        {
            Context lastIgnored = ctx.GetLastIgnored();
            if (!lastIgnored.IsValid())
            {
                ctx.ReportError("Cound not find the last waiting context");
            }

            await Task.Run(() => lastIgnored.HandleFailable());
        }

        public bool IsFinal()
        {
            return false;
        }

        public string GetResult()
        {
            throw new System.InvalidOperationException("Cannot GetResult in a non-final state.");
        }
    }
}