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

#include <memory.h>
#include <i2cp/destination.h>
#include <i2cp/crypto.h>
#include <i2cp/certificate.h>

#define TAG DESTINATION

typedef struct i2cp_destination_t 
{
  struct i2cp_certificate_t *certificate;
  i2cp_signature_keypair_t signature_keypair;
  uint8_t public_key[256];
  uint8_t digest[40];
  const char *b32;
  const char *b64;
} i2cp_destination_t;

static void
_destination_dtor(struct i2cp_destination_t *self)
{
  mpz_clear(self->signature_keypair.dsa_public);
  mpz_clear(self->signature_keypair.dsa_private);

  i2cp_certificate_destroy(self->certificate);

  free((char *)self->b32);

  free((char *)self->b64);

  free(self);
}

static  int
_destination_verify(struct i2cp_destination_t *self)
{
  stream_t s;

  stream_init(&s, 4096);
  i2cp_destination_get_message(self, &s);

  /* write digest to end */
  stream_out_uint8p(&s, self->digest, 40);

  return i2cp_crypto_verify_stream(i2cp_crypto_instance(), &self->signature_keypair, &s);
}

static void
_destination_generate_b32(struct i2cp_destination_t *self)
{
  stream_t stream, hash;

  /* generate b32 address of destination */
  stream_init(&stream, 4096);
  stream_init(&hash, 512);
  i2cp_destination_get_message(self, &stream);
  i2cp_crypto_hash_stream(i2cp_crypto_instance(), HASH_SHA256, &stream, &hash);
  stream_reset(&stream);
  i2cp_crypto_encode_stream(i2cp_crypto_instance(), CODEC_BASE32, &hash, &stream);
  stream_out_uint8p(&stream, ".b32.i2p\0", sizeof(".b32.i2p\0"));
  self->b32 = strdup((char *)stream.data);

  stream_destroy(&stream);
  stream_destroy(&hash);

  debug(TAG, "New destination: %s", self->b32);
}

static void
_destination_generate_b64(struct i2cp_destination_t *self)
{
  char *b64;
  char *p;
  stream_t in, out;

  stream_init(&in, 4096);
  stream_init(&out, 4096);

  i2cp_destination_get_message(self, &in);
  i2cp_crypto_encode_stream(i2cp_crypto_instance(), CODEC_BASE64, &in, &out);
  b64 = strdup((char *)out.data);

  /* urlify base64 encoding */
  p = b64;
  while (*p++)
    if (*p == '/') *p = '~';
    else if (*p == '+') *p = '-';

  stream_destroy(&in);
  stream_destroy(&out);

  self->b64 = b64;
}

struct i2cp_destination_t *
i2cp_destination_copy(const struct i2cp_destination_t *src)
{
  i2cp_destination_t *dest;

  /* allocate destination */
  dest = malloc(sizeof(i2cp_destination_t));
  memset(dest, 0, sizeof(i2cp_destination_t));

  /* make a copy */
  dest->signature_keypair = src->signature_keypair;
  memcpy(dest->public_key, src->public_key, sizeof(dest->public_key));
  memcpy(dest->digest, src->digest, sizeof(dest->digest));

  dest->certificate = i2cp_certificate_copy(src->certificate);
  dest->b32 = strdup(src->b32);
  dest->b64 = strdup(src->b64);

  return dest;
}

struct i2cp_destination_t *
i2cp_destination_new()
{
  i2cp_destination_t *dest;

  /* construct the destination */
  dest = malloc(sizeof(i2cp_destination_t));
  memset(dest, 0, sizeof(i2cp_destination_t));
  dest->certificate = i2cp_certificate_new(CERTIFICATE_NULL);

  /* generate signature keypair for the new destination */
  i2cp_crypto_signature_keygen(i2cp_crypto_instance(), DSA_SHA1, &dest->signature_keypair);

  /* generate b32 and b64 addresss */
  _destination_generate_b32(dest);
  _destination_generate_b64(dest);

  return dest;
}

struct i2cp_destination_t *
i2cp_destination_new_from_message(stream_t *stream)
{
  uint8_t signpubkey[128];

  i2cp_destination_t *dest = malloc(sizeof(i2cp_destination_t));
  memset(dest, 0, sizeof(i2cp_destination_t));

  /* read the public key from stream */
  stream_in_uint8p(stream, dest->public_key, 256);

  /* read public signature key from stream*/
  stream_in_uint8p(stream, signpubkey, 128);
  dest->signature_keypair.type = DSA_SHA1;
  mpz_init(dest->signature_keypair.dsa_public);
  mpz_import(dest->signature_keypair.dsa_public, 128, 1, 1, 0, 0, signpubkey);

  /* construct certificate from stream */
  dest->certificate = i2cp_certificate_new_from_message(stream);
  if (dest->certificate == NULL)
  {
    _destination_dtor(dest);
    return NULL;
  }

  /* generate b32 and b64 addresses */
  _destination_generate_b32(dest);
  _destination_generate_b64(dest);

  return dest;
}

struct i2cp_destination_t *
i2cp_destination_new_from_stream(stream_t *stream)
{
  uint16_t plen;
  i2cp_destination_t *dest;

  if (stream_length(stream) == 0)
    return NULL;

  /* read and parse the stream into a destination instance */
  dest = malloc(sizeof(i2cp_destination_t));
  memset(dest, 0, sizeof(i2cp_destination_t));

  /* read certificate from stream */
  dest->certificate = i2cp_certificate_new_from_stream(stream);

  /* read signature keypair from stream */
  i2cp_crypto_signature_keypair_from_stream(i2cp_crypto_instance(),
					    &dest->signature_keypair, stream);

  /* read public key from stream */
  stream_in_uint16(stream, plen);
  if (plen != 256)
    fatal(TAG, "Failed to load public key len, %d != 256.", plen);
  stream_in_uint8p(stream, dest->public_key, 256);

  /* generate b32 and b64 addresses */
  _destination_generate_b32(dest);
  _destination_generate_b64(dest);

  return dest;
}

struct i2cp_destination_t *i2cp_destination_new_from_file(const char *filename)
{
  stream_t stream;
  i2cp_destination_t *dest;

  /* load destination into stream */
  stream_init(&stream, 4096);
  stream_load(&stream, filename);
  if (stream_length(&stream) == 0)
    return NULL;

  /* instantiate destination from stream */
  dest = i2cp_destination_new_from_stream(&stream);

  return dest;
}


/** \todo Fix encryption keypair to work the same way as for signature keypairs.
 */
void
i2cp_destination_to_stream(struct i2cp_destination_t *self, stream_t *stream)
{
  /* write certificate to stream */
  i2cp_certificate_to_stream(self->certificate, stream);

  /* write signature keypair to stream */
  i2cp_crypto_signature_keypair_to_stream(i2cp_crypto_instance(),
					  &self->signature_keypair, stream);

  /* write public key to stream */
  stream_out_uint16(stream, 256);
  stream_out_uint8p(stream, self->public_key, 256);
  stream_mark_end(stream);
}

void i2cp_destination_save(struct i2cp_destination_t *self, const char *filename)
{
  stream_t stream;
  stream_init(&stream, 4096);

  /* serialize destination into stream */
  i2cp_destination_to_stream(self, &stream);

  /* save stream to file */
  stream_dump(&stream, filename);

  stream_destroy(&stream);
}


struct i2cp_destination_t *i2cp_destination_new_from_base64(const char *base64)
{
  uint8_t *p;
  stream_t in, out;
  i2cp_destination_t *dest;

  stream_init(&in, 4096);
  stream_init(&out, 4096);

  /* write base64 into in stream and unurlify */
  stream_out_uint8p(&in, base64, strlen(base64));
  stream_mark_end(&in);

  /* urlify base64 encoding */
  p = in.data;
  while (*p++)
    if (*p == '~') *p = '/';
    else if (*p == '-') *p = '+';

  stream_seek_set(&in, 0);
  i2cp_crypto_decode_stream(i2cp_crypto_instance(), CODEC_BASE64, &in, &out);

  stream_seek_set(&out, 0);
  dest = i2cp_destination_new_from_message(&out);

  stream_destroy(&in);
  stream_destroy(&out);
  return dest;
}

void i2cp_destination_destroy(struct i2cp_destination_t *self)
{
  _destination_dtor(self);
}

void i2cp_destination_get_message(struct i2cp_destination_t *self, stream_t *stream)
{
  /* write public key */
  stream_out_uint8p(stream, self->public_key, 256);

  /* write sign pubkey */
  i2cp_crypto_signature_publickey_stream(i2cp_crypto_instance(), &self->signature_keypair, stream);

  /* write certificate */
  i2cp_certificate_get_message(self->certificate, stream);

  stream_mark_end(stream);
}

const i2cp_signature_keypair_t *
i2cp_destination_signature_keypair(struct i2cp_destination_t *self)
{
  return &self->signature_keypair;
}

const char *
i2cp_destination_b32(const struct i2cp_destination_t *self)
{
  return self->b32;
}

const char *
i2cp_destination_b64(const struct i2cp_destination_t *self)
{
  return self->b64;
}
