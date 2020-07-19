import ctypes as c
import os
import pathlib as p
import warnings

path = p.Path(p.Path(p.Path.home(), '.in3'))


class Bytes(c.Structure):
    """
    typedef struct bytes {
      uint8_t* data; /**< the byte-data  */
      uint32_t len;  /**< the length of the array ion bytes */
    } bytes_t;
    """
    _fields_ = [("data", c.POINTER(c.c_uint8)),
                ("len", c.c_uint32)]


@c.CFUNCTYPE(None, c.c_void_p, c.POINTER(c.c_char), c.POINTER(Bytes))
def set_item(_cptr, key, value):
    """
    Stores a new cache file or updates an existing one.
    Args:
        _cptr: context pointer, dont mess with it.
        key: File name to be stored.
        value: File contents.
    """
    key = c.string_at(key).decode('utf8')
    path.mkdir(parents=True, exist_ok=True)
    try:
        value = c.string_at(value.contents.data, value.contents.len)
        with open(p.Path(path, key), 'w+b') as file:
            file.write(value)
    except Exception as e:
        warnings.warn('In3 Cache: error:\n{}\ncleaning cache.'.format(str(e)), RuntimeWarning)


@c.CFUNCTYPE(c.c_void_p, c.c_void_p, c.POINTER(c.c_char))
def get_item(_cptr, key):
    """
    Retrieves the contents of existing cache file or None in case it doesn't exist.
    Args:
        _cptr: context pointer, dont mess with it.
        key: File name to be read.
    Returns:
        data (Bytes): Data struct with the file contents.
    """
    key = c.string_at(key).decode('utf8')
    file_path = p.Path(path, key)
    data = None
    try:
        if file_path.exists():
            data = file_path.read_bytes()
    except Exception as e:
        warnings.warn('In3 Cache: error:\n{}\ncleaning cache.'.format(str(e)), RuntimeWarning)
        clear(None)
        return None
    if not data:
        return None
    from in3.libin3 import rpc_api
    data_byte_t = rpc_api.libin3_new_bytes_t(data, len(data))
    return data_byte_t


@c.CFUNCTYPE(None, c.c_void_p)
def clear(_cptr=None):
    """
    Clears all local cache files.
    Args:
        _cptr: context pointer, dont mess with it.
    """
    [os.unlink(file.path) for file in os.scandir(path)]
