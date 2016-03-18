from __future__ import absolute_import, division, print_function, unicode_literals
from builtins import *

from Crypto.Hash import SHA, SHA256
from Crypto.PublicKey import ElGamal, DSA
from Crypto.Random.random import StrongRandom as random
from Crypto.Util import asn1
from pyelliptic.ecc import ECC
from enum import Enum
import libnacl

from .util import *

#
# Parameters
#

elgamal_p = int('FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF', 16)
elgamal_g = 2
ELGAMAL_2048_SPEC = (elgamal_p, elgamal_g)

dsa_seed = int('86108236b8526e296e923a4015b4282845b572cc', 16)
dsa_p = int('9C05B2AA960D9B97B8931963C9CC9E8C3026E9B8ED92FAD0A69CC886D5BF8015FCADAE31A0AD18FAB3F01B00A358DE237655C4964AFAA2B337E96AD316B9FB1CC564B5AEC5B69A9FF6C3E4548707FEF8503D91DD8602E867E6D35D2235C1869CE2479C3B9D5401DE04E0727FB33D6511285D4CF29538D9E3B6051F5B22CC1C93', 16)
dsa_q = int('A5DFC28FEF4CA1E286744CD8EED9D29D684046B7', 16)
dsa_g = int('0C1F4D27D40093B429E962D7223824E0BBC47E7C832A39236FC683AF84889581075FF9082ED32353D4374D7301CDA1D23C431F4698599DDA02451824FF369752593647CC3DDC197DE985E43D136CDCFC6BD5409CD2F450821142A5E6F8EB1C3AB5D0484B8129FCF17BCE4F7F33321C3CB3DBB14A905E7B2B3E93BE4708CBCC82', 16)
DSA_SHA1_SPEC = (dsa_g, dsa_p, dsa_q)

# Use numeric codes because pyelliptic has issues with strings on Py2
P256_SPEC = 415 # prime256v1
P384_SPEC = 715 # secp384r1
P521_SPEC = 716 # secp521r1
F4_2048_SPEC = None
F4_3072_SPEC = None
F4_4096_SPEC = None
Ed25519_SHA_512_SPEC = None


def sha1(x): return SHA.new(x).digest()
def sha256(x): return SHA256.new(x).digest()


#
# Keys
#

class Key(object):
    """Base class for keys."""

    def __init__(self, key_type, pub=None, priv=None, key=None):
        """Create a key of type key_type.

        If pub or priv are set, creates a Key using the provided key material.
        If key is set, creates a Key using the provided key.
        If no kwargs are provided, generates a new Key.
        """
        self.key_type = key_type
        if pub or priv:
            if pub is not None and len(pub) != key_type.pubkey_len:
                raise ValueError('pub key material is wrong length: %d instead of %d' % (len(pub), key_type.pubkey_len))
            if priv is not None and len(priv) != key_type.privkey_len:
                raise ValueError('priv key material is wrong length: %d instead of %d' % (len(priv), key_type.privkey_len))
            # load a key from raw bytes
            key = self._parse(pub, priv)
        elif key is None:
            # generate a new key
            key = self._generate()
        if isinstance(key, bytes):
            raise TypeError('Pass in key material with the pub= and priv= kwargs')
        self.key = key

    def has_private(self):
        """Returns True if this Key contains private material, False otherwise."""
        return self._has_private()

    def to_public(self):
        """Return a copy of this Key without any private key material."""
        if self.has_private():
            return self._to_public()
        else:
            return self

    def get_pubkey(self):
        """Get the serialized public key material."""
        return self._get_pubkey()

    def get_privkey(self):
        """Get the serialized private key material."""
        if not self.has_private():
            raise TypeError('Private key not available in this object')
        return self._get_privkey()


class CryptoKey(Key):
    """Base class for encryption keys."""

    def encrypt(self, plaintext):
        """Encrypt plaintext with public key.

        :param plaintext: bytearray to encrypt
        :return: bytearray with cyphertext
        """
        return self._encrypt(plaintext)

    def decrypt(self, ciphertext):
        """Decrypt ciphertext with private key.

        :param ciphertext: bytearray to decrypt
        :return: bytearray with plaintext
        """
        if not self.has_private():
            raise TypeError('Private key not available in this object')
        return self._decrypt(ciphertext)


class SigningKey(Key):
    """Base class for signing keys."""

    def sign(self, data):
        """Sign data with private key.

        :param data: bytearray to sign
        :return: a detached signature
        """
        if not self.has_private():
            raise TypeError('Private key not available in this object')
        return self._sign(data)

    def verify(self, data, sig):
        """Verify detached signature for data.

        :param data: bytearray for data that was signed
        :param sig: bytearray for detached sig
        :return: True if valid signature otherwise False
        """
        return self._verify(data, sig)


class ElGamalKey(CryptoKey):

    def __init__(self, pub=None, priv=None, key=None):
        """Construct an ElGamal-2048 encryption key.

        With no arguments, generates a new ElGamalKey.
        If pub or priv are set, creates an ElGamalKey using provided key material.
        If key is set, creates an ElGamalKey using the provided key.
        """
        super().__init__(EncType.ELGAMAL_2048, pub, priv, key)

    @staticmethod
    def _parse(pub, priv=None):
        """Parse key data"""
        y = int.from_bytes(pub, 'big')
        x = int.from_bytes(priv, 'big') if priv else None
        return ElGamalKey._construct(y, x)

    @staticmethod
    def _generate():
        """Generate an ElGamal key pair.

        This needs an audit.
        """
        x = random().randint(2, elgamal_p)
        y = pow(elgamal_g, x, elgamal_p)
        return ElGamalKey._construct(y, x)

    @staticmethod
    def _construct(y, x=None):
        tup = (elgamal_p, elgamal_g, y, x) if x else (elgamal_p, elgamal_g, y)
        return ElGamal.construct(tup)

    def _has_private(self):
        return self.key.has_private()

    def _to_public(self):
        return ElGamalKey(key=ElGamalKey._construct(self.key.y, None))

    def _get_pubkey(self):
        return int(self.key.y).to_bytes(256, 'big')

    def _get_privkey(self):
        return int(self.key.x).to_bytes(256, 'big')

    def _encrypt(self, plaintext):
        raise NotImplementedError

    def _decrypt(self, ciphertext):
        raise NotImplementedError


class DSAKey(SigningKey):

    def __init__(self, pub=None, priv=None, key=None):
        """Construct a DSA-SHA1 signing key.

        With no arguments, generates a new DSAKey.
        If pub or priv are set, creates a DSAKey using provided key material.
        If key is set, creates a DSAKey using the provided key.
        """
        super().__init__(SigType.DSA_SHA1, pub, priv, key)

    @staticmethod
    def _parse(pub, priv=None):
        """Parse key data"""
        y = int.from_bytes(pub, 'big')
        x = int.from_bytes(priv, 'big') if priv else None
        return DSAKey._construct(y, x)

    @staticmethod
    def _generate():
        """Generate a DSA key pair.

        This needs an audit.
        """
        x = random().randint(1, 2 ** 160)
        y = pow(dsa_g, x, dsa_p)
        return DSAKey._construct(y, x)

    @staticmethod
    def _construct(y, x=None):
        tup = (y, dsa_g, dsa_p, dsa_q, x) if x else (y, dsa_g, dsa_p, dsa_q)
        return DSA.construct(tup)

    def _has_private(self):
        return self.key.has_private()

    def _to_public(self):
        return DSAKey(key=DSAKey._construct(self.key.y, None))

    def _get_pubkey(self):
        return int(self.key.y).to_bytes(128, 'big')

    def _get_privkey(self):
        return int(self.key.x).to_bytes(20, 'big')

    def _sign(self, data):
        """Generate DSA-SHA1 signature."""
        k = random().randint(1, self.key.q - 1)
        data = sha1(data)
        R, S = self.key.sign(data, k)
        return int(R).to_bytes(20, 'big') + int(S).to_bytes(20, 'big')

    def _verify(self, data, sig):
        """Verify DSA-SHA1 signature."""
        data = sha1(data)
        R, S = int.from_bytes(sig[:20], 'big'), int.from_bytes(sig[20:], 'big')
        return self.key.verify(data, (R, S))


class ECDSAKey(SigningKey):

    def __init__(self, key_type, pub=None, priv=None, key=None):
        """Construct an ECDSA signing key.

        With no arguments, generates a new ECDSAKey.
        If pub or priv are set, creates an ECDSAKey using provided key material.
        If key is set, creates an ECDSAKey using the provided key.
        """
        if key_type.base_algo != SigAlgo.EC:
            raise ValueError('Invalid key_type')
        super().__init__(key_type, pub, priv, key)

    def _parse(self, pub, priv=None):
        """Parse key data"""
        mid = int(self.key_type.pubkey_len/2)
        x = pub[:mid]
        y = pub[mid:]
        return ECC(pubkey_x=x, pubkey_y=y, raw_privkey=priv,
                   curve=self.key_type.spec)

    def _generate(self):
        """Generate an ECDSA key pair."""
        return ECC(curve=self.key_type.spec)

    def _has_private(self):
        return self.key.privkey is not None

    def _to_public(self):
        return ECDSAKey(self.key_type,
                        key=ECC(pubkey_x=self.key.pubkey_x,
                                pubkey_y=self.key.pubkey_y,
                                curve=self.key_type.spec))

    def _get_pubkey(self):
        pubkey = bytes()
        part_len = int(self.key_type.pubkey_len/2)
        pubkey += rectify(self.key.pubkey_x, part_len)
        pubkey += rectify(self.key.pubkey_y, part_len)
        return pubkey

    def _get_privkey(self):
        return self.key.privkey

    def _sign(self, data):
        """Generate ECDSA signature."""
        sig = self.key.sign(data)
        return asn1_to_sig_bytes(sig, self.key_type.sig_len)

    def _verify(self, data, sig):
        """Verify ECDSA signature."""
        return self.key.verify(sig_bytes_to_asn1(sig), data)

class ECDSA256Key(ECDSAKey):

    def __init__(self, pub=None, priv=None, key=None):
        """Construct an ECDSA-SHA256-P256 signing key.

        With no arguments, generates a new ECDSA256Key.
        If pub or priv are set, creates an ECDSA256Key using provided key material.
        If key is set, creates an ECDSA256Key using the provided key.
        """
        super().__init__(SigType.ECDSA_SHA256_P256, pub, priv, key)



class EdDSAKey(SigningKey):

    def __init__(self, pub=None, priv=None, key=None):
        """Construct and Ed25519-SHA512 signing key.

        With no arguments, generatea a new Ed25519Key.
        If pub or priv are set, creates an Ed25519Key using provided key material.
        If key is se, creates an Ed25519Key using the provided key.
        """
        super().__init__(SigType.EdDSA_SHA512_Ed25519, pub, priv, key)

    @staticmethod
    def _parse(pub, priv=None):
        """Parse key data"""
        if priv:
            # if we have private data consider it seed data
            pub, priv = libnacl.crypto_sign_seed_keypair(priv)
        return pub, priv

    @staticmethod
    def _generate():
        """Generate an Ed25519-SHA512 signing key pair
        """
        return libnacl.crypto_sign_keypair()
    

    def _has_private(self):
        return self.key[1] is not None

    def _to_public(self):
        return EdDSAKey(pub=self.key[0])

    def _get_pubkey(self):
        return self.key[0]

    def _get_privkey(self):
        return self.key[1][:libnacl.crypto_sign_SEEDBYTES]

    def _sign(self, data):
        """Generate EdDSA signature"""
        assert self.key[1] is not None
        assert len(self.key[1]) == libnacl.crypto_sign_SECRETKEYBYTES

        # compute sha512 of data
        h = libnacl.crypto_hash_sha512(data)
        # sign hash of data
        # crypto_sign makes a signed message we just want the signature
        smsg = libnacl.crypto_sign(h, self.key[1])
        return smsg[:0-len(h)]

    def _verify(self, data, sig):
        """Verify EdDSA signature."""
        assert self.key[0] is not None
        assert len(self.key[0]) == libnacl.crypto_sign_PUBLICKEYBYTES

        # compute sha512 of data
        h = libnacl.crypto_hash_sha512(data)
        # construct signed message to use with crypto_sign_open
        smsg = sig + h
        # check message integrity
        try:
            return libnacl.crypto_sign_open(smsg, self.key[0]) == h
        except:
            # if an exception happens it's not
            return False

        
#
# Algorithms
#
# Because of the way Enums are instantiated, EncType and SigType must be
# defined after the Keys they reference.
#

class EncAlgo(Enum):
    ELGAMAL = "ElGamal"
    EC = "EC"


class SigAlgo(Enum):
    DSA = "DSA"
    EC = "EC"
    EdDSA = "EdDSA"
    RSA = "RSA"


class EncType(Enum):
    ELGAMAL_2048 = (0, 256, 256, EncAlgo.ELGAMAL, "ElGamal/None/NoPadding", ELGAMAL_2048_SPEC, "0", ElGamalKey)
    EC_P256 = (1, 64, 32, EncAlgo.EC, "EC/None/NoPadding", P256_SPEC, "0.9.20", None)
    EC_P384 = (2, 96, 48, EncAlgo.EC, "EC/None/NoPadding", P384_SPEC, "0.9.20", None)
    EC_P521 = (3, 132, 66, EncAlgo.EC, "EC/None/NoPadding", P521_SPEC, "0.9.20", None)

    @property
    def code(self):
        return self.value[0]

    @property
    def pubkey_len(self):
        return self.value[1]

    @property
    def privkey_len(self):
        return self.value[2]

    @property
    def base_algo(self):
        return self.value[3]

    @property
    def algo_name(self):
        return self.value[4]

    @property
    def spec(self):
        return self.value[5]

    @property
    def since(self):
        return self.value[6]

    @property
    def cls(self):
        if self.value[7] is None:
            raise NotImplementedError('Unsupported encryption type')
        return self.value[7]

    @property
    def is_available(self):
        return self.spec is not None

    @staticmethod
    def get_by_code(code):
        for enc in EncType:
            if enc.code == code:
                return enc
        return None


class SigType(Enum):
    DSA_SHA1 = (0, 128, 20, 20, 40, SigAlgo.DSA, "SHA-1", "SHA1withDSA", DSA_SHA1_SPEC, "0", DSAKey)
    ECDSA_SHA256_P256 = (1, 64, 32, 32, 64, SigAlgo.EC, "SHA-256", "SHA256withECDSA", P256_SPEC, "0.9.12", ECDSA256Key)
    ECDSA_SHA384_P384 = (2, 96, 48, 48, 96, SigAlgo.EC, "SHA-384", "SHA384withECDSA", P384_SPEC, "0.9.12", None)
    ECDSA_SHA512_P521 = (3, 132, 66, 64, 132, SigAlgo.EC, "SHA-512", "SHA512withECDSA", P521_SPEC, "0.9.12", None)
    RSA_SHA256_2048 = (4, 256, 512, 32, 256, SigAlgo.RSA, "SHA-256", "SHA256withRSA", F4_2048_SPEC, "0.9.12", None)
    RSA_SHA384_3072 = (5, 384, 768, 48, 384, SigAlgo.RSA, "SHA-384", "SHA384withRSA", F4_3072_SPEC, "0.9.12", None)
    RSA_SHA512_4096 = (6, 512, 1024, 64, 512, SigAlgo.RSA, "SHA-512", "SHA512withRSA", F4_4096_SPEC, "0.9.12", None)
    EdDSA_SHA512_Ed25519 = (7, 32, 32, 64, 64, SigAlgo.EdDSA, "SHA-512", "SHA512withEdDSA", Ed25519_SHA_512_SPEC, "0.9.17", EdDSAKey)

    @property
    def code(self):
        return self.value[0]

    @property
    def pubkey_len(self):
        return self.value[1]

    @property
    def privkey_len(self):
        return self.value[2]

    @property
    def hash_len(self):
        return self.value[3]

    @property
    def sig_len(self):
        return self.value[4]

    @property
    def base_algo(self):
        return self.value[5]

    @property
    def digest_name(self):
        return self.value[6]

    @property
    def algo_name(self):
        return self.value[7]

    @property
    def spec(self):
        return self.value[8]

    @property
    def since(self):
        return self.value[9]

    @property
    def cls(self):
        if self.value[10] is None:
            raise NotImplementedError('Unsupported signature type')
        return self.value[10]

    @property
    def is_available(self):
        return self.spec is not None

    @staticmethod
    def get_by_code(code):
        for enc in SigType:
            if enc.code == code:
                return enc
        return None


def gen_elgamal_key(fname=None, fd=None):
    key = ElGamalKey()
    doclose = fd is None
    if doclose:
        fd = open(fname, 'wb')
    fd.write(key.serialize())
    if doclose:
        fd.close()


def gen_dsa_key(fname=None, fd=None):
    key = DSAKey()
    nofname = fd is None
    if nofname:
        fd = open(fname, 'wb')
    fd.write(key.serialize())
    if nofname:
        fd.close()


def load_dsa_key(fname):
    with open(fname, 'rb') as rf:
        return DSAKey(fd=rf)


def gen_keypair(fd):
    gen_elgamal_key(fd)
    gen_dsa_key(fd)


def dump_keypair(enckey, sigkey, fd):
    fd.write(enckey.serialize())
    fd.write(sigkey.serialize())


def sig_bytes_to_asn1(sig):
    part_len = int(len(sig)/2)
    r = int.from_bytes(sig[:part_len], 'big')
    s = int.from_bytes(sig[part_len:], 'big')
    der = asn1.DerSequence()
    der.append(r)
    der.append(s)
    return der.encode()


def asn1_to_sig_bytes(asn, tolen):
    der = asn1.DerSequence()
    der.decode(asn)
    sig = bytes()
    part_len = int(tolen/2)
    # Wrap with int() for Py2
    sig += int(der[0]).to_bytes(part_len, 'big')
    sig += int(der[1]).to_bytes(part_len, 'big')
    return sig


def rectify(part, tolen):
    return int.from_bytes(part, 'big').to_bytes(tolen, 'big')
