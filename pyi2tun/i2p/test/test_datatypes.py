from __future__ import absolute_import, division, print_function, unicode_literals
from builtins import *
from unittest import TestCase

from i2p import crypto, datatypes

DSA_ELGAMAL_KEY_CERT = b'BQAEAAAAAA=='
DSA_ELGAMAL_KEY_CERT_PAYLOAD = b'AAAAAA=='
# stats.i2p
DEST_DSA_B64 = 'Okd5sN9hFWx-sr0HH8EFaxkeIMi6PC5eGTcjM1KB7uQ0ffCUJ2nVKzcsKZFHQc7pLONjOs2LmG5H-2SheVH504EfLZnoB7vxoamhOMENnDABkIRGGoRisc5AcJXQ759LraLRdiGSR0WTHQ0O1TU0hAz7vAv3SOaDp9OwNDr9u902qFzzTKjUTG5vMTayjTkLo2kOwi6NVchDeEj9M7mjj5ySgySbD48QpzBgcqw1R27oIoHQmjgbtbmV2sBL-2Tpyh3lRe1Vip0-K0Sf4D-Zv78MzSh8ibdxNcZACmZiVODpgMj2ejWJHxAEz41RsfBpazPV0d38Mfg4wzaS95R5hBBo6SdAM4h5vcZ5ESRiheLxJbW0vBpLRd4mNvtKOrcEtyCvtvsP3FpA-6IKVswyZpHgr3wn6ndDHiVCiLAQZws4MsIUE1nkfxKpKtAnFZtPrrB8eh7QO9CkH2JBhj7bG0ED6mV5~X5iqi52UpsZ8gnjZTgyG5pOF8RcFrk86kHxAAAA'
DEST_DSA_B32 = '7tbay5p4kzeekxvyvbf6v7eauazemsnnl2aoyqhg5jzpr5eke7tq.b32.i2p'
# tracker.thebland.i2p
DEST_ECDSA_P256_B64 = 'gzBtMSRcMD6b36PmPCQWZhh30fYm2Ww2r4tRSref4N2T4~cnXK3DjJOuJwao2jRK4bZwX2Rkyjw849xrFMqaR3SdPe3-K61B~Kr9Uo1KLdm3~oahOWFmCaIlipPs-i3jdTT~721YUcYB09n4PGrDq5KZSOOBlLZKulJficO58QRUlDpva4OCCRrX9EUCoAavOciKpvKtnGwl6AiPFu8WnmEeGQ861vjdirjfkHWNp3gj9IjGuxJNcgyHi51BWYZM6il~LJTcbA4zuZn~qudHIx9uzUtO-t08yzSRrmfVwVVVru6-~BBX0ipADi9UGZjyB-PJEKKjizUPxSp2OCmiOlQ2iXpKs2j8yfjHJbn-eWKpIh4jfpNigy6AbDfzFivkvm8lt8CleYf-p3~SHdqIL0iEaacxi5BAU4Baj5yS818kPQP4hEEMMtq4WnKjl4IW64swXSg1wlVBTiKDJzzQGK20jySBuPxhEbd6sfAeirzn585g5EqeV8DLqsMfe5pZBQAEAAEAAA=='
DEST_ECDSA_P256_B32 = 's5ikrdyjwbcgxmqetxb3nyheizftms7euacuub2hic7defkh3xhq.b32.i2p'


def assert_KeyCert_DSA_ElGamal(cert):
    assert len(cert.data) == 4
    assert cert.sigtype == crypto.SigType.DSA_SHA1
    assert cert.enctype == crypto.EncType.ELGAMAL_2048
    assert len(cert.extra_sigkey_data) == 0
    assert len(cert.extra_enckey_data) == 0


class TestKeyCertificate(TestCase):

    def test_create_from_keys(self):
        cert = datatypes.KeyCertificate(crypto.DSAKey(),
                                        crypto.ElGamalKey())
        assert_KeyCert_DSA_ElGamal(cert)

    def test_parse(self):
        cert = datatypes.KeyCertificate(raw=DSA_ELGAMAL_KEY_CERT, b64=True)
        assert_KeyCert_DSA_ElGamal(cert)

    def test_create_and_serialize(self):
        cert = datatypes.KeyCertificate(data=DSA_ELGAMAL_KEY_CERT_PAYLOAD, b64=True)
        assert_KeyCert_DSA_ElGamal(cert)
        assert cert.serialize(True) == DSA_ELGAMAL_KEY_CERT


class TestDestination(TestCase):

    def test_generate_default(self):
        dest = datatypes.Destination()
        assert dest.enckey.key_type == crypto.EncType.ELGAMAL_2048
        assert dest.sigkey.key_type == crypto.SigType.DSA_SHA1
        assert dest.cert.type == datatypes.CertificateType.NULL
        dest2 = datatypes.Destination()
        assert dest2.enckey.key.y != dest.enckey.key.y
        assert dest2.sigkey.key.y != dest.sigkey.key.y

    def test_generate_specify_types(self):
        dest = datatypes.Destination(crypto.EncType.ELGAMAL_2048, crypto.SigType.DSA_SHA1)
        assert dest.enckey.key_type == crypto.EncType.ELGAMAL_2048
        assert dest.sigkey.key_type == crypto.SigType.DSA_SHA1
        assert dest.cert.type == datatypes.CertificateType.NULL

        dest = datatypes.Destination(sigkey=crypto.SigType.ECDSA_SHA256_P256)
        self._assert_keycert(dest, crypto.EncType.ELGAMAL_2048,
                                   crypto.SigType.ECDSA_SHA256_P256)
        #dest = datatypes.Destination(crypto.EncType.EC_P256)
        #self._assert_keycert(dest, crypto.EncType.EC_P256,
        #                           crypto.SigType.DSA_SHA1)
        #dest = datatypes.Destination(crypto.EncType.EC_P256,
        #                             crypto.SigType.ECDSA_SHA256_P256)
        #self._assert_keycert(dest, crypto.EncType.EC_P256,
        #                           crypto.SigType.ECDSA_SHA256_P256)

    def test_generate_from_keycert(self):
        keycert = datatypes.KeyCertificate(crypto.DSAKey(),
                                           crypto.ElGamalKey())
        dest = datatypes.Destination(cert=keycert)
        assert dest.enckey.key_type == crypto.EncType.ELGAMAL_2048
        assert dest.sigkey.key_type == crypto.SigType.DSA_SHA1
        assert dest.cert.type == datatypes.CertificateType.NULL

        keycert = datatypes.KeyCertificate(crypto.ECDSA256Key(),
                                           crypto.ElGamalKey())
        dest = datatypes.Destination(cert=keycert)
        self._assert_keycert(dest, crypto.EncType.ELGAMAL_2048,
                                   crypto.SigType.ECDSA_SHA256_P256)

    def _assert_keycert(self, dest, enctype, sigtype):
        assert dest.enckey.key_type == enctype
        assert dest.sigkey.key_type == sigtype
        assert dest.cert.type == datatypes.CertificateType.KEY
        assert dest.cert.sigtype == sigtype
        assert dest.cert.enctype == enctype

    def TODO_test_parse_eeppriv(self):
        with open(testkey, 'rb') as rf:
            dest = datatypes.Destination(raw=rf)

    def test_parse_b64(self):
        self._test_parse_b64(DEST_DSA_B64, datatypes.CertificateType.NULL, 0)
        self._test_parse_b64(DEST_ECDSA_P256_B64, datatypes.CertificateType.KEY, 4)

    def _test_parse_b64(self, b64, cert_type, data_len):
        dest = datatypes.Destination(raw=b64, b64=True)
        assert dest.cert.type == cert_type
        assert len(dest.cert.data) == data_len

    def test_serialize_nullcert(self):
        dest = datatypes.Destination(crypto.ElGamalKey(), crypto.DSAKey())
        assert dest.cert.type == datatypes.CertificateType.NULL
        data = dest.serialize()
        dest2 = datatypes.Destination(raw=data)
        assert dest2.enckey.key.y == dest.enckey.key.y
        assert dest2.sigkey.key.y == dest.sigkey.key.y
        assert dest2.cert.type == dest.cert.type
        assert dest2.padding == dest.padding

    def test_serialize_keycert(self):
        dest = datatypes.Destination(crypto.ElGamalKey(), crypto.ECDSA256Key())
        assert dest.cert.type == datatypes.CertificateType.KEY
        data = dest.serialize()
        dest2 = datatypes.Destination(raw=data)
        assert dest2.enckey.key.y == dest.enckey.key.y
        assert dest2.sigkey.key.get_pubkey() == dest.sigkey.key.get_pubkey()
        assert dest2.cert.type == dest.cert.type
        assert dest2.padding == dest.padding

    def test_base64(self):
        self._test_base64(DEST_DSA_B64)
        self._test_base64(DEST_ECDSA_P256_B64)

    def _test_base64(self, b64):
        dest = datatypes.Destination(raw=b64, b64=True)
        assert dest.base64() == b64

    def test_base32(self):
        self._test_base32(DEST_DSA_B64, DEST_DSA_B32)
        self._test_base32(DEST_ECDSA_P256_B64, DEST_ECDSA_P256_B32)

    def _test_base32(self, b64, b32):
        dest = datatypes.Destination(raw=b64, b64=True)
        assert dest.base32() == b32


class TestLeaseSet(TestCase):

    def test_serialize(self):
        dest = datatypes.Destination(crypto.ElGamalKey(), crypto.DSAKey(), datatypes.Certificate())
        lease = datatypes.Lease(b'f'*32, 1, datatypes.Date(1))
        ls = datatypes.LeaseSet(dest=dest, ls_enckey=crypto.ElGamalKey(), ls_sigkey=crypto.DSAKey(), leases=[lease])
        data = ls.serialize()
        dest.verify(data[:-40], data[-40:])

    def test_parse(self):
        dest = datatypes.Destination(crypto.ElGamalKey(), crypto.DSAKey(), datatypes.Certificate())
        lease = datatypes.Lease(b'f'*32, 1, datatypes.Date(1))
        ls = datatypes.LeaseSet(dest=dest, ls_enckey=crypto.ElGamalKey(), ls_sigkey=crypto.DSAKey(), leases=[lease])
        data = ls.serialize()
        ls2 = datatypes.LeaseSet(raw=data)
        assert ls2.dest.base64() == ls.dest.base64()
        assert ls2.enckey.key.y == ls.enckey.key.y
        assert ls2.sigkey.key.y == ls.sigkey.key.y
        assert len(ls2.leases) == len(ls.leases)
