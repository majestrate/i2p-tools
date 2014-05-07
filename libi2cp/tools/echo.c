/*
  i2cp echo service/client tool
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

#define MESSAGE "Hello"

typedef struct echo_t
{
  struct i2cp_client_t *client;
  struct i2cp_session_t *session;
  struct i2cp_session_config_t *session_cfg;
  struct i2cp_destination_t *destination;

  struct i2cp_datagram_t *incoming;
  struct i2cp_datagram_t *outgoing;

  int server;
  const char *address;
  int got_response;

} echo_t;

/*
 * Client Callbacks
 */
void
on_disconnect(struct i2cp_client_t *client, char *reason, void *opaque) {
  fprintf(stderr, "Disconnected with reason; %s\n", reason);
  exit(0);
}

i2cp_client_callbacks_t _client_cb = { NULL, on_disconnect, NULL};

/*
 * Session Callbacks
 */
void on_message(struct i2cp_session_t *session,
		i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
		stream_t *payload, void *opaque)
{
  echo_t *echo;
  stream_t p;
  const struct i2cp_destination_t *from;

  if (protocol != PROTOCOL_DATAGRAM)
    return;

  echo = (echo_t *) opaque;
  stream_init(&p, 0xffff);

  /* parse datagram message from payload */
  i2cp_datagram_from_stream(echo->incoming, payload);

  if (echo->server)
  {
    /* send payload back to destination */
    from = i2cp_datagram_destination(echo->incoming);
    fprintf(stderr, "Got message from %s\n", i2cp_destination_b32(from));

    /* bounce the data back to client */
    i2cp_datagram_get_payload(echo->incoming, &p);
    i2cp_datagram_set_payload(echo->outgoing, &p);

    stream_reset(&p);
    i2cp_datagram_get_message(echo->outgoing, &p);
    i2cp_session_send_message(echo->session, from,
			      PROTOCOL_DATAGRAM, 0, 0,
			      &p, 0);
  }
  else
  {
    /* Got our response as a client, lets echo to stdout and exit */
    echo->got_response = 1;
    i2cp_datagram_get_payload(echo->incoming, &p);
    fwrite(p.data, 1, stream_length(&p), stdout);
    exit(0);
  }

  stream_destroy(&p);
}

void on_status(struct i2cp_session_t *session, i2cp_session_status_t status, void *opaque)
{
  echo_t *echo;
  echo = (echo_t *) opaque;

  if (status == I2CP_SESSION_STATUS_CREATED)
  {

    /* setup outgoing datagram for session */
    i2cp_datagram_for_session(echo->outgoing, echo->session, NULL);

    if (echo->server == 0)
    {
      fprintf(stderr,"Echo client connected.\n");
      i2cp_client_destination_lookup(echo->client, echo->session, echo->address);
    }
    else
    {
      fprintf(stderr,"Echo server connected and running, listening on address:\n%s\n",
	      i2cp_destination_b32(i2cp_session_get_destination(session)));
    }
  }
}

void
on_destination(struct i2cp_session_t *session, uint32_t request_id, const char *address,
	       struct i2cp_destination_t *destination, void *opaque)
{
  echo_t *echo;
  stream_t p;

  echo = (echo_t *) opaque;

  if (echo->server)
    return;

  if (destination == NULL)
  {
    fprintf(stderr, "Failed to resolv a destination for '%s'\n", address);
    exit(1);
  }

  /* store destination of echo server */
  echo->destination = destination;

  /* create datagram to send to echo server */
  stream_init(&p, 0xffff);
  stream_out_uint8p(&p, MESSAGE, strlen(MESSAGE) + 1);
  stream_mark_end(&p);

  i2cp_datagram_set_payload(echo->outgoing, &p);

  /* get datagram message */
  stream_reset(&p);
  i2cp_datagram_get_message(echo->outgoing, &p);

  /* send the echo message to destination */
  i2cp_session_send_message(echo->session, echo->destination,
			    PROTOCOL_DATAGRAM, 0, 0,
			    &p, 0);

  stream_destroy(&p);
}


i2cp_session_callbacks_t _session_cb = { NULL, on_message, on_status, on_destination };

int main(int argc, char **argv)
{
  echo_t echo;
  stream_t stream;
  struct i2cp_destination_t *destination;

  /* setup defaults */
  memset(&echo, 0, sizeof(echo));
  echo.server = 1;
  echo.incoming = i2cp_datagram_new();
  echo.outgoing = i2cp_datagram_new();

  /* initialize and setup */
  i2cp_init();
  echo.client = i2cp_client_new(&_client_cb);
  i2cp_logger_set_filter(i2cp_logger_instance(), ALL);
  i2cp_client_connect(echo.client);

  if (i2cp_client_is_connected(echo.client) == 0)
    return;

  /* if argument, assume b32 address and become a client */
  if (argv[1])
  {
    echo.server = 0;
    echo.address = argv[1];
  }

  /* create the session */
  _session_cb.opaque = &echo;
  echo.session = i2cp_session_new(echo.client, &_session_cb,
				  echo.server ? "echo.destination" : NULL);
  echo.session_cfg = i2cp_session_get_config(echo.session);

#if 0
  /* authenticate */
  i2cp_session_config_set_property(echo.session_cfg, SESSION_CONFIG_PROP_I2CP_USERNAME, "i2cp");
  i2cp_session_config_set_property(echo.session_cfg, SESSION_CONFIG_PROP_I2CP_PASSWORD, "password");
#endif

  if (echo.server)
  {
    /* server options */
    i2cp_session_config_set_property(echo.session_cfg, 
				     SESSION_CONFIG_PROP_INBOUND_NICKNAME,
				     "echo-server");
  }
  else 
  {
    /* client options */
    i2cp_session_config_set_property(echo.session_cfg, 
				     SESSION_CONFIG_PROP_OUTBOUND_NICKNAME,
				     "echo-client");

    i2cp_session_config_set_property(echo.session_cfg, 
				     SESSION_CONFIG_PROP_I2CP_DONT_PUBLISH_LEASE_SET,
				     "true");
  }

  i2cp_client_create_session(echo.client, echo.session);

  /* main loop */
  while (echo.got_response == 0)
  {
    i2cp_client_process_io(echo.client);
    usleep(100);
  }

  /* close and exit */
  i2cp_client_destroy(echo.client);
  return 0;
}
