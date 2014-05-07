/*
  Part of i2cp C library
  Copyright (C) 2013 Oliver Queen <oliver@mail.i2p>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>

#include <nettle/sha1.h>
#include <nettle/sha2.h>
#include <nettle/base64.h>
#include <gmp.h>

#include <i2cp/crypto.h>

#define TAG CRYPTO

typedef struct i2cp_crypto_t
{
  struct {
    mpz_t q;
    mpz_t p;
    mpz_t g;
   } dsa;

  /** \brief GMP random state */
  gmp_randstate_t random;

} i2cp_crypto_t;

static i2cp_crypto_t *_crypto;

static void
_encode_base64_stream(i2cp_crypto_t *self, stream_t *src, stream_t *dest)
{
  base64_encode_raw(dest->data, stream_length(src), src->data);
  dest->p += strlen((char *)dest->data);
  stream_mark_end(dest);
}

static void
_decode_base64_stream(i2cp_crypto_t *self, stream_t *src, stream_t *dest)
{
  int ret;
  unsigned dest_length;
  struct base64_decode_ctx b64;

  dest_length = stream_size(dest);

  base64_decode_init(&b64);
  ret = base64_decode_update(&b64, &dest_length, dest->data, stream_length(src), src->data);
  base64_decode_final(&b64);

  if (ret != 1)
    fatal(TAG|FATAL, "%s", "Failed to decode base64 input stream.");

  dest->p += dest_length;
  stream_mark_end(dest);
}

static void
_encode_base32_stream(i2cp_crypto_t *self, stream_t *src, stream_t *dest)
{
  uint8_t in;
  int length;
  int buffer;
  int bitsLeft;
  int pad;
  int index;

  length = stream_length(src);
  if (length < 0 || length > (1 << 28))
    return;

  stream_seek_set(src, 0);
  stream_reset(dest);

  if (length > 0)
  {
    stream_in_uint8(src, in);
    buffer = in;
    bitsLeft = 8;
    while ((bitsLeft > 0 || src->p < src->end))
    {
      if (bitsLeft < 5)
      {
        if (src->p < src->end)
	{
	  stream_in_uint8(src, in);
          buffer <<= 8;
          buffer |= in & 0xFF;
          bitsLeft += 8;
        }
	else
	{
          pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      index = 0x1F & (buffer >> (bitsLeft - 5));
      bitsLeft -= 5;
      stream_out_uint8(dest,  "abcdefghijklmnopqrstuvwxyz234567"[index]);
    }
  }

  stream_mark_end(dest);
}

static void
_decode_base32_stream(i2cp_crypto_t *self, stream_t *src, stream_t *dest)
{
  uint8_t ch = 0;
  uint32_t bitsLeft = 0;
  uint32_t buffer = 0;

  while (!stream_eof(src))
  {
    stream_in_uint8(src, ch);
    buffer <<= 5;
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
      ch = (ch & 0x1f) - 1;
    else if (ch >= '2' && ch <= '7')
      ch -= '2' - 26;
    else
    {
      stream_reset(dest);
      return;
    }

    buffer |= ch;
    bitsLeft += 5;
    if (bitsLeft >= 8)
    {
      stream_out_uint8(dest, buffer >> (bitsLeft - 8));
      bitsLeft -= 8;
    }
  }

  if (stream_length(dest) < stream_length(src))
    stream_out_uint8(dest, 0);

  stream_mark_end(dest);
}

static void 
_dsa_signature_to_byte_array(mpz_t r, mpz_t s, uint8_t *array, size_t size)
{
  int i;
  char *rs;
  char *ss;
  char digest[81];
  char *p;

  /* compose hexstring of r and s*/
  for (i = 0; i < 81; i++) 
    digest[i] = '0';

  rs = mpz_get_str(NULL, 16, r);
  if (strlen(rs) > 42)
    fatal(TAG|FATAL, "dsa digest r > %d bytes", 21);

  if (strlen(rs) == 40)
    memcpy(digest, rs, 40);
  else if(strlen(rs) == 42)
    for (i = 0; i < strlen(rs); i++) digest[i] = rs[i+2];
  else
    for (i = 0; i < strlen(rs); i++) digest[i + 40 - strlen(rs)] = rs[i];

  ss = mpz_get_str(NULL, 16, s);
  if (strlen(ss) > 42)
    fatal(TAG|FATAL, "dsa digest s > %d bytes", 21);

  if (strlen(ss) == 40)
    memcpy(digest+40, ss, 40);
  else if(strlen(rs) == 42)
    for (i = 0; i < strlen(ss); i++) digest[i + 40] = ss[i+2];
  else
    for (i = 0; i < strlen(rs); i++) digest[i + 40 + 40 - strlen(ss)] = ss[i];

  digest[80] = '\0';

  debug(TAG, "dsa digest: %s", digest);

  /* convert digest hex string to byte array */
  if ((strlen(digest)/2) != size)
    fatal(TAG|FATAL, "failed to convert hex string to byte array, size %ld != digest %d", size, strlen(digest)/2);

  p = digest;
  for (i = 0; i < size; i++)
  {
    sscanf(p, "%2hhx", &array[i]);
    p += 2;
  }
}


static int
_dsa_sha1_sign(i2cp_crypto_t *self,
	       const i2cp_signature_keypair_t *keypair,
	       uint8_t *in, size_t ilen,
	       uint8_t *out, size_t olen)
{
  struct sha1_ctx sha1;
  uint8_t hash[20];

  mpz_t k;
  mpz_t kinv;
  mpz_t r;
  mpz_t s;
  mpz_t m;
  mpz_t tmp;

  mpz_init(k);
  mpz_init(kinv);
  mpz_init(r);
  mpz_init(s);
  mpz_init(m);
  mpz_init(tmp);

  /* calculate sha1 hash of input into m */
  sha1_init(&sha1);
  sha1_update(&sha1, ilen, in);
  sha1_digest(&sha1, SHA1_DIGEST_SIZE, hash);
  mpz_import(m, SHA1_DIGEST_SIZE, 1, 1, 0, 0, hash);

  /* randomize k, make sure its smaller then q */
restart_calc:
  do {
    mpz_urandomb(k, self->random, 160);
  } while(mpz_sizeinbase(k, 10) > mpz_sizeinbase(self->dsa.q, 10));

  /* calculate r */
  mpz_powm(tmp, self->dsa.g, k, self->dsa.p);
  mpz_mod(r, tmp, self->dsa.q);
  if (mpz_cmp_ui(r, 0) == 0)
    goto restart_calc;

  /* calculate s */
  mpz_invert(kinv, k, self->dsa.q);
  mpz_mul(tmp, keypair->dsa_private, r);
  mpz_add(tmp, m, tmp);
  mpz_mul(tmp, kinv, tmp);
  mpz_mod(s, tmp, self->dsa.q);

  if (mpz_cmp_ui(s, 0) == 0)
    goto restart_calc;

  /* generate */
 
 /* TODO: This could be done a lot more prettier using
     mpz_export() and shift results in byte array*/
  _dsa_signature_to_byte_array(r, s, out, olen);

  /* create */
  mpz_clear(m);
  mpz_clear(kinv);
  mpz_clear(k);
  mpz_clear(r);
  mpz_clear(s);

  return olen;
}

static int
_dsa_sha1_verify(i2cp_crypto_t *self,
			const i2cp_signature_keypair_t *keypair,
			uint8_t *data, size_t len,
			uint8_t *digest)
{
  int ok;
  struct sha1_ctx sha1;
  uint8_t hash[20];

  mpz_t r, s, m, w, u1, u2, tmp1, tmp2;

  mpz_init(r);
  mpz_init(s);
  mpz_init(m);
  mpz_init(w);
  mpz_init(u1);
  mpz_init(u2);
  mpz_init(tmp1);
  mpz_init(tmp2);

  ok = 0;

  /* split digest into r and s*/
  mpz_import(r, 20, 1, 1, 0, 0, digest);
  mpz_import(s, 20, 1, 1, 0, 0, digest + 20);

  /* calculate sha1 hash of data */
  sha1_init(&sha1);
  sha1_update(&sha1, len, data);
  sha1_digest(&sha1, SHA1_DIGEST_SIZE, hash);
  mpz_import(m, SHA1_DIGEST_SIZE, 1, 1, 0, 0, hash);

  /* verify signature */
  mpz_invert(w, s, self->dsa.q);
  mpz_mul(tmp1, m, w); mpz_mod(u1, tmp1, self->dsa.q);
  mpz_mul(tmp1, r, w); mpz_mod(u2, tmp1, self->dsa.q);
  mpz_powm(tmp1, self->dsa.g, u1, self->dsa.p);
  mpz_powm(tmp2, keypair->dsa_public, u2, self->dsa.p);
  mpz_mul(tmp1, tmp1, tmp2);
  mpz_mod(tmp1, tmp1, self->dsa.p);
  mpz_mod(tmp1, tmp1, self->dsa.q);
  
  if (mpz_cmp(tmp1, r) == 0)
    ok = 1;

  mpz_clear(r);
  mpz_clear(s);
  mpz_clear(m);
  mpz_clear(w);
  mpz_clear(u1);
  mpz_clear(u2);
  mpz_clear(tmp1);
  mpz_clear(tmp2);

  return ok;
}

i2cp_crypto_t *
i2cp_crypto_instance()
{
  struct timeval tp;
  unsigned long seed;

  if (_crypto == NULL)
  {
    /* initialize the crypto singelton context */
    _crypto = malloc(sizeof(i2cp_crypto_t));
    memset(_crypto, 0, sizeof(i2cp_crypto_t));

    /* setup dsa-sha1 constants */
    /* prime, 1024 bits*/
    mpz_init_set_str(_crypto->dsa.p,
		     "9C05B2AA960D9B97B8931963C9CC9E8C3026E9B8ED92FAD0"
		     "A69CC886D5BF8015FCADAE31A0AD18FAB3F01B00A358DE23"
		     "7655C4964AFAA2B337E96AD316B9FB1CC564B5AEC5B69A9F"
		     "F6C3E4548707FEF8503D91DD8602E867E6D35D2235C1869C"
		     "E2479C3B9D5401DE04E0727FB33D6511285D4CF29538D9E3"
		     "B6051F5B22CC1C93",16);
    
    /* quointient, 160 bits*/
    mpz_init_set_str(_crypto->dsa.q, "A5DFC28FEF4CA1E286744CD8EED9D29D684046B7",16);

    /* generator */
    mpz_init_set_str(_crypto->dsa.g,
		     "0C1F4D27D40093B429E962D7223824E0BBC47E7C832A3923"
		     "6FC683AF84889581075FF9082ED32353D4374D7301CDA1D2"
		     "3C431F4698599DDA02451824FF369752593647CC3DDC197D"
		     "E985E43D136CDCFC6BD5409CD2F450821142A5E6F8EB1C3A"
		     "B5D0484B8129FCF17BCE4F7F33321C3CB3DBB14A905E7B2B"
		     "3E93BE4708CBCC82",16);

    /* initialize random */
    gettimeofday(&tp, NULL);
    seed = tp.tv_usec + rand() + tp.tv_sec  % 0xffffffff;
    gmp_randinit_default(_crypto->random);
    gmp_randseed_ui(_crypto->random, seed);
  }

  return _crypto;
}


void 
i2cp_crypto_sign_stream(struct i2cp_crypto_t *self,
			const i2cp_signature_keypair_t *keypair,
			stream_t *stream)
{
  int length;

  if (keypair->type == DSA_SHA1)
  {
    length = _dsa_sha1_sign(self, keypair, stream->data, stream_length(stream), stream->p, 40);
    stream->p += length;
  }

  if (length != 40)
    fatal(TAG|FATAL, "failed to sign stream, digest length %d != 40.", length);

  stream_mark_end(stream);

}

void
i2cp_crypto_signature_publickey_stream(struct i2cp_crypto_t *self,
				       const i2cp_signature_keypair_t *keypair,
				       stream_t *stream)
{
  size_t bytes;
  bytes = 0;

  if (keypair->type == DSA_SHA1)
  {
    mpz_export(stream->p, &bytes, 1, 1, 0, 0, keypair->dsa_public);
    if (bytes != 128)
      fatal(TAG|FATAL, "Sign pubkey length %d != 128 bytes", bytes);
  }
  else
    fatal(TAG|FATAL, "%s", "Unknown signature algorithm.");

  stream->p += bytes;
  stream_mark_end(stream);

}

int
i2cp_crypto_verify_stream(struct i2cp_crypto_t *self, 
			  const i2cp_signature_keypair_t *keypair,
			  stream_t *stream)
{
  int ret = 0;

  if (keypair->type == DSA_SHA1)
  {
    if (stream_length(stream) < 40)
      fatal(TAG|FATAL, "%s", "Stream length < 40 bytes (eg. the signature length)");

    ret = _dsa_sha1_verify(self, keypair,
			   stream->data, stream_length(stream) - 40, stream->end - 40);
  }
  else
    fatal(TAG|FATAL, "%s", "verify using an unsupported algorithm.");

  if (ret != 1)
    fatal(TAG|FATAL, "%s", "failed to verify stream.");

  return ret;
}

void
i2cp_crypto_signature_keygen(struct i2cp_crypto_t *self,
			     i2cp_signature_algorithm_t type,
			     i2cp_signature_keypair_t *keypair)
{
  struct timeval tp;
  unsigned long seed;
  keypair->type = type;

  gettimeofday(&tp, NULL);
  srand(tp.tv_usec  + rand() + tp.tv_usec % 0xffffffff);

  if (type == DSA_SHA1)
  {
    mpz_init(keypair->dsa_public);
    mpz_init(keypair->dsa_private);

    /* randomize private key, make sure its smaller then q */
    do {
      seed = rand() % 0xffffffff;
      gmp_randseed_ui(_crypto->random, seed);
      mpz_urandomb(keypair->dsa_private, self->random, 160);
    } while(mpz_sizeinbase(keypair->dsa_private, 10) > mpz_sizeinbase(keypair->dsa_private, 10));

    /* calculate public key */
    mpz_powm(keypair->dsa_public, self->dsa.g, keypair->dsa_private, self->dsa.p);
  }
  else
    fatal(TAG|FATAL, "%s", "Request generating keypair of unsupported algorithm.");

}

void
i2cp_crypto_signature_keypair_to_stream(struct i2cp_crypto_t *self,
					const i2cp_signature_keypair_t *keypair,
					stream_t *stream)
{
  size_t bytes;

  /* write signature type */
  stream_out_uint32(stream, keypair->type);

  if (keypair->type == DSA_SHA1)
  {
    /* write private key */
    mpz_export(stream->p, &bytes, 1, 1, 0, 0, keypair->dsa_private);
    if (bytes != 20)
      fatal(TAG, "failed to export signature private key, %d != 20", bytes);
    stream->p += bytes;

    /* write public key */
    mpz_export(stream->p, &bytes, 1, 1, 0, 0, keypair->dsa_public);
    if (bytes != 128)
      fatal(TAG, "failed to export signature private key, %d != 128", bytes);
    stream->p += bytes;
  }
  else
    fatal(TAG, "Failed to write unsupported signature keypair to stream.");

  stream_mark_end(stream);
}

void
i2cp_crypto_signature_keypair_from_stream(struct i2cp_crypto_t *self,
					  i2cp_signature_keypair_t *keypair,
					  stream_t *stream)
{
  uint8_t type;
  uint8_t blob[4096];

  mpz_init(keypair->dsa_public);
  mpz_init(keypair->dsa_private);

  stream_in_uint32(stream, keypair->type);
  if (keypair->type == DSA_SHA1)
  {
    /* read private key 20 bytes */
    stream_in_uint8p(stream, blob, 20);
    mpz_import(keypair->dsa_private, 20, 1, 1, 0, 0, blob);

    /* read public key 128 bytes */
    stream_in_uint8p(stream, blob, 128);
    mpz_import(keypair->dsa_public, 128, 1, 1, 0, 0, blob);

  }
  else
    fatal(TAG, "Failed to read signature keypair from stream, unsupported type.");
}

void
i2cp_crypto_hash_stream(struct i2cp_crypto_t *self, i2cp_hash_algorithm_t type, stream_t *src, stream_t *dest)
{
  uint8_t hash[4096];
  struct sha256_ctx sha;


  if (type == HASH_SHA256)
  {  
    /* calculate sha256 hash of src stream */
    sha256_init(&sha);
    sha256_update(&sha, stream_length(src), src->data);
    sha256_digest(&sha, SHA256_DIGEST_SIZE, hash);

    /* write to dest stream */
    stream_reset(dest);
    stream_out_uint8p(dest, hash, SHA256_DIGEST_SIZE);
    stream_mark_end(dest);
  }
  else
    fatal(TAG|FATAL, "%s", "Request of unsupported hash algorithm.");
}

void
i2cp_crypto_encode_stream(struct i2cp_crypto_t *self, i2cp_codec_algorithm_t type,
			  stream_t *src, stream_t *dest)
{
  if (type == CODEC_BASE32)
    _encode_base32_stream(self, src, dest);
  else if (type == CODEC_BASE64)
    _encode_base64_stream(self, src, dest);
  else
    fatal(TAG|FATAL, "%s", "Request of unsupported encode algorithm.");
}

void
i2cp_crypto_decode_stream(struct i2cp_crypto_t *self, i2cp_codec_algorithm_t type,
			  stream_t *src, stream_t *dest)
{
  if (type == CODEC_BASE32)
    _decode_base32_stream(self, src, dest);
  else if (type == CODEC_BASE64)
    _decode_base64_stream(self, src, dest);
  else
    fatal(TAG|FATAL, "%s", "Request of unsupported decode algorithm.");
}
