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
#include <inttypes.h>

#include <i2cp/i2cp.h>

#define TAG SESSION

typedef struct i2cp_session_t
{
  uint16_t session_id;
  struct i2cp_client_t *client;
  struct i2cp_session_config_t *config;
  i2cp_session_callbacks_t *callbacks;
} i2cp_session_t;


void
_session_dispatch_message(struct i2cp_session_t *self,
			  i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
			  stream_t *payload)
{
  if (self->callbacks == NULL)
    return;

  if (self->callbacks->on_message == NULL)
    return;
  
  /* dispatch message */
  self->callbacks->on_message(self, protocol, src_port, dest_port, payload,
			      self->callbacks->opaque);
}

void
_session_dispatch_session_status(struct i2cp_session_t *session, 
				 i2cp_session_status_t status)
{
  switch(status)
  {
  case I2CP_SESSION_STATUS_CREATED:
    debug(TAG, "session %p is created.", session);
    break;
  case I2CP_SESSION_STATUS_DESTROYED:
    debug(TAG, "session %p is destroyed.", session);
    break;
  case I2CP_SESSION_STATUS_UPDATED:
    debug(TAG, "session %p is updated.", session);
    break;
  case I2CP_SESSION_STATUS_INVALID:
    debug(TAG, "session %p is invalid.", session);
    break;
  }

  if (session->callbacks == NULL)
    return;

  if (session->callbacks->on_status == NULL)
    return;


  session->callbacks->on_status(session, status, session->callbacks->opaque);
}

void 
_session_dispatch_destination(struct i2cp_session_t *session, uint32_t request_id,
			      char *address, struct i2cp_destination_t *destination)
{
  if (session->callbacks == NULL)
    return;

  if (session->callbacks->on_destination == NULL)
    return;

  session->callbacks->on_destination(session, request_id,  address, destination, session->callbacks->opaque);
}

void
_i2cp_session_set_id(struct i2cp_session_t *self, uint16_t session_id)
{
  self->session_id = session_id;
}
		
struct i2cp_session_t *
i2cp_session_new(struct i2cp_client_t *client,
		 struct i2cp_session_callbacks_t *callbacks,
		 const char *dest_fname)
{
  i2cp_session_t *session;

  session = malloc(sizeof(i2cp_session_t));
  memset(session, 0, sizeof(i2cp_session_t));
  session->client = client;
  session->config = i2cp_session_config_new(dest_fname);
  session->callbacks = callbacks;
  return session;
}

void
i2cp_session_destroy(struct i2cp_session_t *self)
{
  i2cp_session_config_destroy(self->config);
  free(self);
}

struct i2cp_session_config_t *
i2cp_session_get_config(struct i2cp_session_t *self)
{
  return self->config;
}

uint16_t
i2cp_session_get_id(struct i2cp_session_t *self)
{
  return self->session_id;
}

void
i2cp_session_send_message(struct i2cp_session_t *self,
			  const struct i2cp_destination_t *destination,
			  i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
			  stream_t *payload, uint32_t nonce)
{
  _client_msg_send_message(self->client, self, destination,
			   protocol, src_port, dest_port,
			   payload, nonce);
}

const struct i2cp_destination_t *
i2cp_session_get_destination(struct i2cp_session_t *self)
{
  return i2cp_session_config_get_destination(self->config);
}
