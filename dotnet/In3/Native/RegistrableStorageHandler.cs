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
        private delegate void in3_storage_set_item(IntPtr cptr, string key, IntPtr content);
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
            IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(byte)) * result.Length);
            Marshal.Copy(result, 0, p, result.Length);
            IntPtr unmanagedPointer = b_new(p, (uint) result.Length);
            Marshal.FreeHGlobal(p);
            return unmanagedPointer;
        }

        private void SetItem(IntPtr ignored, string key, IntPtr byteArray)
        {
            if (byteArray != IntPtr.Zero)
            {
                uint size = b_get_len(byteArray);
                byte[] byteArrayContent = new byte[size];
                IntPtr byteArrayData = b_get_data(byteArray);
                Marshal.Copy(byteArrayData, byteArrayContent, 0, (int) size);
                Wrapper.Client.Storage.SetItem(key, byteArrayContent);
            }
        }

        private bool Clear(IntPtr ignored)
        {
            return Wrapper.Client.Storage.Clear();
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr b_get_data(IntPtr b);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern uint b_get_len(IntPtr b);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr b_new(IntPtr content, uint len);
        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern IntPtr in3_set_storage_handler(IntPtr c, in3_storage_get_item get_item, in3_storage_set_item set_item, in3_storage_clear clear, IntPtr cptr);
    }
}