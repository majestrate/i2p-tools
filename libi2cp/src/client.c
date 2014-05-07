/*
  Part of i2cp C library
  Copyright (C) 2013-2014 Oliver Queen <oliver@mail.i2p>

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

#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <zlib.h>

#include <i2cp/client.h>
#include <i2cp/destination.h>
#include <i2cp/lease.h>
#include <i2cp/session.h>
#include <i2cp/session_config.h>
#include <i2cp/crypto.h>
#include <i2cp/tcp.h>
#include <i2cp/queue.h>
#include <i2cp/stringmap.h>
#include <i2cp/config_file.h>
#include <i2cp/version.h>

#define I2CP_CLIENT_VERSION "0.9.11"

#define TAG CLIENT

#define I2CP_PROTOCOL_INIT 0x2a
#define I2CP_MESSAGE_SIZE 0xffff
#define I2CP_MAX_SESSIONS 0xffff
#define I2CP_MAX_SESSIONS_PER_CLIENT 32

#define I2CP_MSG_ANY                        0
#define I2CP_MSG_BANDWIDTH_LIMITS          23
#define I2CP_MSG_CREATE_LEASE_SET           4
#define I2CP_MSG_CREATE_SESSION             1
#define I2CP_MSG_DEST_LOOKUP               34
#define I2CP_MSG_DEST_REPLY                35
#define I2CP_MSG_DESTROY_SESSION            3
#define I2CP_MSG_DISCONNECT                30
#define I2CP_MSG_GET_BANDWIDTH_LIMITS       8
#define I2CP_MSG_GET_DATE                  32
#define I2CP_MSG_HOST_LOOKUP               38
#define I2CP_MSG_HOST_REPLY                39
#define I2CP_MSG_MESSAGE_STATUS            22
#define I2CP_MSG_PAYLOAD_MESSAGE           31
#define I2CP_MSG_REQUEST_LEASESET          21
#define I2CP_MSG_REQUEST_VARIABLE_LEASESET 37
#define I2CP_MSG_SEND_MESSAGE               5
#define I2CP_MSG_SESSION_STATUS            20
#define I2CP_MSG_SET_DATE                  33

/* Router capabilities */
#define ROUTER_CAN_HOST_LOOKUP              1

/* I2CP_MSG_HOST_LOOKUP types */
enum {
  HOST_LOOKUP_TYPE_HASH,
  HOST_LOOKUP_TYPE_HOST
};

typedef struct i2cp_client_t
{
  i2cp_logger_callbacks_t logger;
  i2cp_client_callbacks_t *callbacks;

  const char * properties[NR_OF_I2CP_CLIENT_PROPERTIES];
  struct tcp_t * tcp;

  stream_t output_stream;
  stream_t message_stream;

  struct {
    uint64_t date;
    struct i2cp_version_t *version;
    uint32_t capabilities;
  } router;

  /* TODO: We should probably replace the output queue to a ringbuffer based on stream_t
           to optimize away all the allocs() done when putting on outqueue.
   */
  struct queue_t *output_queue;

  struct i2cp_session_t *sessions[I2CP_MAX_SESSIONS];
  int session_count;

  /* mapping table for address lookups */
  struct stringmap_t *lookups;
  struct intmap_t *lookup_requests;
  uint32_t lookup_request_id;

} i2cp_client_t;

/* This is used to map a dest lookup request id with a session */
typedef struct _client_host_lookup_item_t
{
  const char *address;
  struct i2cp_session_t *session;
} _client_host_lookup_item_t;

/*
 * External decls for private session dispatch functions.
 */
extern void _session_dispatch_message(struct i2cp_session_t *session,
				      uint8_t protocol, uint16_t src_port, uint16_t dest_port,
				      stream_t *payload);

extern void _session_dispatch_session_status(struct i2cp_session_t *session, i2cp_session_status_t status);

extern void _session_dispatch_destination(struct i2cp_session_t *session, uint32_t request_id,
					  char *address, struct i2cp_destination_t *destination);

static void _client_msg_create_lease_set(i2cp_client_t *self, struct i2cp_session_t *session,
					 uint8_t tunnels, struct i2cp_lease_t **leases, int queue);

static _client_host_lookup_item_t *
_client_host_lookup_item_ctor(char *address, struct i2cp_session_t *session)
{
  _client_host_lookup_item_t *lup;
  lup = (_client_host_lookup_item_t *)malloc(sizeof(_client_host_lookup_item_t));
  memset(lup, 0, sizeof(_client_host_lookup_item_t));
  lup->address = strdup(address);
  lup->session = session;
  return lup;
}

void
_client_host_lookup_item_dtor(_client_host_lookup_item_t * item)
{
  free(item->address);
  free(item);
}

static void
_client_config_file_parse_callback(const char *name, const char *value, void *opaque)
{
  i2cp_client_t *client;
  client = (i2cp_client_t *)opaque;

  if (strcmp(name, "i2cp.tcp.host") == 0)
    client->properties[CLIENT_PROP_ROUTER_ADDRESS] = strdup(value);
  else if (strcmp(name, "i2cp.tcp.port") == 0)
    client->properties[CLIENT_PROP_ROUTER_PORT] = strdup(value);
#ifdef WITH_GNUTLS
  else if (strcmp(name, "i2cp.tcp.SSL") == 0)
    client->properties[CLIENT_PROP_ROUTER_USE_TLS] = strdup(value);
#endif
  else if (strcmp(name, "i2cp.username") == 0)
    client->properties[CLIENT_PROP_USERNAME] = strdup(value);
  else if (strcmp(name, "i2cp.password") == 0)
    client->properties[CLIENT_PROP_PASSWORD] = strdup(value);
}

static void
_client_default_properties(i2cp_client_t *self)
{
  char *home;
  char cfgfile[4096];
  struct config_file_t *cfg;

  /* hardcoded defaults */
  self->properties[CLIENT_PROP_ROUTER_ADDRESS] = "127.0.0.1";
  self->properties[CLIENT_PROP_ROUTER_PORT]    =      "7654";
#ifdef WITH_GNUTLS
  self->properties[CLIENT_PROP_ROUTER_USE_TLS] =         "0";
#endif

  /* load user config file */
  home = getenv("HOME");
   if (home == NULL)
    return;

  sprintf(cfgfile,"%s/.i2cp.conf", home);
  debug(TAG, "Loading config file '%s'", cfgfile);
  cfg = config_file_new(cfgfile);
  if (cfg == NULL)
    return;

  /* parse config file*/
  config_file_parse(cfg, _client_config_file_parse_callback, self);

  config_file_destroy(cfg);
}

static void
_client_on_log_callback(i2cp_logger_t *logger, i2cp_logger_tags_t tags,
			const char *message, void *opaque)
{
  char tagline[512]={0};
  i2cp_client_t *client;
  client = (i2cp_client_t *)opaque;

  if (client->callbacks == NULL || client->callbacks->on_log == NULL)
  {
    /* level */
    if (tags & DEBUG) strcat(tagline, "\e[32m");
    if (tags & INFO) strcat(tagline, "\e[37m");
    if (tags & WARNING) strcat(tagline, "\e[33m");
    if (tags & ERROR) strcat(tagline, "\e[31m");
    if (tags & FATAL) strcat(tagline, "\e[31m");

    /* component */
    if (tags & STRINGMAP) strcat(tagline, "i2cp.StringMap");
    if (tags & INTMAP) strcat(tagline, "i2cp.IntMap");
    if (tags & STREAM) strcat(tagline, "i2cp.Stream");
    if (tags & CRYPTO) strcat(tagline, "i2cp.Crypto");
    if (tags & TCP) strcat(tagline, "i2cp.TCP");
    if (tags & CLIENT) strcat(tagline, "i2cp.Client");
    if (tags & CERTIFICATE) strcat(tagline, "i2cp.Certificate");
    if (tags & LEASE) strcat(tagline, "i2cp.Lease");
    if (tags & DESTINATION) strcat(tagline, "i2cp.Destination");
    if (tags & SESSION) strcat(tagline, "i2cp.Session");
    if (tags & SESSION_CONFIG) strcat(tagline, "i2cp.SessionConfig");
    if (tags & TEST) strcat(tagline, "i2cp.Test");
    if (tags & CONFIG_FILE) strcat(tagline, "i2cp.ConfigFile");
    if (tags & DATAGRAM) strcat(tagline, "i2p.Datagram");

    /* tag */
    if (tags & PROTOCOL) strcat(tagline, ", Protocol");
    if (tags & LOGIC) strcat(tagline, ", Logic");

    fprintf(stderr, "%s: ", tagline);
    fprintf(stderr, "%s\n\e[0m", message);
  }
  else
    client->callbacks->on_log(client, tags, message, client->callbacks->opaque);
}

static void
_client_dispatch_on_disconnect(i2cp_client_t *self, const char *message)
{
  if (self->callbacks == NULL)
    return;

  if (self->callbacks->on_disconnect == NULL)
    return;

  /* dispatch */
  self->callbacks->on_disconnect(self, message, self->callbacks->opaque);  
}

static void
_client_on_msg_set_date(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  debug(TAG | PROTOCOL, "%s", "Received SetDate message.");
  stream_in_uint64(stream, self->router.date);

  self->router.version = i2cp_version_new_from_stream(stream);
  debug(TAG, "Router version %s, date %ld", i2cp_version_to_string(self->router.version), self->router.date);

  /* Now when we have the router version lets setup protocol capabilities */
  if (i2cp_version_cmp(self->router.version, 0, 9, 10, 0) >= 0)
    self->router.capabilities |= ROUTER_CAN_HOST_LOOKUP;

}

static void
_client_on_msg_disconnect(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  uint8_t size;
  char *string;

  debug(TAG|PROTOCOL, "%s", "Received Disconnect message.");

  stream_in_uint8(stream, size);
  string = malloc(size+1);
  stream_in_uint8p(stream, string, size);
  debug(TAG, "Received disconnect with reason; %s", string);

  _client_dispatch_on_disconnect(self, string);

  free(string);
}

static void
_client_stream_deflate(stream_t *in, stream_t *out)
{
  int ret;
  z_stream zs;

  memset(&zs, 0, sizeof(zs));
  zs.next_in = in->p;
  zs.avail_in = stream_length(in) - stream_tell(in);
  zs.next_out = out->data;
  zs.avail_out = stream_size(out);
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;

  ret = deflateInit2(&zs, 9, Z_DEFLATED, MAX_WBITS+16, 9, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK)
  {
    warning(TAG, "Failed to initialize deflate of stream with reason: %s", zs.msg);
    return;
  }  

  /* compress of stream */
  ret = deflate(&zs, Z_FINISH);
  if (ret != Z_STREAM_END)
  {
    warning(TAG, "Failed to deflate of stream with reason %d: %s", ret, zs.msg);
    deflateEnd(&zs);
    return;
  }

  out->p += zs.total_out;
  stream_mark_end(out);

  deflateEnd(&zs);
  return;
}

static void
_client_stream_inflate(stream_t *in, stream_t *out)
{
  int ret;
  z_stream zs;

  /* initialize inflate of raw gzip stream */
  memset(&zs, 0, sizeof(zs));
  zs.next_in = in->p;
  zs.avail_in = stream_length(in) - stream_tell(in);
  zs.next_out = out->data;
  zs.avail_out = stream_size(out);
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;

  ret = inflateInit2(&zs, MAX_WBITS+16);
  if (ret != Z_OK)
  {
    warning(TAG, "Failed to initialize inflate of stream with reason: %s", zs.msg);
    return;
  }
  
  /* decompress stream */
  ret = inflate(&zs, Z_FINISH);
  if (ret != Z_STREAM_END)
  {
    warning(TAG, "Failed to inflate of stream with reason %d: %s", ret, zs.msg);
    inflateEnd(&zs);
    return;
  }

  /* finalize the stream payload */
  out->p += zs.total_out;
  stream_mark_end(out);

  inflateEnd(&zs);
}

static void
_client_on_msg_payload_message(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  uint16_t session_id;
  uint32_t message_id;
  uint8_t gzip_header[3] = {0x1f, 0x8b, 0x08};
  stream_t out;
  uint16_t src_port;
  uint16_t dest_port;
  uint8_t protocol;
  struct i2cp_session_t *session;

  debug(TAG|PROTOCOL, "%s", "Received PayloadMessage message.");

  session_id = message_id = 0;
  stream_in_uint16(stream, session_id);
  stream_in_uint32(stream, message_id);

  session = self->sessions[session_id];
  if (session == NULL)
    fatal(TAG|FATAL, "Session id %d does not match session client %p initiated.",
	  session_id, (void *)self);

  /* skip payload_size */
  stream_skip(stream, 4);

  /* validate payload header */
  if (memcmp(stream->p, gzip_header, 3) != 0)
  {
    warning(TAG, "%s", "Payload header validation failed, skipping payload.");
    return;
  }

  /* setup zlib stream and decompress payload */
  stream_init(&out, 0xffff);
  _client_stream_inflate(stream, &out);

  if (stream_length(&out) > 0)
  {
    /* read streaming info out of gzip header */
    stream_seek_set(stream, 0);

    /* skip session_id, message_id, payload size */
    stream_skip(stream, 2);
    stream_skip(stream, 4);
    stream_skip(stream, 4);

    /* skip gzip header and gzip flags */
    stream_skip(stream, 3);
    stream_skip(stream, 1);

    /* read stream src and dest port */
    stream_in_uint16(stream, src_port);
    stream_in_uint16(stream, dest_port);

    /* skip gzip xflags */
    stream_skip(stream, 1);

    /* read stream protocol */
    stream_in_uint8(stream, protocol);

    /* dispatch uncompressed payload stream to session */
    stream_seek_set(&out, 0);
    _session_dispatch_message(session, protocol, src_port, dest_port, &out);

    /* cleanup */
    stream_destroy(&out);
  }
}

static void
_client_on_msg_message_status(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  uint16_t session_id;
  uint32_t message_id;
  uint8_t status;
  uint32_t size;
  uint32_t nonce;

  debug(TAG|PROTOCOL, "%s", "Received MessageStatus message.")

  stream_in_uint16(stream, session_id);
  stream_in_uint32(stream, message_id);
  stream_in_uint8(stream, status);
  stream_in_uint32(stream, size);
  stream_in_uint32(stream, nonce);

  debug(TAG|PROTOCOL, "Message status; session id %d, message id %d, status %d, size %d, nonce %d",
	session_id, message_id, status, size, nonce);

  /* TODO: we do not support this due to fastReceive use */
}

static void
_client_on_msg_dest_reply(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  uint32_t request_id;
  const char *p;
  stream_t b32;
   struct i2cp_destination_t * destination;
  _client_host_lookup_item_t *lup;

  debug(TAG|PROTOCOL, "%s", "Received DestReply message.");
  destination = NULL;

  stream_init(&b32, 512);

  /* if result not is length of sha256, a destination was found */
  if (stream_length(stream) != 32) 
  {
    destination = i2cp_destination_new_from_message(stream);
    if (destination == NULL)
      fatal(TAG|FATAL, "%s", "Failed to construct destination from stream.");
    
    p = i2cp_destination_b32(destination);
    stream_out_uint8p(&b32, p, strlen(p) + 1);
    stream_mark_end(&b32);
  }
  else
  {
    i2cp_crypto_encode_stream(i2cp_crypto_instance(), CODEC_BASE32, stream, &b32);
    stream_out_uint8p(&b32, ".b32.i2p\0", 9);
    stream_mark_end(&b32);

    debug(TAG, "%s", "Could not resolve destination.");
  }

  /* lookup destination lookup request for dispatch of the results */
  request_id = (uint32_t)stringmap_get(self->lookups, (const char *)b32.data);
  stringmap_remove(self->lookups, (const char *)b32.data);

  lup = (_client_host_lookup_item_t *)intmap_get(self->lookup_requests, request_id);
  intmap_remove(self->lookup_requests, request_id);

  if (lup == NULL)
  {
    warning(TAG, "No session for destination lookup of address '%s'.", b32.data);
    if (destination)
      i2cp_destination_destroy(destination);
    return;
  }
  
  /* dispatch result to session */
  _session_dispatch_destination(lup->session, request_id, (char *)b32.data, destination);

  _client_host_lookup_item_dtor(lup);

  stream_destroy(&b32);

}

static void
_client_on_msg_host_reply(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  uint8_t result;
  uint16_t session_id;
  uint32_t request_id;
  struct i2cp_sessiont_t *session;
  struct i2cp_destination_t *destination;
  _client_host_lookup_item_t *lup;

  debug(TAG|PROTOCOL, "%s", "Received HostReply message.");
  session = destination = NULL;

  stream_in_uint16(stream, session_id);
  stream_in_uint32(stream, request_id);
  stream_in_uint8(stream, result);

  if (result == 0)
    destination = i2cp_destination_new_from_message(stream);

  if (result == 0 && destination == NULL)
    fatal(TAG|FATAL, "%s", "Failed to construct destination from stream.");

  session = self->sessions[session_id];
  if (session == NULL)
    fatal(TAG|FATAL, "Session with id %d doesn't exists in client instance %p.",
		session_id, (void *)self);

  lup = (_client_host_lookup_item_t *)intmap_get(self->lookup_requests, request_id);
  intmap_remove(self->lookup_requests, lup);

  _session_dispatch_destination(session, request_id, lup->address, destination);

  _client_host_lookup_item_dtor(lup);
}


static void
_client_on_msg_bandwidth_limits(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  debug(TAG|PROTOCOL, "%s", "Received BandwidthLimits message.");
}

static void
_client_on_msg_session_status(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  struct i2cp_session_t *session;
  uint16_t session_id;
  uint8_t session_status;

  debug(TAG|PROTOCOL, "%s", "Received SessionStatus message.");

  /* read message */
  stream_in_uint16(stream, session_id);
  stream_in_uint8(stream, session_status);

  /* if session is created lets assign session instance to session_id */
  if (session_status == I2CP_SESSION_STATUS_CREATED)
  {
    _i2cp_session_set_id((struct i2cp_sessiont_t *)opaque, session_id);
    self->sessions[session_id] = (struct i2cp_session_t *)opaque;
  }

  session = self->sessions[session_id];

  if (session == NULL)
    fatal(TAG|FATAL, "Session with id %d doesn't exists in client instance %p.",
		session_id, (void *)self);

  _session_dispatch_session_status(session, session_status);
}

static void
_client_on_msg_request_variable_leaseset(i2cp_client_t *self, stream_t *stream, void *opaque)
{
  int t;
  uint16_t session_id;
  uint8_t tunnels;
  struct i2cp_session_t *session;
  struct i2cp_lease_t **leases;

  debug(TAG|PROTOCOL, "%s", "Received RequesetVariableLeaseSet message.");

  /* read message */
  stream_in_uint16(stream, session_id);
  stream_in_uint8(stream, tunnels);

  /* get session for the request */
  session = self->sessions[session_id];
  if (session == NULL)
    fatal(TAG|FATAL, "Session with id %d doesn't exists in client instance %p.",
		session_id, (void *)self);

  /* create leases */
  leases = (struct i2cp_lease_t **) malloc(tunnels * sizeof(struct i2cp_lease_t *));
  for (t = 0; t < tunnels; t++)
    leases[t] = i2cp_lease_new_from_stream(stream);

  /* create a lease set response to the request */
  _client_msg_create_lease_set(self, session, tunnels, leases, 1);

  /* cleanup */
  for (t = 0; t < tunnels; t++)
    i2cp_lease_destroy(leases[t]);

}

static void
_client_on_msg(i2cp_client_t *self, uint8_t type, stream_t *stream, void *opaque)
{
  /* TODO: rewrite for performance, static array with func pointers index by type */
  switch (type)
  {
  case I2CP_MSG_SET_DATE:
    _client_on_msg_set_date(self, stream, opaque);
    break;

  case I2CP_MSG_DISCONNECT:
    _client_on_msg_disconnect(self, stream, opaque);
    break;

  case I2CP_MSG_PAYLOAD_MESSAGE:
    _client_on_msg_payload_message(self, stream, opaque);
    break;

  case I2CP_MSG_MESSAGE_STATUS:
    _client_on_msg_message_status(self, stream, opaque);
    break;

  case I2CP_MSG_DEST_REPLY:
    _client_on_msg_dest_reply(self, stream, opaque);
    break;

  case I2CP_MSG_BANDWIDTH_LIMITS:
    _client_on_msg_bandwidth_limits(self, stream, opaque);
    break;

  case I2CP_MSG_SESSION_STATUS:
    _client_on_msg_session_status(self, stream, opaque);
    break;

  case I2CP_MSG_REQUEST_VARIABLE_LEASESET:
    _client_on_msg_request_variable_leaseset(self, stream, opaque);
    break;

  case I2CP_MSG_HOST_REPLY:
    _client_on_msg_host_reply(self, stream, opaque);
    break;

  default:
    info(TAG, "%s", "recieved unhandled i2cp message.");
    break;
  }
}

/* TODO: This needs to be rewritten so that recv buffer is drained in chunks and multiple
         messages are processed and dispatched.
 */
static int
_client_recv_msg(i2cp_client_t *self, uint8_t type, stream_t *stream, int dispatch, void *opaque)
{
  int ret;
  uint32_t length;
  uint8_t msg_type;

  stream_reset(stream);

  length = msg_type = 0;

  /* recv i2cp message header into stream */
  ret = tcp_recv(self->tcp, stream, 5);
  if (ret == 0)
    _client_dispatch_on_disconnect(self, "Peer shutdown the connection.");
    

  if (ret != 5)
    error(TAG|PROTOCOL, "failed to receive header with reason: %s", strerror(errno));

  /* parse header */
  stream_seek_set(stream, 0);
  stream_in_uint32(stream, length);
  stream_in_uint8(stream, msg_type);

  /* Detect SSL connection */
  if (type == I2CP_MSG_SET_DATE && length > 0xffff)
    fatal(TAG|PROTOCOL, "unexpected response, your router is probably configured to use SSL.");

  /* Abort on unexpected message lengths */
  if (length > 0xffff)
    fatal(TAG|PROTOCOL, "unexpected message length, length > 0xffff");

  /* Is message of expected type */
  if (type && msg_type != type)
    error(TAG|PROTOCOL, "expected message type %d, received %d", type, msg_type);

  /* read the message body into begin of stream */
  stream_seek_set(stream, 0);
  ret = tcp_recv(self->tcp, stream, length);
  if (ret != length)
    error(TAG|PROTOCOL, "failed to receive %d bytes of data", length);

  /* mark end and reposition stream pointer to the actual message payload */
  stream_mark_end(stream);
  stream_seek_set(stream, 0);
 
   
  /* dispatch received message to handlers */
  if (dispatch)
    _client_on_msg(self, msg_type, stream, opaque);

  return ret;
}

/* Send a message, puts message on output queue if instructed wither a send is carried out
   syncronously
 */
static int
_client_send_msg(i2cp_client_t *self, uint8_t type, stream_t *stream, int queue)
{
  int ret;
  stream_t *s;

  ret = 0;
  s = malloc(sizeof(stream_t));
  stream_init(s, stream_length(stream) + 4 + 1);

  /* write i2cp message header */
  stream_out_uint32(s, stream_length(stream));
  stream_out_uint8(s, type);

  /* write the actual message */
  stream_out_stream(s, stream);

  stream_mark_end(s);

  if (queue)
  {
    debug(TAG|PROTOCOL, "Putting %d bytes message on output queue.", stream_length(s)); 
    /* put message on output queue */
    queue_lock(self->output_queue);
    queue_push(self->output_queue, s);
    queue_unlock(self->output_queue);
    ret = stream_length(s);
  }
  else
  {
    /* send the message directly */
    ret = tcp_send(self->tcp, s);
    stream_destroy(s);
    free(s);
  }

  return ret;
}

static void
_client_msg_get_date(i2cp_client_t *self, int queue)
{
  int ret;
  stream_t auth;

  debug(TAG|PROTOCOL, "%s", "Sending GetDateMessage.");
  stream_reset(&self->message_stream);
  stream_out_string(&self->message_stream, I2CP_CLIENT_VERSION, sizeof(I2CP_CLIENT_VERSION) - 1);

  /* write new 0.9.10 auth mapping if username property is set */
  if (self->properties[CLIENT_PROP_USERNAME])
  {
    stream_init(&auth, 512);
    stream_out_string(&auth, "i2cp.password", strlen("i2cp.password"));
    stream_out_uint8(&auth, '=');
    stream_out_string(&auth, self->properties[CLIENT_PROP_PASSWORD],
		      strlen(self->properties[CLIENT_PROP_PASSWORD]));
    stream_out_uint8(&auth, ';');
    stream_out_string(&auth, "i2cp.username", strlen("i2cp.username"));
    stream_out_uint8(&auth, '=');
    stream_out_string(&auth, self->properties[CLIENT_PROP_USERNAME],
		      strlen(self->properties[CLIENT_PROP_USERNAME]));
    stream_out_uint8(&auth, ';');
    stream_mark_end(&auth);

    stream_out_uint16(&self->message_stream, stream_length(&auth));
    stream_out_stream(&self->message_stream, &auth);

    stream_destroy(&auth);
  }

  stream_mark_end(&self->message_stream);
  ret = _client_send_msg(self, I2CP_MSG_GET_DATE, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending GetDateMessage.");
  
}

static void
_client_msg_create_session(i2cp_client_t *self, struct i2cp_session_config_t *config, int queue)
{
  int ret;
  debug(TAG|PROTOCOL, "%s", "Sending CreateSessionMessage.");
  stream_reset(&self->message_stream);
  i2cp_session_config_get_message(config, &self->message_stream);
  stream_mark_end(&self->message_stream);

  ret = _client_send_msg(self, I2CP_MSG_CREATE_SESSION, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending CreateSessionMessage.");
  
}

static void
_client_msg_create_lease_set(i2cp_client_t *self, struct i2cp_session_t *session,
			     uint8_t tunnels, struct i2cp_lease_t **leases, int queue)
{
  int ret, t;
  uint8_t nullbytes[256];
  stream_t leaseset;
  struct i2cp_session_config_t *session_cfg;
  struct i2cp_destination_t *session_destination;
  const i2cp_signature_keypair_t *signature_keys;

  debug(TAG|PROTOCOL, "%s", "Sending CreateLeaseSetMessage");

  stream_init(&leaseset, 4096);
  stream_reset(&self->message_stream);

  /* get the instance */
  session_cfg = i2cp_session_get_config(session);
  session_destination = i2cp_session_config_get_destination(session_cfg);
  signature_keys = i2cp_destination_signature_keypair(session_destination);

  memset(nullbytes, 0, sizeof(nullbytes));

  /* construct the message */
  stream_out_uint16(&self->message_stream, i2cp_session_get_id(session));
  stream_out_uint8p(&self->message_stream, nullbytes, 20);
  stream_out_uint8p(&self->message_stream, nullbytes, 256);

  /* build lease set stream and sign it */
  i2cp_destination_get_message(session_destination, &leaseset);
  stream_out_uint8p(&leaseset, nullbytes, 256);
  i2cp_crypto_signature_publickey_stream(i2cp_crypto_instance(), signature_keys, &leaseset);
  stream_out_uint8(&leaseset, tunnels);
  for (t = 0; t < tunnels; t++)
  {
    i2cp_lease_get_message(leases[t], &leaseset);
  }

  /* sign leasset stream */
  i2cp_crypto_sign_stream(i2cp_crypto_instance(), signature_keys, &leaseset);
  
  /* write signed leasset stream into message */
  stream_out_stream(&self->message_stream, &leaseset);

  stream_mark_end(&self->message_stream);

  /* send the message */
  ret = _client_send_msg(self, I2CP_MSG_CREATE_LEASE_SET, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending CreateSessionMessage.");
}

static void 
_client_msg_dest_lookup(i2cp_client_t *self, uint8_t *hash, int queue)
{
  int ret;
  debug(TAG|PROTOCOL, "%s", "Sending DestLookupMessage."); 
  stream_reset(&self->message_stream);
  
  stream_out_uint8p(&self->message_stream, hash, 32);
  stream_mark_end(&self->message_stream);

  ret = _client_send_msg(self, I2CP_MSG_DEST_LOOKUP, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending DestLookupMessage.");
}

static void
_client_msg_host_lookup(i2cp_client_t *self, struct i2cp_session_t *session,
			uint32_t request_id, uint32_t timeout,
			int type, void *data, size_t len, int queue)
{
  int ret;
  uint16_t session_id;

  debug(TAG|PROTOCOL, "%s", "Sending HostLookupMessage.");
  stream_reset(&self->message_stream);

  session_id = i2cp_session_get_id(session);

  stream_out_uint16(&self->message_stream, session_id);
  stream_out_uint32(&self->message_stream, request_id);
  stream_out_uint32(&self->message_stream, timeout);
  stream_out_uint8(&self->message_stream, type);

  if (type == HOST_LOOKUP_TYPE_HASH)
  {
    stream_out_uint8p(&self->message_stream, data, len);
  }
  else
  {
    stream_out_string(&self->message_stream, (char*)data, strlen(data));
  }

  stream_mark_end(&self->message_stream);

  ret = _client_send_msg(self, I2CP_MSG_HOST_LOOKUP, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending HostLookupMessage.");
}


static void 
_client_msg_destroy_session(i2cp_client_t *self, struct i2cp_session_t *session, int queue)
{
  int ret;
  uint16_t session_id;
  debug(TAG|PROTOCOL, "%s", "Sending DestroySessionMessage.");
  stream_reset(&self->message_stream);

  session_id = i2cp_session_get_id(session);
  
  stream_out_uint16(&self->message_stream, session_id);
  stream_mark_end(&self->message_stream);

  ret = _client_send_msg(self, I2CP_MSG_DESTROY_SESSION, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending DestroySessionMessage.");
}

static void
_client_msg_get_bandwidth_limits(i2cp_client_t *self, int queue)
{
  int ret;
  debug(TAG|PROTOCOL, "%s", "Sending GetBandwidthLimitsMessage.");
  stream_reset(&self->message_stream);

  ret = _client_send_msg(self, I2CP_MSG_GET_BANDWIDTH_LIMITS, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending GetBandwidthLimitsMessage.");
}

/* Keep non static dues to internal i2cp lib access from session.c */
void
_client_msg_send_message(i2cp_client_t *self, struct i2cp_session_t *session,
			 struct i2cp_destination_t *destination,
			 i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
			 stream_t *payload, uint32_t nonce, int queue)
{
  int ret;
  stream_t in, out;

  debug(TAG|PROTOCOL, "%s", "Sending SendMessageMessage.");
  stream_reset(&self->message_stream);

  /* write packet */
  stream_out_uint16(&self->message_stream, i2cp_session_get_id(session));
  i2cp_destination_get_message(destination, &self->message_stream);

  /* deflate payload and write to stream */
  stream_init(&in, 0xffff);
  stream_init(&out, 0xffff);

  /* write payload into stream to be deflated */
  stream_seek_set(payload, 0);
  stream_out_stream(&in, payload);
  stream_mark_end(&in);
  stream_seek_set(&in, 0);

  _client_stream_deflate(&in, &out);

  /* update gzip headers with protocol and ports */
  stream_seek_set(&out, 0);
  stream_skip(&out, 3);
  stream_skip(&out, 1);
  stream_out_uint16(&out, src_port);
  stream_out_uint16(&out, dest_port);
  stream_skip(&out, 1);
  stream_out_uint8(&out, protocol);

  stream_out_uint32(&self->message_stream, stream_length(&out));
  stream_out_stream(&self->message_stream, &out);
  stream_out_uint32(&self->message_stream, nonce);
  stream_mark_end(&self->message_stream);

  stream_destroy(&in);
  stream_destroy(&out);

  ret = _client_send_msg(self, I2CP_MSG_SEND_MESSAGE, &self->message_stream, queue);
  if (ret <= 0)
    error(TAG, "%s", "error while sending SendMessageMessage.");
}


struct i2cp_client_t *
i2cp_client_new(i2cp_client_callbacks_t *callbacks)
{
  i2cp_client_t *client = malloc(sizeof(i2cp_client_t));
  memset(client, 0, sizeof(i2cp_client_t));

  client->callbacks = callbacks;

  /* setup logger callback */
  client->logger.opaque = client;
  client->logger.on_log = _client_on_log_callback;
  i2cp_logger_init(&client->logger);

  stream_init(&client->output_stream, I2CP_MESSAGE_SIZE);
  stream_init(&client->message_stream, I2CP_MESSAGE_SIZE);

  _client_default_properties(client);

  /* setup tcp with default properties */
  client->tcp = tcp_new();
  tcp_set_property(client->tcp, TCP_PROP_ADDRESS, client->properties[CLIENT_PROP_ROUTER_ADDRESS]);
  tcp_set_property(client->tcp, TCP_PROP_PORT, client->properties[CLIENT_PROP_ROUTER_PORT]);

#ifdef WITH_GNUTLS
  tcp_set_property(client->tcp, TCP_PROP_USE_TLS, client->properties[CLIENT_PROP_ROUTER_USE_TLS]);
#endif

  client->lookups = stringmap_new(1000);
  client->lookup_requests = intmap_new(1000);
  client->output_queue = queue_new();

  return client;
}

void
i2cp_client_destroy(struct i2cp_client_t *self)
{
  if (i2cp_client_is_connected(self))
    i2cp_client_disconnect(self);

  stream_destroy(&self->message_stream);
  stream_destroy(&self->output_stream);

  free(self);
}

void
i2cp_client_set_property(struct i2cp_client_t *self, i2cp_client_property_t prop, const char *value)
{
  self->properties[prop] = value;

  /* set tcp properties */
  switch (prop)
  {
  case CLIENT_PROP_ROUTER_ADDRESS:
    tcp_set_property(self->tcp, TCP_PROP_ADDRESS, self->properties[CLIENT_PROP_ROUTER_ADDRESS]);
    break;

  case CLIENT_PROP_ROUTER_PORT:
    tcp_set_property(self->tcp, TCP_PROP_PORT, self->properties[CLIENT_PROP_ROUTER_PORT]);
    break;

  case CLIENT_PROP_ROUTER_USE_TLS:
#ifdef WITH_GNUTLS
    tcp_set_property(self->tcp, TCP_PROP_USE_TLS, self->properties[CLIENT_PROP_ROUTER_USE_TLS]);
#endif
    break;
  }
}

const char *
i2cp_client_get_property(struct i2cp_client_t *self, i2cp_client_property_t prop)
{
  return self->properties[prop];
}

void
i2cp_client_connect(struct i2cp_client_t *self)
{
  info(TAG, "connecting to i2cp at %s:%s",
       self->properties[CLIENT_PROP_ROUTER_ADDRESS],
       self->properties[CLIENT_PROP_ROUTER_PORT]);

  tcp_connect(self->tcp);

  if (!tcp_is_connected(self->tcp))
    return;

  stream_reset(&self->output_stream);
  stream_out_uint8(&self->output_stream, I2CP_PROTOCOL_INIT);
  stream_mark_end(&self->output_stream);

  debug(TAG|PROTOCOL, "%s", "Sending protocol byte message.");
  tcp_send(self->tcp, &self->output_stream);

  /* handshake get/set date */
  _client_msg_get_date(self, 0);
  _client_recv_msg(self, I2CP_MSG_SET_DATE, &self->message_stream, 1, NULL);
}

int
i2cp_client_is_connected(struct i2cp_client_t *self)
{
  return tcp_is_connected(self->tcp);
}

void
i2cp_client_disconnect(struct i2cp_client_t *self)
{
  info(TAG, "disconnection client %p", (void *)self);
  tcp_disconnect(self->tcp);
}

void
i2cp_client_create_session(struct i2cp_client_t *self, struct i2cp_session_t *session)
{
  struct i2cp_session_config_t *config;

  /* rejecet creating more sessions than there is allowed per client */
  if (self->session_count == I2CP_MAX_SESSIONS_PER_CLIENT)
  {
    warning(TAG, "%s", "Maximum number of session per client connection reached.");
    return;
  }

  /*
   * Force a few options for this i2cp implementation
   */
  config = i2cp_session_get_config(session);

  /* from 0.9.4 client versions set the following defaults  */
  i2cp_session_config_set_property(config, SESSION_CONFIG_PROP_I2CP_FAST_RECEIVE, "true");
  i2cp_session_config_set_property(config, SESSION_CONFIG_PROP_I2CP_MESSAGE_RELIABILITY, "none");

  /* send CreateSession message */
  _client_msg_create_session(self, config, 0);

  /* receive SessionStatus message */
  _client_recv_msg(self, I2CP_MSG_ANY, &self->message_stream, 1, session);
}

int
i2cp_client_process_io(struct i2cp_client_t *self)
{
  int ret;
  stream_t *stream;

  ret = 0;

  /* send messages in output queue */
  queue_lock(self->output_queue);
  stream = (stream_t *)queue_pop(self->output_queue);
  while(stream)
  {
    debug(TAG|PROTOCOL, "Sending %d bytes message", stream_length(stream)); 
    ret = tcp_send(self->tcp, stream);
    stream_destroy(stream);
    free(stream);

    if (ret < 0)
      return ret;

    if (ret == 0)
      break;

    stream = (stream_t *)queue_pop(self->output_queue);
  }
  queue_unlock(self->output_queue);

  /* drain incoming message */
  while (tcp_can_read(self->tcp))
  {
    ret = _client_recv_msg(self, I2CP_MSG_ANY, &self->message_stream, 1, NULL);
    if (ret < 0)
      return ret;

    if (ret == 0)
      break;
  }

  return ret;
}

uint32_t
i2cp_client_destination_lookup(struct i2cp_client_t *self,
			       struct i2cp_session_t *session, const char *address)
{
  char *pe;
  stream_t in, out;
  uint32_t request_id;
  _client_host_lookup_item_t *lup;

  if (!(self->router.capabilities & ROUTER_CAN_HOST_LOOKUP) && strlen(address) != (52+8))
  {
    warning(TAG, "Address '%s' is not a b32 address %d.", address, strlen(address));
    return 0;
  }

  stream_init(&in, 512);
  stream_init(&out, 512);

  /* if address is a b32 address lets decode into hash */
  if (strlen(address) == (52+8))
  {
    debug(TAG, "Lookup of b32 address detected, decode and use hash for faster lookup.");

    pe = strchr(address, '.');
    stream_out_uint8p(&in, address, pe - address);
    stream_mark_end(&in);
    stream_seek_set(&in, 0);
    i2cp_crypto_decode_stream(i2cp_crypto_instance(), CODEC_BASE32, &in, &out);

    /* TODO: Notify session about lookup failure */
    if (stream_length(&out) == 0)
      warning(TAG, "failed to decode hash of address '%s'.", address);
  }

  /* Create a client host lookup item for this request id and add it to the map */
  lup = _client_host_lookup_item_ctor(address, session);

  /* FIXME: using 0 as intmap key is invalid */
  request_id = (++self->lookup_request_id);
  intmap_put(self->lookup_requests, request_id, lup);

  if (self->router.capabilities & ROUTER_CAN_HOST_LOOKUP)
  {
    /* >= 0.9.10 dest host lookup by string or sha256 hash */
    if (stream_length(&out) == 0)
      _client_msg_host_lookup(self, session, request_id, 30000, HOST_LOOKUP_TYPE_HOST,
			      address, strlen(address), 1);
    else
      _client_msg_host_lookup(self, session, request_id, 30000, HOST_LOOKUP_TYPE_HASH,
			      out.data, stream_length(&out), 1);
  }
  else
  {
    /* pre 0.9.10 approach of dest lookup */
    stringmap_put(self->lookups, address, (void *)request_id);
    _client_msg_dest_lookup(self, out.data, 1);
  }

  stream_destroy(&in);
  stream_destroy(&out);

  return request_id;
}
