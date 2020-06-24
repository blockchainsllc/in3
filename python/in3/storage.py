import pathlib as p
import os
import warnings
import ctypes as c

path = p.Path(p.Path(p.Path.home(), '.in3'))
"""
class In3Request(c.Structure):

    _fields_ = [("payload", c.POINTER(c.c_char)),
                ("urls", c.POINTER(c.POINTER(c.c_char))),
                ("urls_len", c.c_int),
                ("results", c.c_void_p),
                ("timeout", c.c_uint32),
                ("times", c.c_uint32)]
"""


@c.CFUNCTYPE(c.c_int, c.c_int, c.c_char_p, c.c_int)
def store(_cptr, key: str, value: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    try:
        with open(p.Path(path, key), 'wb') as file:
            file.write(value)
    except Exception as e:
        warnings.warn('In3 Cache: error:\n{}\ncleaning cache.'.format(str(e)), RuntimeWarning)


@c.CFUNCTYPE(c.c_int, c.c_int, c.c_char_p)
def retrieve(_cptr, key: str):
    file_path = p.Path(path, key)
    data = bytearray()
    try:
        if file_path.exists():
            data.join(file_path.read_bytes())
    except Exception as e:
        warnings.warn('In3 Cache: error:\n{}\ncleaning cache.'.format(str(e)), RuntimeWarning)
        delete_all()
        return None
    return data


@c.CFUNCTYPE(c.c_int, c.c_int)
def delete_all(_cptr=None):
    [os.unlink(file.path) for file in os.scandir(path)]
