##
#
# bencode.py python3 compatable bencode / bdecode
#
##
from future.builtins import *

def _decode_int(data):
    """
    decode integer from bytearray
    return int, remaining data
    """
    data = data[1:]
    end = data.index(b'e')
    return int(data[:end],10), data[end+1:]

def _decode_str(data):
    """
    decode string from bytearray
    return string, remaining data
    """
    start = data.index(b':')
    dlen = int(data[:start].decode(),10)
    if dlen <= 0:
        raise Exception('invalid string size: %d'%d)
    start += 1
    ret = bytes(data[start:start+dlen])
    data = data[start+dlen:]
    return ret, data

def _decode_list(data):
    """
    decode list from bytearray
    return list, remaining data
    """
    ls = []
    data = data[1:]
    while data[0] != ord(b'e'):
        elem, data = _decode(data)
        ls.append(elem)
    return ls, data[1:]

def _decode_dict(data):
    """
    decode dict from bytearray
    return dict, remaining data
    """
    d = {}
    data = data[1:]
    while data[0] != ord(b'e'):
        k, data = _decode_str(data)
        v, data = _decode(data)
        d[k.decode()] = v
    return d, data[1:]

def _decode(data):
    """
    decode a bytearray
    return deserialized object, remaining data
    """
    ch = chr(data[0])
    if ch == 'l':
        return _decode_list(data)
    elif ch == 'i':
        return _decode_int(data)
    elif ch == 'd':
        return _decode_dict(data)
    elif ch.isdigit():
        return _decode_str(data)

def decode(data):
    """
    decode a bytearray
    return deserialized object or None
    """
    obj , data = _decode(data)
    if data and len(data) == 0:
        return obj

def _encode_str(s,buff):
    """
    encode string to a buffer
    """
    s = bytearray(s)
    l = len(s)
    buff.append(bytearray(str(l)+':','utf-8'))
    buff.append(s)
    
def _encode_int(i,buff):
    """
    encode integer to a buffer
    """
    buff.append(b'i')
    buff.append(bytearray(str(i),'ascii'))
    buff.append(b'e')

def _encode_list(l,buff):
    """
    encode list of elements to a buffer
    """
    buff.append(b'l')
    for i in l:
        _encode(i,buff)
    buff.append(b'e')

def _encode_dict(d,buff):
    """
    encode dict
    """
    buff.append(b'd')
    l = list(d.keys())
    l.sort()
    for k in l:
        _encode(str(k),buff)
        _encode(d[k],buff)
    buff.append(b'e')

def _encode(obj,buff):
    """
    encode element obj to a buffer buff
    """
    if isinstance(obj,str):
        _encode_str(bytearray(obj,'utf-8'),buff)
    elif isinstance(obj,bytes):
        _encode_str(bytearray(obj),buff)
    elif isinstance(obj,bytearray):
        _encode_str(obj,buff)
    elif str(obj).isdigit():
        _encode_int(obj,buff)
    elif isinstance(obj,list):
        _encode_list(obj,buff)
    elif hasattr(obj,'keys') and hasattr(obj,'values'):
        _encode_dict(obj,buff)
    elif str(obj) in ['True','False']:
        _enocde_int(int(obj and '1' or '0'),buff)
    else:
        raise Exception('non serializable object: %s'%obj)


def encode(obj):
    """
    bencode element, return bytearray
    """
    buff = []
    _encode(obj,buff)
    ret =  bytearray()
    for ba in buff:
        ret +=  ba
    return bytes(ret)
