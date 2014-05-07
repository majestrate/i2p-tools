##
## i2p netdb parser
##
## Author: Jeff
## MIT Liecense 2014
##
import os,sys,struct,time,hashlib,fnmatch,io
import logging

def sha256(data,raw=False):
    """
    compute sha256 of data
    """
    h = hashlib.new('sha256')
    h.update(data)
    if raw:
        return h.digest()
    else:
        return h.hexdigest()

class Inspector:

    _log = logging.getLogger('NDBInspector')
    
    def inspect(self, entry):
        pass
        
    def run(self, ndb):
        entry_counter = 0
        for root, dirnames, filenames in os.walk(ndb):
            for filename in fnmatch.filter(filenames, '*.dat'):
                fname = os.path.join(root, filename)
                e = Entry(fname)
                e.verify()
                if e.valid:
                    entry_counter += 1
                    self.inspect(e)
                else:
                    self._log.warn('invalid entry in file {}'.format(fname))
        self._log.info('read {} entries'.format(entry_counter))

class Address:
    """
    netdb address
    """


class Entry:
    """
    netdb entry
    """
    _pubkey_size = 256
    _signkey_size = 128

    _log = logging.getLogger('NDBEntry')

    @staticmethod
    def _read_short(fd):
        Entry._log.debug('read_short')
        return struct.unpack('!H',fd.read(2))[0]

    @staticmethod
    def _read_mapping(fd):
        Entry._log.debug('read_mapping')
        mapping = dict()
        tsize = Entry._read_short(fd)
        data = Entry._read(fd, tsize)
        sfd = io.BytesIO(data)
        ind = 0
        while ind < tsize:
            Entry._log.debug(ind)
            key = Entry._read_string(sfd)
            Entry._log.debug([key])
    
            ind += len(key) + 2
            Entry._read_byte(sfd)
            val = Entry._read_string(sfd)
            Entry._log.debug([val])

            ind += len(val) + 2
            Entry._read_byte(sfd)
    
            key = key[:-1]
            val = val[:-1]
            if key in mapping:
                v = mapping[key]
                if isinstance(v,list):
                    mapping[key].append(val)
                else:
                    mapping[key] = [v,val]
            else:
                mapping[key] = val
        return mapping

    @staticmethod
    def _read(fd, amount):
        Entry._log.debug('read %d bytes' % amount)
        return fd.read(amount)

    @staticmethod 
    def _read_byte(fd):
        return struct.unpack('!B', Entry._read(fd,1))[0]
    
    @staticmethod
    def _read_string(fd):
        Entry._log.debug('read_string')
        slen = Entry._read_byte(fd)
        return Entry._read(fd, slen)

    @staticmethod
    def _read_time(fd):
        li = struct.unpack('!Q', fd.read(8))
        return li

    @staticmethod
    def _read_addr(fd):
        """
        load next router address
        """
        Entry._log.debug('read_addr')
        addr = Address()
        addr.cost = Entry._read_byte(fd)
        addr.expire = Entry._read_time(fd)
        addr.transport = Entry._read_string(fd)
        addr.options = Entry._read_mapping(fd)



    def __init__(self, filename):
        """
        construct a NetDB Entry from a file
        """
        self.addrs = list()
        self.options = dict()
        self.pubkey = None
        self.signkey = None
        self.cert = None
        self.published = None
        self.signature = None
        self.peer_size = None
        self.valid = False
        with open(filename, 'rb') as fr:
            self._load(fr)
            self.routerHash = filename.split('routerInfo-')[-1].split('.dat')[0]
            
    def _load(self, fd):
        """
        load from file descriptor
        """
        data = self._read(fd, 387)
        ind = 0
        # public key
        self.pubkey = sha256(data[ind:ind+self._pubkey_size])
        ind += self._pubkey_size

        # signing key
        self.signkey = sha256(data[ind:ind+self._signkey_size])
        ind + self._signkey_size

        # certificate
        self.cert =  sha256(data[ind:])

        # date published
        self.published  = self._read_time(fd)
        
        # reachable addresses
        self.addrs = list()
        addrlen = self._read_byte(fd)
        for n in range(addrlen):
            addr = self._read_addr(fd)
            self.addrs.append(addr)
 
        # peer size
        self.peer_size = self._read_byte(fd)
        
        # other options
        self.options = self._read_mapping(fd)
        # signature
        self.signature = sha256(self._read(fd, 40))

    def verify(self):
        """
        verify router identity
        """
        #TODO: verify 
        self.valid = True

    def dict(self):
        """
        return dictionary in old format
        """
        #TODO: implement old format

def inspect(hook=None,netdb_dir=os.path.join(os.environ['HOME'],'.i2p','netDb')):
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

    insp = Inspector()
    if hook is not None:
        insp.inspect = hook 
    insp.run(netdb_dir)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    inspect()
