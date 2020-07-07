using System.Collections.Generic;
using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class StateMachine
    {
        private readonly IState _state;

        public StateMachine(int code)
        {
            Dictionary<ContextState, IState> actions = new Dictionary<ContextState, IState>
            {
                {ContextState.CTX_SUCCESS, new OkState()},
                {ContextState.CTX_WAITING_TO_SEND, new WaitingState()},
                // TODO as long as we fetch all responses (as we do now), CTX_WAITING_FOR_RESPONSE will never happen.
                // so we might be able to skip it for now
//                {ContextState.CTX_WAITING_FOR_RESPONSE, new WaitingState()}, 
                {ContextState.CTX_ERROR, new ErrorState()}
            };

            _state = actions[(ContextState)code];
        }

        public async Task<IState> HandleChange(Context ctx)
        {
            await _state.PostExecuteAction(ctx);
            return _state;
        }
    }
}