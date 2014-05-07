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

/*
 * Session callback handlers
 */
void
on_message(struct i2cp_session_t *session,
	   i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
	   stream_t *payload, void *opaque)
{
}

void
on_status(struct i2cp_session_t *session,
	  i2cp_session_status_t status,
	  void *opaque)
{
}

i2cp_session_callbacks_t _session_cb = { NULL, on_message, on_status };

/*
 * Client callback handlers
 */
void on_disconnect(struct i2cp_client_t *client, char *reason, void *opaque)
{
  fprintf(stderr, "Client %p disconnected with reason; %s\n", (void *)client, reason);
}

i2cp_client_callbacks_t _client_cb = { NULL, on_disconnect };

int test_create_session(struct i2cp_client_t *client)
{
  struct i2cp_session_t *session;
  struct i2cp_session_config_t *config;


  /* we are now connected, lets setup and create a session */
  session = i2cp_session_new(client, &_session_cb, NULL);
  config = i2cp_session_get_config(session);

  i2cp_session_config_set_property(config, SESSION_CONFIG_PROP_I2CP_FAST_RECEIVE, "true");
  i2cp_session_config_set_property(config, SESSION_CONFIG_PROP_OUTBOUND_NICKNAME, "test-i2cp");
  i2cp_session_config_set_property(config, SESSION_CONFIG_PROP_OUTBOUND_QUANTITY, "4");

  i2cp_client_create_session(client, session);

  return 1;
}

int main(int argc, char **argv)
{
  struct i2cp_client_t *client;

  i2cp_init();

  /* initialize client and connect */
  client = i2cp_client_new(&_client_cb);
  i2cp_client_connect(client);

#if 1
  test_create_session(client);
#endif

  while(1) {
    i2cp_client_process_io(client);
  };

  /* disconnect */
  i2cp_client_disconnect(client);

  i2cp_client_destroy(client);

  i2cp_deinit();


  return 0;
}
