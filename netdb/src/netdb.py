##
## Thanks to psi for this python netdb reader!
## Some modefications by Meeh
##
import os,sys,struct,time,hashlib,fnmatch


class _mutnum:
    """
    Mutable Number
    By Default python numbers are immutable
    """

    def __init__(self,val=0):
        self.val = val

    def inc(self,n):
        self.val += n
        return self

    def dec(self,n):
        self.val -= n
        return self
    
    def less(self,n):
        return self - n

    def __len__(self):
        return 0

    def __str__(self):
        return self.val.__str__()
    
    def __add__(self,x):
        return self.val.__add__(x)

    def __sub__(self,x):
        return self.val.__sub__(x)

    def __mul__(self,x):
        return self.val.__mul__(x)

    def __div__(self,x):
        return self.val.__div__(x)

    def __int__(self):
        return self.val.__int__()

    def __long__(self):
        return self.val.__long__()
    
    def __float__(self):
        return self.val.__float__()

    def __oct__(self):
        return self.val.__oct__()
    
    def __hex__(self):
        return self.val.__hex__()
    
    def __index__(self):
        return self.val.__index__()



# begin lambda hacks
_getslice = lambda d,l,h: d[l:h]
_getint = lambda d: struct.unpack('L',d)[0]
_getshort = lambda d: struct.unpack('!H',d)[0]
_get = lambda d, i: d[i]
if sys.version.startswith('3'):
    _get = lambda d, i: chr(d[i])

_getbyte = lambda d: d
_getbyteasint = lambda d: struct.unpack('!B',d)[0]
_getqlong = lambda d: struct.unpack('!Q',d)[0]
_getnext = lambda d, n, i: _getslice(d,i.val,i.inc(n))
_getrest = lambda d, i: d[i:]
_getnextlong = lambda d, i: _getint(_getnext(d,8,i))
_getnextbyte = lambda d, i: _getbyte(_getnext(d,1,i))
_getnextbyteasint = lambda d, i: _getbyteasint(_getnext(d,1,i))
_getnextshort = lambda d, i: _getshort(_getnext(d,2,i))
_getnextstr = lambda d, i: _getnext(d,_getnextbyteasint(d,i),i)
_mktime = lambda li: (time.asctime(time.gmtime(li)),li)
_getnexttime = lambda d,i: _mktime(_getqlong(_getnext(d,8,i))/1000.0)
# end lambda hacks


def _put_next_mapping(mapping,ind,data):
    tsize = _getnextshort(data,ind)
    data = _getnext(data,tsize,ind)
    ind = _mutnum()
    while ind.val < tsize:
        key = _getnextstr(data,ind)
        assert ord(_get(data,ind)) == ord(b'=')
        ind.inc(1)
        val = _getnextstr(data,ind)
        assert ord(_get(data,ind)) == ord(b';')
        ind.inc(1)
        if key in mapping:
            v = mapping[key]
            if isinstance(v,list):
                mapping[key].append(val)
            else:
                mapping[key] = [v,val]
        else:
            mapping[key] = val


def _read_rid(rid,data,pksize=256,sksize=128):
    ind = _mutnum()
    rid['pkey'] = hashlib.sha256(_getnext(data,pksize,ind)).hexdigest()
    rid['skey'] = hashlib.sha256(_getnext(data,sksize,ind)).hexdigest()
    rid['cert'] = hashlib.sha256(_getrest(data,ind)).hexdigest()

def _read_addr(addr,ind,data):
    addr['cost'] = _getnextbyteasint(data,ind)
    addr['expire'] = _getnexttime(data,ind)
    addr['transport'] = _getnextstr(data,ind)
    opts = dict()
    _put_next_mapping(opts,ind,data)
    addr['options'] = opts

def _read_entry(entry,fname):
    """
    read a netdb entry from a file
    to an object that can be accessed like a dictionary
    """

    try: # read data
        with open(fname,'rb') as r:
            data = r.read()
    except:
        return False

    ind = _mutnum()
    rid = dict()
    _read_rid(rid,_getnext(data,387,ind))
    entry['router_id'] = rid
    entry['published'] = _getnexttime(data,ind)
    size = _getnextbyteasint(data,ind)
    entry['addrs'] = []
    for n in range(size):
        addr = dict()
        _read_addr(addr,ind,data)
        entry['addrs'].append(addr)

    entry['peer_size'] = _getnextbyteasint(data,ind)
    opts = dict()
    _put_next_mapping(opts,ind,data)
    entry['options'] = opts
    entry['signature'] = hashlib.sha256(_getnext(data,40,ind)).hexdigest()
    entry['routerHash'] = fname.split('routerInfo-')[-1].split('.dat')[0]
    return True


def _print_hook(netdbe):
    print (netdbe)
    

def inspect(hook=_print_hook,netdb_dir=os.path.join(os.environ['HOME'],'.i2p','netDb')):
    """
    iterate through the netdb
    
    parameters:
    
      hook - function taking 1 parameter
        - the 1 parameter is a dictionary containing the info
          of a netdb enrty
        - called on every netdb entry

      netdb_dir - path to netdb folder
        - defaults to $HOME/.i2p/netDb/

    """
    for root, dirnames, filenames in os.walk(netdb_dir):
        for filename in fnmatch.filter(filenames, '*.dat'):
            fname = os.path.join(root, filename)
            e = dict()
            # Added a try. If one netDb file is corrupted, all fails if no try is used.
            try:
                if _read_entry(e,fname):
                    r = hook(e)
                    if r is not None:
                        print r
                        return r
            except:
                pass



