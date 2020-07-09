using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using In3.Context.Action;
using In3.Exceptions;
using In3.Native;

namespace In3.Context
{
    /// <summary>
    /// Acts as the main orchestrator for the execution of an rpc. Holds a reference to the native context (ctx) and wraps behavior around it.
    /// </summary>
    internal class Context : IDisposable
    {
        private readonly IntPtr _nativeCtx;
        private readonly NativeClient _wrapper;

        /// <summary>
        /// Standard constructor, private so people use <see cref="FromRpc"/>.
        /// </summary>
        /// <param name="ctx">The native rpc context.</param>
        /// <param name="nativeClient">Object that encapsulates the native client.</param>
        private Context(IntPtr ctx, NativeClient nativeClient)
        {
            _nativeCtx = ctx;
            _wrapper = nativeClient;
        }
        /// <summary>
        /// Factory-like method to build a Context object from an rpc request.
        /// </summary>
        /// <param name="wrapper">The object that encapsulates the native client pointer.</param>
        /// <param name="rpc">The rpc request</param>
        /// <returns>An instance of context.</returns>
        public static Context FromRpc(NativeClient wrapper, string rpc)
        {
            IntPtr ctx = CreateNativeCtx(wrapper.Pointer, rpc);
            return new Context(ctx, wrapper);
        }

        /// <summary>
        /// Method to manage the creation of the native ctx request.
        /// </summary>
        /// <param name="nativeIn3Ptr">Native client pointer.</param>
        /// <param name="rpc">The rpc request</param>
        /// <returns>Native rpc pointer</returns>
        /// <exception cref="RpcException"></exception>
        private static IntPtr CreateNativeCtx(IntPtr nativeIn3Ptr, string rpc)
        {
            IntPtr rpcPtr = Marshal.StringToHGlobalAnsi(rpc);
            IntPtr context = ctx_new(nativeIn3Ptr, rpcPtr);
            string err = ctx_get_error_data(context);
            if (!String.IsNullOrEmpty(err))
            {
                ctx_free(context);
                throw new RpcException(err);
            }
            return context;
        }

        /// <summary>
        /// Proxy to in3_ctx_execute, every invocation generates a new state.
        /// </summary>
        /// <returns>The state as computed by in3_ctx_execute.</returns>
        public async Task<IState> Execute()
        {
            return await new StateMachine(in3_ctx_exec_state(this._nativeCtx))
                .HandleChange(this);
        }

        /// <summary>
        /// Method responsible to fetch the pending context references in the current context.
        /// </summary>
        /// <returns>A context object.</returns>
        public Context GetLastWaiting()
        {
            return new Context(in3_ctx_last_waiting(_nativeCtx), _wrapper);
        }


        /// <summary>
        /// Conditional to verify if the encapsulated pointer actually points to something.
        /// </summary>
        /// <returns><clang langword="true"/> if its valid, <see langword="false"/> if it is not.</returns>
        public bool IsValid()
        {
            return _nativeCtx != IntPtr.Zero;
        }

        /// <summary>
        /// Method to get the consolidated response of a request.
        /// </summary>
        /// <returns>The final result.</returns>
        public string GetResponse()
        {
            IntPtr rspPtr = ctx_get_response_data(_nativeCtx);
            string msg = Marshal.PtrToStringUTF8(rspPtr);
            // This needs to be freed since it is a copy of the response context.
            Native.Utils._free_(rspPtr);
            return msg;
        }


        /// <summary>
        /// Method to get the consolidated response of a request.
        /// </summary>
        /// <returns>The final result.</returns>
        public new CtxType GetType()
        {
            uint type = ctx_get_type(_nativeCtx);
            return (CtxType)type;
        }

        /// <summary>
        /// Destructor method for the native ctx encapsulated by the <see cref="Context"/> object.
        /// </summary>
        public void Dispose()
        {
            ctx_free(_nativeCtx);
        }

        /// <summary>
        /// Handle rpc request in an asynchronous manner.
        /// </summary>
        public async Task HandleRequest()
        {
            await new AsyncRequestHandler(_wrapper).Handle(_nativeCtx);
        }

        /// <summary>
        /// Handle signing request in an asynchronous manner.
        /// </summary>
        public async Task HandleSign()
        {
            await new AsyncSignerHandler(_wrapper).Handle(_nativeCtx);
        }

        /// <summary>
        /// Setter for the error on the current context. Proxies it to the native context.
        /// </summary>
        public void ReportError(string errorMessage)
        {
            IntPtr rpcPtr = Marshal.StringToHGlobalAnsi(errorMessage);
            ctx_set_error_intern(_nativeCtx, rpcPtr, (int)In3Code.IN3_ERPC);
            // This needs to be freed since our pointer is copied into the context by the function and needs to be freed this way else it will lead to Heap Corruption and its platform independent.
            Marshal.FreeHGlobal(rpcPtr);
        }

        /// <summary>
        /// Retrieve the error result on the context.
        /// </summary>
        /// <returns>A string describing the encountered error.</returns>
        public string GetErrorMessage()
        {
            IntPtr msgPtr = ctx_get_error_rpc(_nativeCtx, 0);
            string msg = Marshal.PtrToStringUTF8(msgPtr);
            // This needs to be freed since it is a copy of the error context.
            Native.Utils._free_(msgPtr);
            return msg;
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr ctx_get_error_rpc(IntPtr ctx, int ret);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void ctx_set_error_intern(IntPtr ctx, IntPtr message, int errnumber);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern uint ctx_get_type(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr ctx_new(IntPtr client, IntPtr req_data);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern string ctx_get_error_data(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void ctx_free(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern int in3_ctx_exec_state(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_ctx_last_waiting(IntPtr ctx);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr ctx_get_response_data(IntPtr ctx);

    }
}