/*
  i2cp address lookup tool.
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

#include <unistd.h>
#include <i2cp/i2cp.h>
#include <i2cp/destination.h>

typedef struct lookup_t
{
  struct i2cp_client_t *client;
  struct i2cp_session_t *session;
  struct i2cp_session_config_t *session_cfg;
  uint8_t got_response;
  const char *address;
} lookup_t;

/*
 * Client Callbacks
 */
static void
on_disconnect(struct i2cp_client_t *client, char *reason, void *opaque) {
  exit(0);
}

static void
on_status(struct i2cp_session_t *session, i2cp_session_status_t status, void *opaque)
{
  lookup_t *lookup;
  lookup = (lookup_t *) opaque;

  if (status == I2CP_SESSION_STATUS_CREATED)
  {
    fprintf(stderr, "Session ID %d created.\n", i2cp_session_get_id(session));
    /* perform the lookup */
    i2cp_client_destination_lookup(lookup->client, lookup->session, lookup->address);
  }
}

i2cp_client_callbacks_t _client_cb = { NULL, on_disconnect};

/*
 * Session Callbacks
 */
static void
on_destination(struct i2cp_session_t *session, uint32_t request_id, const char *address,
	       struct i2cp_destination_t *destination, void *opaque)
{
  lookup_t *lookup;
  lookup = (lookup_t *) opaque;

  lookup->got_response = 1;

  if (destination == NULL)
    fprintf(stderr, "Could not resolv '%s' to a destination.\n", address);
  else
    fprintf(stderr, "%s\n", i2cp_destination_b64(destination));
}

i2cp_session_callbacks_t _session_cb = { NULL, NULL, on_status, on_destination };

int main(int argc, char **argv)
{
  int timeout = 60;
  lookup_t lookup;

  /* setup defaults */
  memset(&lookup, 0, sizeof(lookup));

  if (!argv[1])
  {
    fprintf(stderr, "usage: i2cp-lookup <b32 or host address>\n");
    return 1;
  }

  lookup.address = argv[1];
  
  /* initialize and setup */
  i2cp_init();
  lookup.client = i2cp_client_new(&_client_cb);

  i2cp_logger_set_filter(i2cp_logger_instance(), ALL);

  i2cp_client_connect(lookup.client);

  /* create the session */
  _session_cb.opaque = &lookup;
  lookup.session = i2cp_session_new(lookup.client, &_session_cb, NULL);
  lookup.session_cfg = i2cp_session_get_config(lookup.session);

  /* set client options */
  i2cp_session_config_set_property(lookup.session_cfg, 
				   SESSION_CONFIG_PROP_OUTBOUND_NICKNAME,
				   "i2cp-lookup");
  i2cp_session_config_set_property(lookup.session_cfg, 
				   SESSION_CONFIG_PROP_I2CP_DONT_PUBLISH_LEASE_SET,
				   "true");

  /* create session */
  i2cp_client_create_session(lookup.client, lookup.session);

  while (lookup.got_response == 0)
  {
    i2cp_client_process_io(lookup.client);
    usleep(100);
  }

  i2cp_session_destroy(lookup.session);
  i2cp_client_destroy(lookup.client);
  return 0;
}
