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

#ifndef _crypto_h
#define _crypto_h

#include <gmp.h>

#include <i2cp/stream.h>

/** \brief Supported hash algorithms */
typedef enum i2cp_hash_algorithm_t {
  HASH_SHA1,
  HASH_SHA256
} i2cp_hash_algorithm_t;

/** \brief Supported signature algorithms */
typedef enum i2cp_signature_algorithm_t {
  DSA_SHA1,
  DSA_SHA256
} i2cp_signature_algorithm_t;

/** \brief Supported codec algorithms */
typedef enum i2cp_codec_algorithm_t {
  CODEC_BASE32,
  CODEC_BASE64
} i2cp_codec_algorithm_t;

typedef struct i2cp_signature_keypair_t
{
  i2cp_signature_algorithm_t type;

  mpz_t dsa_public;
  mpz_t dsa_private;

} i2cp_signature_keypair_t;

/** \brief A crypto context.
 *  The crypto context contains all cryptographic functionality and
 *  the crypto context is a singelton in i2cp library.
 *  \see i2cp_client_t
 */
struct i2cp_crypto_t;

/** \brief Returns crypto singelton instance. */
struct i2cp_crypto_t *i2cp_crypto_instance();

/** \brief Sign a stream using specified algorithm.
 *  The digest is appended to the stream.
 *  \param[in] keypair Keys to use for signing
 *  \param[in/out] stream Output stream
 *  \see i2cp_signature_algorithm_t
 */
void i2cp_crypto_sign_stream(struct i2cp_crypto_t *self, 
			     const i2cp_signature_keypair_t *keypair, 
			     stream_t *stream);

/** \brief Verify a stream using specified algorithm.
    The digest is read from the end of stream.
    \see i2cp_signature_algorithm_t
*/
int i2cp_crypto_verify_stream(struct i2cp_crypto_t *self,
			      const i2cp_signature_keypair_t *keypair,
			      stream_t *stream);

/** \brief Write public signature key into stream.
 */
void i2cp_crypto_signature_publickey_stream(struct i2cp_crypto_t *self,
					    const i2cp_signature_keypair_t *keypair,
					    stream_t *stream);

/** \brief Write a signature keypair to stream.
*/
void i2cp_crypto_signature_keypair_to_stream(struct i2cp_crypto_t *self,
					     const i2cp_signature_keypair_t *keypair,
					     stream_t *stream);
/** \brief Reads and initialize signature keypair from stream.
    \see i2cp_crypto_signature_keypair_to_stream()
*/
void i2cp_crypto_signature_keypair_from_stream(struct i2cp_crypto_t *self,
					       i2cp_signature_keypair_t *keypair,
					       stream_t *stream);

/** \brief Generates a signature keypair 
    \param[in] type Specify which algorithm to use
    \param[out] keypair Destination of generated keys
 */
void i2cp_crypto_signature_keygen(struct i2cp_crypto_t *self,
				  i2cp_signature_algorithm_t type,
				  i2cp_signature_keypair_t *keypair);


void i2cp_crypto_hash_stream(struct i2cp_crypto_t *self, i2cp_hash_algorithm_t type,
			     stream_t *src, stream_t *dest);

/** \brief Encodes a stream into output stream. */
void i2cp_crypto_encode_stream(struct i2cp_crypto_t *self, i2cp_codec_algorithm_t type,
			       stream_t *src, stream_t *dest);
/** \brief Decodes a stream inti output stream. */
void i2cp_crypto_decode_stream(struct i2cp_crypto_t *self, i2cp_codec_algorithm_t type,
			       stream_t *src, stream_t *dest);
#endif
