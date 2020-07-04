using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class WaitingState : IState
    {
        public async Task PostExecuteAction(Context ctx)
        {
            Context lastWaiting = ctx.GetLastWaiting();
            if (lastWaiting.IsValid())
            {
                switch (lastWaiting.GetType())
                {
                    case CtxType.Rpc:
                        await lastWaiting.HandleRequest();
                        break;
                    case CtxType.Sign:
                        await lastWaiting.HandleSign();
                        break;
                }
            }
            else
            {
                ctx.ReportError("Could not find the last waiting context");
            }
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
