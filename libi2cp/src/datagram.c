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


#include <i2cp/i2cp.h>


#define TAG DATAGRAM

typedef struct i2cp_datagram_t
{
  int owned;
  struct i2cp_destination_t *destination;
  stream_t payload;
} i2cp_datagram_t;

static void _datagram_clean(i2cp_datagram_t *self)
{
  if (self->owned && self->destination)
    i2cp_destination_destroy(self->destination);
  self->destination = NULL;
  stream_reset(&self->payload);
}

struct i2cp_datagram_t *
i2cp_datagram_new()
{
  i2cp_datagram_t *dg;
  dg = malloc(sizeof(i2cp_datagram_t));
  memset(dg, 0, sizeof(i2cp_datagram_t));
  stream_init(&dg->payload, 0xffff);
  return dg;
}

void
i2cp_datagram_destroy(struct i2cp_datagram_t *self)
{
  stream_destroy(&self->payload);
  free(self);
}

void
i2cp_datagram_for_session(struct i2cp_datagram_t *self,
			  struct i2cp_session_t *session, stream_t *payload)
{
  if (self->owned)
    i2cp_destination_destroy(self->destination);

  self->owned = 0;

  self->destination = i2cp_session_get_destination(session);

  if (payload)
  {
    stream_reset(&self->payload);
    stream_out_stream(&self->payload, payload);
    stream_mark_end(&self->payload);
  }
}

void
i2cp_datagram_from_stream(struct i2cp_datagram_t *self, stream_t *message)
{
  int ret;
  uint8_t digest[40];
  stream_t hash;

  if (self->owned)
    i2cp_destination_destroy(self->destination);

  self->owned = 1;

  /* read destination */
  self->destination = i2cp_destination_new_from_message(message);
  if (self->destination == NULL)
  {
    warning(TAG|PROTOCOL, "failed to read destination from packet.");
    _datagram_clean(self);
    return;
  }
  
  /* read signature */
  ret = stream_tell(message);
  stream_in_uint8p(message, digest, 40);
  if (ret != stream_tell(message) - 40)
  {
    warning(TAG|PROTOCOL, "failed to read digest from packet.");
    _datagram_clean(self);
    return;
  }

  /* read datagram payload */
  stream_reset(&self->payload);
  stream_out_uint8p(&self->payload, message->p, message->end - message->p);
  stream_mark_end(&self->payload);

  /* verify sha256 hash of datagram with signature */
  stream_init(&hash, 4096);

  stream_seek_set(&self->payload, 0);
  i2cp_crypto_hash_stream(i2cp_crypto_instance(), HASH_SHA256, &self->payload, &hash);
  stream_out_uint8p(&hash, digest, 40);
  stream_mark_end(&hash);

  stream_seek_set(&hash, 0);

  ret = i2cp_crypto_verify_stream(i2cp_crypto_instance(),
				  i2cp_destination_signature_keypair(self->destination),
				  &hash);
  if (ret == 0)
  {
    warning(TAG, "Failed to verify signature of datagram.");
    _datagram_clean(self);
    return;
  }
}

void
i2cp_datagram_get_payload(struct i2cp_datagram_t *self, stream_t *payload)
{
  stream_seek_set(&self->payload, 0);
  stream_out_stream(payload, &self->payload);
  stream_mark_end(payload);
}

void
i2cp_datagram_set_payload(struct i2cp_datagram_t *self, stream_t *payload)
{
  stream_reset(&self->payload);
  stream_out_stream(&self->payload, payload);
  stream_mark_end(&self->payload);
}

void
i2cp_datagram_get_message(struct i2cp_datagram_t *self, stream_t *message)
{
  stream_t hash;
  int ret;

  /* calculate sha256 for payload */
  stream_init(&hash, 4096);
  stream_seek_set(&self->payload, 0);
  i2cp_crypto_hash_stream(i2cp_crypto_instance(), HASH_SHA256, &self->payload, &hash);

  /* write destination to message  */
  ret = stream_tell(message);
  i2cp_destination_get_message(self->destination, message);
  
  /* sign hash and write signature to message */
  i2cp_crypto_sign_stream(i2cp_crypto_instance(),
			  i2cp_destination_signature_keypair(self->destination), &hash);
  stream_out_uint8p(message, hash.end - 40, 40);
  stream_mark_end(message);
  
  /* write payload to message */
  stream_out_stream(message, &self->payload);

  stream_mark_end(message);
  stream_destroy(&hash);
}


const struct i2cp_destination_t *
i2cp_datagram_destination(struct i2cp_datagram_t *self)
{
  return self->destination;
}

size_t
i2cp_datagram_payload_size(struct i2cp_datagram_t *self)
{
  return stream_length(&self->payload);
}
