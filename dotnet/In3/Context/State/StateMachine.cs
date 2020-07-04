using System.Collections.Generic;
using System.Threading.Tasks;

namespace In3.Context.Action
{
    internal class StateMachine
    {
        private readonly IState _state;

        public StateMachine(int code)
        {
            Dictionary<In3Code, IState> actions = new Dictionary<In3Code, IState>
            {
                {In3Code.IN3_OK, new OkState()},
                {In3Code.IN3_WAITING, new WaitingState()},
                {In3Code.IN3_EIGNORE, new IgnoreState()}
            };

            _state = actions.ContainsKey((In3Code) code) ? actions[(In3Code) code] : new ErrorState(); ;
        }

        public async Task<IState> HandleChange(Context ctx)
        {
            await _state.PostExecuteAction(ctx);
            return _state;
        }
    }
}