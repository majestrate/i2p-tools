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

#ifndef _destination_h
#define _destination_h

struct stream_t;

/** \brief An i2p destination */
struct i2cp_destination_t;

/** \brief Construct a destination */
struct i2cp_destination_t *i2cp_destination_new();

/** \brief Construct a destination from a stream and verifies it aginst digest. */
struct i2cp_destination_t *i2cp_destination_new_from_message(struct stream_t *stream);

/** \brief Construct a destination from a base64 string. */
struct i2cp_destination_t *i2cp_destination_new_from_base64(const char *base64);

/** \brief Construct a destination from a stream. */
struct i2cp_destination_t *i2cp_destination_new_from_stream(struct stream_t *stream);

/** \brief Construct a destination from a file. */
struct i2cp_destination_t *i2cp_destination_new_from_file(const char *filename);

/** \brief Creates a copy of src destination. */
struct i2cp_destination_t *i2cp_destination_copy(const struct i2cp_destination_t *src);

/** \brief Destroys a destination instance. */
void i2cp_destination_destroy(struct i2cp_destination_t *self);

/** \brief Saves a destination to file.
    \see i2cp_destination_new_from_file()
*/
void i2cp_destination_save(struct i2cp_destination_t *self, const char *filename);

/** \brief Serialize a destination into a steam using internal format.
    \warning This also stores the private keys in the stream.
 */
void i2cp_destination_to_stream(struct i2cp_destination_t *self, struct stream_t *stream);

/** \brief Get a destination message. */
void i2cp_destination_get_message(struct i2cp_destination_t *self, struct stream_t *stream);

/** \brief Get the signature keypair for the destination.
    If the destination is not ours only the public key is available in the pair.
    \param[in] self The destination to get keypairs from.
    \see i2cp_signature_keypair_t i2cp_destination_t
 */
const struct i2cp_signature_keypair_t *i2cp_destination_signature_keypair(struct i2cp_destination_t *self);

/** \brief Retreive the b32 address of the destination.
    A b32 address is the base32 encoded sha256 hash of a desination
    suffixed with the string ".b32.i2p".
    \return A string with a b32 address.
 */
const char *i2cp_destination_b32(const struct i2cp_destination_t *self);

/** \brief Reretive the b64 address of the destination.
    A b64 destination is the base64 encoded of the raw destination.
    \return A string with b64 address.
 */
const char *i2cp_destination_b64(const struct i2cp_destination_t *self);

#endif
