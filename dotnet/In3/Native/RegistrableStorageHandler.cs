using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class RegistrableStorageHandler : IRegistrableHandler
    {
        private NativeClient Wrapper;

        private GCHandle _giCol;
        private GCHandle _siCol;
        private GCHandle _cCol;
        private delegate IntPtr in3_storage_get_item(IntPtr cptr, string key);
        private delegate void in3_storage_set_item(IntPtr cptr, string key, byte[] content);
        private delegate bool in3_storage_clear(IntPtr cptr);

        public RegistrableStorageHandler(NativeClient wrapper)
        {
            Wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            in3_storage_get_item getItemDel = GetItem;
            _giCol = GCHandle.Alloc(getItemDel);

            in3_storage_set_item setItemDel = SetItem;
            _siCol = GCHandle.Alloc(setItemDel);

            in3_storage_clear clearDel = Clear;
            _cCol = GCHandle.Alloc(clearDel);

            in3_set_storage_handler(Wrapper.Pointer, getItemDel, setItemDel, clearDel,
                Wrapper.Pointer);
        }

        public void UnregisterNativeHandler()
        {
            _giCol.Free();
            _siCol.Free();
            _cCol.Free();
        }

        private IntPtr GetItem(IntPtr ignored, string key)
        {
            byte[] result = Wrapper.Client.Storage.GetItem(key);
            if (result == null) return IntPtr.Zero;
            // This needs to be release on c-side so its needs to be created there as well to prevent heap corruption.
            IntPtr unmanagedPointer = b_new(result, (uint) result.Length);
            return unmanagedPointer;
        }

        private void SetItem(IntPtr ignored, string key, [MarshalAs(UnmanagedType.LPArray)] byte[] content)
        {
            Wrapper.Client.Storage.SetItem(key, content);
        }

        private bool Clear(IntPtr ignored)
        {
            return Wrapper.Client.Storage.Clear();
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr b_new([MarshalAs(UnmanagedType.LPArray)] byte[] content, uint len);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_set_storage_handler(IntPtr c, in3_storage_get_item get_item, in3_storage_set_item set_item, in3_storage_clear clear, IntPtr cptr);
    }
}