from __future__ import absolute_import, division, print_function
from __future__ import unicode_literals
from builtins import *
import logging
import struct
import time

from Crypto.Random.random import StrongRandom as random
from enum import Enum

from . import crypto
from . import util


#
# Common data types
#

def Date(num=None):
    if isinstance(num, bytes):
        num = struct.unpack(b'>Q', num)[0]
    if num is None:
        num = time.time() * 1000
    num = int(num)
    return struct.pack(b'>Q', num)


class Mapping(object):
    """
    i2p dictionary object
    it sucks
    """

    _log = logging.getLogger('Mapping')

    def __init__(self, opts=None, raw=None):
        if raw:
            self.data = raw
            self.opts = {}
            dlen = struct.unpack(b'>H', raw[:2])
            data = raw[2:2+dlen]
            while dlen > 0:
                key = String.parse(data)
                data = data[len(key)+1:]
                val = String.parse(data)
                data = data[len(val)+1:]
                dlen = len(data)
                self.opts[key] = val
        else:
            self.opts = opts or {}
            data = bytes()
            keys = sorted(self.opts.keys())
            for key in keys:
                val = bytes(opts[key], 'utf-8')
                key = bytes(key, 'utf-8')
                data += String.create(key)
                data += bytes('=', 'utf-8')
                data += String.create(val)
                data += bytes(';', 'utf-8')
            dlen = len(data)
            self._log.debug('len of Mapping is %d bytes' % dlen)
            dlen = struct.pack(b'>H', dlen)
            self.data = dlen + data

    def serialize(self):
        return self.data

    def __str__(self):
        return str([self.opts])


class String(object):

    @staticmethod
    def parse(data):
        dlen = util.get_as_int(data[0])
        return bytearray(data[:dlen])

    @staticmethod
    def create(data):
        if not isinstance(data, bytes):
            data = bytearray(data, 'utf-8')
        dlen = len(data)
        return struct.pack(b'>B', dlen) + data


class CertificateType(Enum):
    NULL = 0
    HASHCASH = 1
    HIDDEN = 2
    SIGNED = 3
    MULTI = 4
    KEY = 5


class Certificate(object):

    _log = logging.getLogger('Certificate')

    def _parse(self, raw, b64=False):
        if b64:
            if hasattr(raw, 'read'):
                raise TypeError('b64 flag is incompatible with stream data')
            raw = util.i2p_b64decode(raw)
        # TODO handle length in both cases
        #if len(raw) < 3:
        #    raise ValueError('invalid Certificate')
        if hasattr(raw, 'read'):
            ctype = raw.read(1)
            clen = raw.read(2)
        else:
            ctype = raw[0]
            clen = raw[1:3]
            raw = raw[3:]
        ctype = CertificateType(util.get_as_int(ctype))
        clen = struct.unpack(b'>H', clen)[0]
        if hasattr(raw, 'read'):
            data = raw.read(clen)
        else:
            data = raw[:clen]
            raw = raw[clen:]
        return ctype, data

    def __init__(self, type=CertificateType.NULL, data=bytes(), raw=None, b64=False):
        if raw:
            type, data = self._parse(raw, b64)
        if isinstance(type, str):
            type = type.encode('ascii')
        if isinstance(type, int) or isinstance(type, bytes):
            type = CertificateType(type)
        if raw is None and b64:
            data = util.i2p_b64decode(data)
        self.data = data
        self.type = type
        self._log.debug('type=%s data=%s raw=%s' % (type.name, util.i2p_b64encode(data), self.serialize()))

    def __str__(self):
        return '[cert type=%s data=%s]' % (self.type.name, self.data)

    def upconvert(self):
        if self.type == CertificateType.KEY:
            return KeyCertificate(data=self.data)
        else:
            return self

    def serialize(self, b64=False):
        data = bytearray()
        data += struct.pack(b'>B', self.type.value)
        data += struct.pack(b'>H', len(self.data))
        data += self.data
        if b64:
            data = util.i2p_b64encode(data)
        return data


class KeyCertificate(Certificate):

    _log = logging.getLogger('KeyCertificate')

    def __init__(self, sigkey=None, enckey=None, data=bytes(), raw=None,
                 b64=False):
        if sigkey is not None and enckey is not None:
            data = self._data_from_keys(sigkey, enckey)
        super().__init__(CertificateType.KEY, data, raw, b64)
        if len(self.data) < 4:
            raise ValueError("data too short: %s" % self.data)

    @staticmethod
    def _data_from_keys(sigkey, enckey):
        if not isinstance(sigkey, crypto.SigningKey):
            raise TypeError('sigkey is not a SigningKey')
        if not isinstance(enckey, crypto.CryptoKey):
            raise TypeError('enckey is not a CryptoKey')
        data = bytes()
        data += struct.pack(b'>H', sigkey.key_type.code)
        data += struct.pack(b'>H', enckey.key_type.code)
        # XXX Assume no extra crypto key data
        sigpub = sigkey.get_pubkey()
        extra = max(0, len(sigpub) - 128)
        data += sigpub[128:128+extra]
        return data

    @property
    def sigtype(self):
        return crypto.SigType.get_by_code(struct.unpack(b'>H',
                                                        self.data[:2])[0])

    @property
    def enctype(self):
        return crypto.EncType.get_by_code(struct.unpack(b'>H',
                                                        self.data[2:4])[0])

    @property
    def extra_sigkey_data(self):
        if self.sigtype is None:
            raise ValueError("unknown sig type")
        # XXX Assume no extra crypto key data
        extra = max(0, self.sigtype.pubkey_len - 128)
        return self.data[4:4+extra]

    @property
    def extra_enckey_data(self):
        # XXX Assume no extra crypto key data
        return bytes()


#
# Common data structures
#

class Destination(object):

    _log = logging.getLogger('Destination')

    def __init__(self, enckey=None, sigkey=None, cert=None, padding=bytes(),
                 raw=None, b64=False, private=False):
        """Construct a Destination.

        A Destination can be constructed in several ways:

        1. Generate a Destination with default types
            Destination()

        2. Generate a Destination with specified types
            Destination(EncType.EC_P256, SigType.ECDSA_SHA256_P256)

        3. Generate a Destination with one default and one specified type
            Destination(sigkey=SigType.ECDSA_SHA256_P256)

        4. Generate a Destination using types specified in a KeyCertificate
           (the KeyCertificate's extra key data is ignored)
            Destination(cert=keycert)

        5. Read a Destination in from an eepPriv.dat file
            with open(keyfile, 'rb') as rf:
                Destination(raw=rf, private=True)

        6. Parse a B64 Destination
            Destination(raw=b64string, b64=True)

        7. Construct a Destination from the provided keys
            Destination(enckey, sigkey)

        8. Construct a Destination from the provided keys and cert
            Destination(enckey, sigkey, cert)
        """
        if raw:
            enckey, sigkey, cert, padding = self._parse(raw, b64, private)

        rebuild_cert = False
        if enckey is None or isinstance(enckey, crypto.EncType) or \
                sigkey is None or isinstance(sigkey, crypto.SigType):
            if enckey is None:
                if isinstance(cert, KeyCertificate):
                    enckey = cert.enctype.cls()
                else:
                    enckey = crypto.ElGamalKey()
            elif isinstance(enckey, crypto.EncType):
                enckey = enckey.cls()
            if sigkey is None:
                if isinstance(cert, KeyCertificate):
                    sigkey = cert.sigtype.cls()
                else:
                    sigkey = crypto.DSAKey()
            elif isinstance(sigkey, crypto.SigType):
                sigkey = sigkey.cls()
            rebuild_cert = True

        # Cases:
        # - cert is None, need NULL -> build NULL
        # - cert is None, need KEY -> build KEY
        # - cert is MULTI -> error
        # - need NULL, cert is KEY -> build NULL
        # - need NULL, cert is not KEY -> leave
        # - need KEY, cert is not NULL or KEY -> error
        # - need KEY -> build KEY
        if cert is None:
            if enckey.key_type == crypto.EncType.ELGAMAL_2048 and \
                    sigkey.key_type == crypto.SigType.DSA_SHA1:
                cert = Certificate()
            else:
                cert = KeyCertificate(sigkey, enckey)
        elif rebuild_cert:
            if cert.type == CertificateType.MULTI:
                raise NotImplementedError('Multiple certs not yet supported')
            elif enckey.key_type == crypto.EncType.ELGAMAL_2048 and \
                    sigkey.key_type == crypto.SigType.DSA_SHA1:
                # Without MULTI+KEY, other certs are assumed to be ElG/DSA
                if cert.type == CertificateType.KEY:
                    cert = Certificate()
            elif cert.type != CertificateType.NULL and \
                    cert.type != CertificateType.KEY:
                raise NotImplementedError('Multiple certs not yet supported')
            else:
                cert = KeyCertificate(sigkey, enckey)

        self.enckey = enckey
        self.sigkey = sigkey
        self.cert = cert
        self.padding = padding

    def _parse(self, raw, b64=False, private=False):
        if b64:
            if hasattr(raw, 'read'):
                raise TypeError('b64 flag is incompatible with stream data')
            raw = util.i2p_b64decode(raw)
        # TODO handle length in both cases
        #if len(data) < 387:
        #    raise ValueError('invalid Destination')
        if hasattr(raw, 'read'):
            data = raw.read(384)
        else:
            data = raw[:384]
            raw = raw[384:]
        cert = Certificate(raw=raw)
        # If this is an eepPriv.dat, there will be more key material
        if hasattr(raw, 'read'):
            rest = raw.read()
        else:
            rest = raw[len(cert.serialize()):]

        if cert.type == CertificateType.KEY:
            cert = cert.upconvert()
            # XXX Assume no extra crypto key data
            enc_end = min(256, cert.enctype.pubkey_len)
            sig_start = max(256, 384-cert.sigtype.pubkey_len)
            encpub = data[:enc_end]
            padding = data[enc_end:sig_start]
            sigpub = data[sig_start:384] + \
                     cert.extra_sigkey_data
            if len(rest):
                encpriv = rest[:cert.enctype.privkey_len]
                sigpriv = rest[cert.enctype.privkey_len:\
                               cert.enctype.privkey_len+\
                               cert.sigtype.privkey_len]
                return cert.enctype.cls(encpub, encpriv), \
                       cert.sigtype.cls(sigpub, sigpriv), \
                       cert, \
                       padding
            else:
                return cert.enctype.cls(encpub), \
                       cert.sigtype.cls(sigpub), \
                       cert, \
                       padding
        elif cert.type != CertificateType.MULTI:
            # No KeyCert, so defaults to ElGamal/DSA
            encpub = data[:256]
            sigpub = data[256:384]
            if len(rest) and private:
                encpriv = rest[:256]
                sigpriv = rest[256:276]
                return crypto.ElGamalKey(encpub, encpriv), \
                       crypto.DSAKey(sigpub, sigpriv), \
                       cert, \
                       bytes()
            else:
                return crypto.ElGamalKey(encpub), \
                       crypto.DSAKey(sigpub), \
                       cert, \
                       bytes()
        else:
            raise NotImplementedError('Multiple certs not yet supported')

    def __str__(self):
        return '[Destination %s %s cert=%s]' % (
            self.base32(), self.base64(),
            self.cert)

    def has_private(self):
        """
        Returns True if this Destination contains private material, 
        False otherwise.
        """
        return self.enckey.has_private or self.sigkey.has_private()

    def to_public(self):
        """
        Return a copy of this Destination without any private key material.
        """
        if self.has_private():
            return Destination(self.enckey.to_public(),
                               self.sigkey.to_public(),
                               self.cert,
                               self.padding)
        else:
            return self

    def sign(self, data):
        return self.sigkey.sign(data)

    def signature_size(self):
        return self.sigkey.key_type.sig_len

    def verify(self, data, sig):
        return self.sigkey.verify(data, sig)

    def __len__(self):
        return len(self.serialize())

    def serialize(self, priv=False):
        if priv and not self.has_private():
            raise ValueError('No private key material in this Destination')
        data = bytes()
        if self.cert.type == CertificateType.KEY:
            encpub = self.enckey.get_pubkey()
            sigpub = self.sigkey.get_pubkey()
            enc_end = min(256, self.cert.enctype.pubkey_len)
            sig_start = max(256, 384-self.cert.sigtype.pubkey_len)
            if len(self.padding) == 0:
                # Generate random padding
                pad_len = sig_start - enc_end
                # Wrap with int() for Py2
                self.padding = int(random().getrandbits(pad_len*8)).to_bytes(
                    pad_len, 'big')
            data += encpub[:enc_end]
            data += self.padding
            data += sigpub[:384-sig_start]
            data += self.cert.serialize()
        elif self.cert.type != CertificateType.MULTI:
            data += self.enckey.get_pubkey()
            data += self.sigkey.get_pubkey()
            data += self.cert.serialize()
        else:
            raise NotImplementedError('Multiple certs not yet supported')
        if priv:
            data += self.enckey.get_privkey()
            data += self.sigkey.get_privkey()
        self._log.debug('serialize len=%d' % len(data))
        return data

    def base32(self):
        data = self.serialize()
        return util.i2p_b32encode(crypto.sha256(data)).decode('ascii')

    def base64(self):
        return util.i2p_b64encode(self.serialize()).decode('ascii')


class Lease(object):

    _log = logging.getLogger('Lease')

    def __init__(self, ri_hash=None, tid=None, end_date=None):
        self.ri = ri_hash
        self.tid = tid
        self.end_date = end_date
        self._log.debug('ri_hash %d bytes' % len(ri_hash))

    def serialize(self):
        data = bytearray()
        data += self.ri
        data += struct.pack(b'>I', self.tid)
        data += self.end_date
        self._log.debug('Lease is %d bytes' % len(data))
        assert len(data) == 44
        return data

    def __repr__(self):
        return '[Lease ri=%s tid=%d]' % ([self.ri], self.tid)


class LeaseSet(object):
    _log = logging.getLogger('LeaseSet')

    def __init__(self, raw=None, dest=None, ls_enckey=None, ls_sigkey=None,
                 leases=None):
        if raw:
            data = raw
            self.leases = []
            self.dest = Destination(raw=data)
            self._log.debug(self.dest)
            # Verify that the signature matches the Destination
            self.sig = raw[-40:]
            self.dest.verify(raw[:-40], self.sig)
            # Signature matches, now parse the rest
            data = data[len(self.dest):]
            self.enckey = crypto.ElGamalKey(data[:256])
            self._log.debug(self.enckey)
            data = data[256:]
            self.sigkey = crypto.DSAKey(data[:128])
            self._log.debug(self.sigkey)
            data = data[128:]
            numls = data[0]
            data = data[1:]
            while numls > 0:
                _l = data[:44]
                l = Lease(_l[:32], _l[32:36], _l[36:44])
                data = data[44:]
                numls -= 1
                self.leases.append(l)
        else:
            self.dest = dest
            self.enckey = ls_enckey
            self.sigkey = ls_sigkey
            self.leases = list(leases)

    def __str__(self):
        return '[LeaseSet leases=%s enckey=%s sigkey=%s dest=%s]' % (
            self.leases,
            [self.enckey.get_pubkey()],
            [self.sigkey.get_pubkey()],
            self.dest)

    def serialize(self):
        """
        serialize and sign LeaseSet
        only works with DSA-SHA1 right now
        """
        data = bytes()
        data += self.dest.serialize()
        data += self.enckey.get_pubkey()
        data += self.sigkey.get_pubkey()
        # Wrap with int() for Py2
        data += int(len(self.leases)).to_bytes(1, 'big')
        for l in self.leases:
            data += l.serialize()
        sig = self.dest.sign(data)
        data += sig
        self._log.debug('LS has length %d' % len(data))
        return data


class I2CPProtocol(Enum):

    STREAMING = 6
    DGRAM = 17
    RAW = 18


class datagram(object):

    def __eq__(self, obj):
        if hasattr(self, 'payload') and hasattr(obj, 'payload'):
            return self.payload == obj.payload
        return False


class raw_datagram(object):

    protocol = I2CPProtocol.RAW

    def __init__(self, dest=None, raw=None, payload=None):
        if raw:
            self.data = raw
        else:
            self.data = payload

        self.dest = None

    def serialize(self):
        return self.data


class dsa_datagram(datagram):

    protocol = I2CPProtocol.DGRAM
    _log = logging.getLogger('datagram-dsa')

    def __init__(self, dest=None, raw=None, payload=None):
        if raw:
            self._log.debug('rawlen=%d' % len(raw))
            self.data = raw
            self._log.debug('load dgram data: %s' % [raw])
            self.dest = Destination(raw=raw)
            self._log.debug('destlen=%s' % self.dest)
            raw = raw[len(self.dest):]
            self._log.debug('raw=%s' % [raw])
            self.sig = raw[:40]
            raw = raw[40:]
            self._log.debug('payloadlen=%d' % len(raw))
            self.payload = raw
            phash = crypto.sha256(self.payload)
            self._log.debug('verify dgram: sig=%s hash=%s' % (
                [self.sig], [phash]))
            self.dest.verify(phash, self.sig)
        else:
            self.dest = dest
            self.payload = payload
            self.data = bytearray()
            self.data += self.dest.serialize()
            payload_hash = crypto.sha256(self.payload)
            self.sig = self.dest.sign(payload_hash)
            self._log.debug('signature=%s' % [self.sig])
            self.data += self.sig
            self.data += payload

    def serialize(self):
        return self.data

    def __str__(self):
        return '[DSADatagram payload=%s sig=%s]' % (self.payload, self.sig)


class i2cp_payload(object):

    gz_header = b'\x1f\x8b\x08'

    _log = logging.getLogger('i2cp_payload')

    def __init__(self, raw=None, data=None, srcport=0, dstport=0,
                 proto=I2CPProtocol.RAW):
        if raw:
            self.dlen = struct.unpack(b'>I', raw[:4])[0]
            self._log.debug('payload len=%d' % self.dlen)
            data = raw[4:self.dlen]
            self._log.debug('compressed payload len=%d' % len(data))
            assert data[:3] == self.gz_header
            self.flags = data[3]
            self.srcport = struct.unpack(b'>H', data[4:6])[0]
            self.dstport = struct.unpack(b'>H', data[6:8])[0]
            self.xflags = data[8]
            self.proto = I2CPProtocol(util.get_as_int(data[9]))
            self.data = util.i2p_decompress(data[10:])
            self._log.debug('decompressed=%s' % [self.data])
        else:
            if util.check_portnum(srcport) and util.check_portnum(dstport):
                self._log.debug('payload data len=%d' % len(data))
                self.data = util.i2p_compress(data)
                self._log.debug('compressed payload len=%d' % len(self.data))
                self.srcport = srcport
                self.dstport = dstport
                self.proto = I2CPProtocol(proto)
                self.flags = 0
                self.xflags = 2
            else:
                raise ValueError('invalid ports: srcport=%s dstport=%s' % (
                    [srcport], [dstport]))

    def serialize(self):
        data = bytearray()
        data += self.gz_header
        data += struct.pack(b'>B', self.flags)
        data += struct.pack(b'>H', self.srcport)
        data += struct.pack(b'>H', self.dstport)
        data += struct.pack(b'>B', self.xflags)
        data += struct.pack(b'>B', self.proto.value)
        data += self.data
        dlen = len(data)
        self._log.debug('serialize len=%d' % dlen)
        return struct.pack(b'>I', dlen) + data

    def __str__(self):
        return ('[Payload flags=%s srcport=%s dstport=%s xflags=%s' +
               ' proto=%s data=%s]') % (
            self.flags,
            self.srcport,
            self.dstport,
            self.xflags,
            self.proto,
            self.data)


def to_b32_bytes(val):
    if isinstance(val, Destination):
        return to_b32_bytes(val.base64())
    if isinstance(val, bytes):
        if val.lower().endswith(b".b32.i2p"):
            return util.i2p_b32decode(val)
        else:
            return crypto.sha256(vale)
    raise TypeError("invalid type", val)
