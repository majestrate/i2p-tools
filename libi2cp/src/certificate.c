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

#include <inttypes.h>
#include <stdlib.h>

#include <i2cp/i2cp.h>

#define TAG_CERT "CERTIFICATE"

typedef struct i2cp_certificate_t
{
  i2cp_certificate_type_t type;
  uint8_t *data;
  uint32_t length;
} i2cp_certificate_t;


struct i2cp_certificate_t *
i2cp_certificate_copy(const i2cp_certificate_t *src)
{
  i2cp_certificate_t *cert;

  /* allocate certificate */
  cert = malloc(sizeof(i2cp_certificate_t));
  memset(cert, 0, sizeof(i2cp_certificate_t));

  /* make a copy */
  cert->type = src->type;
  cert->length = src->length;
  cert->data = malloc(src->length);
  memcpy(cert->data, src->data, src->length);
  return cert;
}

struct i2cp_certificate_t *
i2cp_certificate_new(i2cp_certificate_type_t type)
{
  i2cp_certificate_t *cert;
  cert = malloc(sizeof(i2cp_certificate_t));
  memset(cert, 0, sizeof(i2cp_certificate_t));
  cert->type = type;

  return cert;
}


struct i2cp_certificate_t *
i2cp_certificate_new_from_message(stream_t *stream)
{
  i2cp_certificate_t *cert;
  cert = malloc(sizeof(i2cp_certificate_t));
  memset(cert, 0, sizeof(i2cp_certificate_t));
  
  /* type */
  stream_in_uint8(stream, cert->type);

  /* cert length */
  stream_in_uint16(stream, cert->length);
  if (cert->type != CERTIFICATE_NULL && cert->length == 0)
  {
    fatal(CERTIFICATE|PROTOCOL, "%s", "only null certificates are allowed to have zero length.");
    free(cert);
    return NULL;
  }

  /* if null cert */
  if (cert->type == CERTIFICATE_NULL)
    return cert;

  /* cert data */
  cert->data = malloc(cert->length);
  stream_in_uint8p(stream, cert->data, cert->length); 

  return cert;
}

struct i2cp_certificate_t *
i2cp_certificate_new_from_stream(stream_t *stream)
{
  i2cp_certificate_t *cert;
  cert = malloc(sizeof(i2cp_certificate_t));
  memset(cert, 0, sizeof(i2cp_certificate_t));
  
  /* type */
  stream_in_uint8(stream, cert->type);

  /* cert length */
  stream_in_uint16(stream, cert->length);
  if (cert->type != CERTIFICATE_NULL && cert->length == 0)
  {
    fatal(CERTIFICATE|PROTOCOL, "%s", "only null certificates are allowed to have zero length.");
    free(cert);
    return NULL;
  }

  /* if null cert */
  if (cert->type == CERTIFICATE_NULL)
    return cert;

  /* cert data */
  cert->data = malloc(cert->length);
  stream_in_uint8p(stream, cert->data, cert->length); 

  return cert;
}


void
i2cp_certificate_to_stream(struct i2cp_certificate_t *self, stream_t *stream)
{
  stream_out_uint8(stream, self->type);
  stream_out_uint16(stream, self->length);
  if (self->length > 0)
    stream_out_uint8p(stream, self->data, self->length);

  stream_mark_end(stream);
}

void
i2cp_certificate_destroy(struct i2cp_certificate_t *self)
{
  free(self);
}

void
i2cp_certificate_get_message(struct i2cp_certificate_t *self, stream_t *stream)
{
  stream_out_uint8(stream, self->type);
  stream_out_uint16(stream, self->length);
  if (self->length > 0)
    stream_out_uint8p(stream, self->data, self->length);

  stream_mark_end(stream);
}
