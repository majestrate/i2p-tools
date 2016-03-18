from __future__ import absolute_import, division, print_function, unicode_literals
from builtins import *
from unittest import TestCase

from i2p import crypto


class KeyMixin(object):

    def setUp(self):
        self.key = self.cls()
        self.data = 'test 12345'.encode('utf-8')

    def test_to_public(self):
        assert self.key.has_private()
        pubkey = self.key.to_public()
        assert not pubkey.has_private()

    def test_serialize_and_parse(self):
        pubkey = self.key.get_pubkey()
        privkey = self.key.get_privkey()
        key2 = self.cls(pubkey, privkey)


class SigningKeyMixin(KeyMixin):

    def test_sign_verify_valid(self):
        assert self.key is not None

        sig = self.key.sign(self.data)
        assert sig is not None
        assert len(sig) == self.key.key_type.sig_len

        assert self.key.verify(self.data, sig)

    def test_sign_verify_invalid(self):
        assert self.key is not None

        badsig = b'\x00' * self.key.key_type.sig_len

        assert not self.key.verify(self.data, badsig)


class TestDSAKey(SigningKeyMixin, TestCase):

    cls = crypto.DSAKey

class TestEd25519Key(SigningKeyMixin, TestCase):

    cls = crypto.EdDSAKey
    
class TestECDSA256Key(SigningKeyMixin, TestCase):

    cls = crypto.ECDSA256Key

    def test_serialize_and_parse_stress(self):
        # Sometimes test_serialize_and_parse() fails with wrong pubkey length.
        # This test should make it happen more frequently.
        for i in range(0, 1000):
            key = crypto.ECDSA256Key()
            pubkey = self.key.get_pubkey()
            privkey = self.key.get_privkey()
            key2 = self.cls(pubkey, privkey)
