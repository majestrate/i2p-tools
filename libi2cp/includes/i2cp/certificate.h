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
#ifndef _certificate_h
#define _certificate_h

struct stream_t;
struct i2cp_certificate_t;

/** \brief Available certificate types.
    \todo Implement other certificate types then just a NULL certificate.
*/
typedef enum i2cp_certificate_type_t
{
  CERTIFICATE_NULL = 0,
  CERTIFICATE_HASHCASH,
  CERTIFICATE_SIGNED,
  CERTIFICATE_MULTIPLE
} i2cp_certificate_type_t;

/** \brief Creates a new certificate of specified type.
    \remark For now we can only generate a NULL certificate.
*/
struct i2cp_certificate_t *i2cp_certificate_new(i2cp_certificate_type_t type);

/** \brief reads and construct a certificate out of i2cp certificate
    message stored in stream.
    \see i2cp_certificate_get_message
*/
struct i2cp_certificate_t *i2cp_certificate_new_from_message(struct stream_t *stream);

/** \brief reads and construct a certificate from serialized certificate
    from stream.
    \remark
    This will only work with streams containing internal representation
    of a certificate stream.
    \see i2cp_certificate_to_stream
*/
struct i2cp_certificate_t *i2cp_certificate_new_from_stream(struct stream_t *stream);

/** \brief Creates a copy of src certificate. */
struct i2cp_certificate_t *i2cp_certificate_copy(const struct i2cp_certificate_t *src);

/** \brief writes a certificate to a stream.
    Serializes a certificate to an internal representation and output the data
    into the stream.
 */
void i2cp_certificate_to_stream(struct i2cp_certificate_t *self, struct stream_t *stream);

/** \brief destroys a certificate instance. */
void i2cp_certificate_destroy(struct i2cp_certificate_t *self);

/** \brief write a i2cp certificate struct into stream.
    This is not necessary the same as serialize which is the internal
    serialization format of the object.
*/
void i2cp_certificate_get_message(struct i2cp_certificate_t *self, struct stream_t *stream);

#endif
