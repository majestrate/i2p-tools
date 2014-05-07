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

#include <i2cp/crypto.h>
#include <i2cp/destination.h>
#include <i2cp/stream.h>

#define TAG TEST

int _test_dsa_sign_and_verify()
{
  stream_t stream;
  i2cp_signature_keypair_t keypair;

  stream_init(&stream, 64);

  i2cp_crypto_signature_keygen(i2cp_crypto_instance(), DSA_SHA1, &keypair);
 
  /* first sign the stream */
  i2cp_crypto_sign_stream(i2cp_crypto_instance(), &keypair, &stream);

  /* then try to verify it */
  return i2cp_crypto_verify_stream(i2cp_crypto_instance(), &keypair, &stream);
}

int
_test_dsa_sign_and_verify_with_provided_key()
{
  i2cp_signature_keypair_t keypair;
  stream_t stream;
  stream_init(&stream, 4096);

  /* generate signature keypair */
  i2cp_crypto_signature_keygen(i2cp_crypto_instance(), DSA_SHA1, &keypair);

  /* store public signing key in stream */
  i2cp_crypto_signature_publickey_stream(i2cp_crypto_instance(), &keypair, &stream);

  /* sign stream */
  i2cp_crypto_sign_stream(i2cp_crypto_instance(), &keypair, &stream);

  /* verify stream using provided public signing key */
  return i2cp_crypto_verify_stream(i2cp_crypto_instance(), &keypair, &stream);
}

int _test_dsa_router_info_verify()
{
  struct i2cp_destination_t *dest;
  stream_t stream;

  /* load a netdb entry */
  stream_init(&stream, 4096);
  stream_load(&stream, "routerinfo.dat");
  stream_seek_set(&stream, 0);

  /* test to import destination from stream */
  dest = i2cp_destination_new_from_message(&stream);
  if (dest == NULL)
    return 0;

  /* verify stream  */
  return i2cp_crypto_verify_stream(i2cp_crypto_instance(), i2cp_destination_signature_keypair(dest), &stream);
}

int _test_dsa_broken_router_info_verify()
{
  struct i2cp_destination_t *dest;
  stream_t stream;

  /* load a netdb entry */
  stream_init(&stream, 4096);
  stream_load(&stream, "routerinfo_broken.dat");

  /* test to import destination from stream */
  dest = i2cp_destination_new_from_message(&stream);
  if (dest == NULL)
    return 0;

  /* verify stream  */
  return i2cp_crypto_verify_stream(i2cp_crypto_instance(), i2cp_destination_signature_keypair(dest), &stream);
}

const char *_codec_to_string(i2cp_codec_algorithm_t type)
{
  switch(type)
  {
  case CODEC_BASE32:
    return "base32";
  case CODEC_BASE64:
    return "base64";
  default:
    return "unknown";
  }
}

int _test_codec(i2cp_codec_algorithm_t type)
{
  uint8_t data[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
  stream_t in, out;

  stream_init(&in, 4096);
  stream_init(&out, 4096);

  stream_out_uint8p(&in, data, 8);
  stream_mark_end(&in);
  stream_seek_set(&in ,0);

  i2cp_crypto_encode_stream(i2cp_crypto_instance(), type, &in, &out);
  if (stream_length(&out) == 0)
    fatal(TAG, "Failed to %s encode input stream.", _codec_to_string(type));

  stream_reset(&in);
  stream_seek_set(&out, 0);
  i2cp_crypto_decode_stream(i2cp_crypto_instance(), type, &out, &in);
  if (stream_length(&in) == 0)
    fatal(TAG, "Failed to %s decode input stream.", _codec_to_string(type));

  if (memcmp(in.data, data, 8) != 0)
    fatal(TAG, "Failed to %s encode/decode corrupted", _codec_to_string(type));

  stream_destroy(&in);
  stream_destroy(&out);
  return 1;
}

int main(int argc, char **argv)
{
  /* test encode and decode base32 */
  if (_test_codec(CODEC_BASE32) == 0)
    fatal(TAG, "%s", "Failed to verify base32 codec.");

  /* test encode and decode base64 */
  if (_test_codec(CODEC_BASE64) == 0)
    fatal(TAG, "%s", "Failed to verify base64 codec.");

  /* test sign and verify */
  if (_test_dsa_sign_and_verify() == 0)
    fatal(TAG, "%s", "Failed to sign and verify a stream.");

  /* test sign and verify using provided key */
  if ( _test_dsa_sign_and_verify_with_provided_key() == 0)
    fatal(TAG, "%s", "Failed to sign and verify a stream using provided public signing key.");

  /* test verify of router info */
  if (_test_dsa_router_info_verify() == 0)
    fatal(TAG, "%s", "Failed to verify a routerinfo stream.");

  /* test verify of broken router info */
  if (_test_dsa_router_info_verify() != 0)
    fatal(TAG, "%s", "Verify of broken hash returned true.");

  return 0;
}
