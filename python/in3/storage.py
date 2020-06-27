import ctypes as c
import os
import pathlib as p
import warnings

path = p.Path(p.Path(p.Path.home(), '.in3'))


def store(_cptr: int, key: str, value: c.c_char_p):
    path.parent.mkdir(parents=True, exist_ok=True)
    try:
        with open(p.Path(path, key), 'wb') as file:
            file.write(c.string_at(value))
    except Exception as e:
        warnings.warn('In3 Cache: error:\n{}\ncleaning cache.'.format(str(e)), RuntimeWarning)


def retrieve(_cptr: int, key: str):
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


def delete_all(_cptr=None):
    [os.unlink(file.path) for file in os.scandir(path)]
