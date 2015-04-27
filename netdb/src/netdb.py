##
## i2p netdb parser
##
## Author: Jeff
## MIT Liecense 2014
##
import os,sys,struct,time,hashlib,fnmatch,io
from geoip import geolite2
import base64
import logging

b64encode = lambda x : base64.b64encode(x, '~-')

def sha256(data,raw=True):
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
    cost = None
    transport = None
    options = None
    expire = None
    location = None

    def valid(self):
        return None not in (self.cost, self.transport, self.options, self.expire)
    
    def __repr__(self):
        return 'Address: transport={} cost={} expire={} options={} location={}' \
            .format(self.transport, self.cost, self.expire, self.options, self.location)
    
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
        d = Entry._read(fd, 2)
        if d:
            return struct.unpack('!H',d)[0]

    @staticmethod
    def _read_mapping(fd):
        Entry._log.debug('read_mapping')
        mapping = dict()
        tsize = Entry._read_short(fd)
        if tsize is None:
            return
        data = Entry._read(fd, tsize)
        if data is None:
            return
        sfd = io.BytesIO(data)
        ind = 0
        while ind < tsize:
            Entry._log.debug(ind)
            key = Entry._read_string(sfd)
            if key is None:
                return
            Entry._log.debug(['key', key])
    
            ind += len(key) + 2
            Entry._read_byte(sfd)
            val = Entry._read_string(sfd)
            if val is None:
                return
            Entry._log.debug(['val',val])

            ind += len(val) + 2
            Entry._read_byte(sfd)
    
            #key = key[:-1]
            #val = val[:-1]
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
        dat = fd.read(amount)
        Entry._log.debug('read %d of %d bytes' % (len(dat), amount))
        if len(dat) == amount:
            return dat
        

    @staticmethod 
    def _read_byte(fd):
        b = Entry._read(fd,1)
        if b:
            return struct.unpack('!B', b)[0]
    
    @staticmethod
    def _read_string(fd):
        Entry._log.debug('read_string')
        slen = Entry._read_byte(fd)
        if slen:
            return Entry._read(fd, slen)

    @staticmethod
    def _read_time(fd):
        d = Entry._read(fd, 8)
        if d:
            li = struct.unpack('!Q', d)[0]
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
        if addr.valid():
			# This is a try because sometimes hostnames show up.
			# TODO: Make it allow host names.
            try:
                addr.location = geolite2.lookup(addr.options.get('host', None))
            except:
                addr.location = None
            return addr

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
            self._log.debug('load from file {}'.format(filename))
            self._load(fr)
            #self.routerHash = 
            
    def _load(self, fd):
        """
        load from file descriptor
        """
        data = self._read(fd, 387)
        if data is None:
            return
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
        if self.published is None:
            return

        # reachable addresses
        self.addrs = list()
        addrlen = self._read_byte(fd)
        if addrlen is None:
            return
        for n in range(addrlen):
            addr = self._read_addr(fd)
            if addr is None:
                return
            self.addrs.append(addr)
 
        # peer size
        self.peer_size = self._read_byte(fd)
        if self.peer_size is None:
            return
        
        # other options
        self.options = self._read_mapping(fd)
        if self.options is None:
            return

        # signature
        self.signature = sha256(self._read(fd, 40))
        if self.signature is None:
            return
        self.valid = True
        
    def verify(self):
        """
        verify router identity
        """
        #TODO: verify 

    def __repr__(self):
        val = str()
        val += 'NetDB Entry '
        val += 'pubkey={} '.format(b64encode(self.pubkey))
        val += 'signkey={} '.format(b64encode(self.signkey))
        val += 'options={} '.format(self.options)
        val += 'addrs={} '.format(self.addrs)
        val += 'cert={} '.format(b64encode(self.cert))
        val += 'published={} '.format(self.published)
        val += 'signature={}'.format(b64encode(self.signature))
        return val

    def dict(self):
        """
        return dictionary in old format
        """
        return dict({
           'pubkey':b64encode(self.pubkey),
            'signkey':b64encode(self.signkey),
            'options':self.options,
            'addrs':self.addrs,
            'cert':b64encode(self.cert),
            'published':self.published,
            'signature':b64encode(self.signature)
            })

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

