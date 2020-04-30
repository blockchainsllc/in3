using System;
using System.Runtime.InteropServices;

namespace In3.Native
{
    internal class NativeStorageHandler
    {
        private NativeWrapper Wrapper;

        private in3_storage_get_item GetItemDel { get; set; }
        private in3_storage_set_item SetItemDel { get; set; }
        private in3_storage_clear ClearDel { get; set; }

        private delegate byte[] in3_storage_get_item(IntPtr cptr, string key);
        private delegate void in3_storage_set_item(IntPtr cptr, string key, byte[] content);
        private delegate bool in3_storage_clear(IntPtr cptr);

        // structs
        [StructLayout(LayoutKind.Sequential)] private struct in3_storage_handler_t {
            public in3_storage_get_item get_item;
            public in3_storage_set_item set_item;
            public in3_storage_clear clear;
            public IntPtr cptr;
        }

        public NativeStorageHandler(NativeWrapper wrapper)
        {
            Wrapper = wrapper;
        }

        public void RegisterNativeHandler()
        {
            in3_storage_get_item getItemDel = new in3_storage_get_item(GetItem);
            GCHandle giCol = GCHandle.Alloc(getItemDel);

            in3_storage_set_item setItemDel = new in3_storage_set_item(SetItem);
            GCHandle siCol = GCHandle.Alloc(setItemDel);

            in3_storage_clear clearDel = new in3_storage_clear(Clear);
            GCHandle cCol = GCHandle.Alloc(clearDel);

            in3_storage_handler_t storageHandler;
            storageHandler.get_item = getItemDel;
            storageHandler.set_item = setItemDel;
            storageHandler.clear = clearDel;
            storageHandler.cptr = Wrapper.NativeClientPointer;

            IntPtr myStructPtr = Marshal.AllocHGlobal(Marshal.SizeOf<in3_storage_handler_t>()); // Allocate unmanaged memory for the struct
            Marshal.StructureToPtr(storageHandler, myStructPtr, false);
            in3_set_default_storage(myStructPtr);
        }

        private byte[] GetItem(IntPtr ignored, string key)
        {
            return Wrapper.Client.Storage.GetItem(key);
        }

        private void SetItem(IntPtr ignored, string key, [MarshalAs(UnmanagedType.LPArray)] byte[] content)
        {
            Wrapper.Client.Storage.SetItem(key, content);
        }

        private bool Clear(IntPtr ignored)
        {
            return Wrapper.Client.Storage.Clear();
        }

        [DllImport("libin3", CharSet = CharSet.Ansi)] private static extern void in3_set_default_storage(IntPtr cacheStorage);
    }
}