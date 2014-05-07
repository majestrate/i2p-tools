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

#include <i2cp/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/time.h>


#ifdef _WIN32
#include <Winsock2.h>
#else
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef WITH_GNUTLS
#include <gnutls/gnutls.h>

#ifndef GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT
#define GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ((unsigned int)-1)
#endif

#endif

#include <i2cp/logger.h>
#include <i2cp/stream.h>
#include <i2cp/tcp.h>

#define TAG TCP

#define CAFILE "/etc/ssl/certs/ca-certificates.crt"

typedef struct tcp_t
{
  int socket;
  struct addrinfo *server;

#ifdef WITH_GNUTLS
  gnutls_session_t session;
  gnutls_certificate_credentials_t creds;
#endif

  const char *properties[NR_OF_TCP_PROPERTIES];
  
} tcp_t;

#ifdef WITH_GNUTLS
static int
_verify_certificate_callback(gnutls_session_t session)
{
  unsigned int status;
  int ret, type;
  const char *hostname;
  gnutls_datum_t out;

  /* read hostname */
  hostname = gnutls_session_get_ptr(session);

  /* This verification function uses the trusted CAs in the credentials
   * structure. So you must have installed one or more CA certificates.
   */
  ret = gnutls_certificate_verify_peers3(session, hostname, &status);
  if (ret < 0) {
    error(TAG, "server certificate is not trusted.");
    return GNUTLS_E_CERTIFICATE_ERROR;
  }

  /* fetch status string and print */
  type = gnutls_certificate_type_get(session);
  ret = gnutls_certificate_verification_status_print(status, type, &out, 0);
  if (ret < 0)
  {
    error(TAG, "failed to get verification status string.");
    return GNUTLS_E_CERTIFICATE_ERROR;
  }

  /* Certificate is not trusted */
  if (status != 0)
  {
    warning(TAG, "%s", out.data);
    status = 0;
  }
  else
  {
    info(TAG, "%s", out.data);
  }

  gnutls_free(out.data);

  return status;
}
#endif // WITH_GNUTLS

void tcp_init()
{
#ifdef _WIN32
  int res;
  WSADATA data;
  res = WSAStartup(0x0202, &data);
  if (res != 0)
    error(TAG, "Failed to initialize Winsock, code %d.", res);
  
#endif

#ifdef WITH_GNUTLS
  gnutls_global_init();
#endif
}

void tcp_deinit()
{

#ifdef _WIN32
  WSAClenup();
#endif

#ifdef WITH_GNUTLS
  gnutls_global_deinit();
#endif
}

tcp_t *tcp_new()
{
  tcp_t *tcp;

  tcp = malloc(sizeof(tcp_t));
  if (tcp == NULL)
    return NULL;

  memset(tcp, 0, sizeof(tcp_t));
  tcp->socket = -1;
  debug(TAG, "new context %p", (void *)tcp);
  return tcp;
}

void tcp_destroy(struct tcp_t *self)
{  
  debug(TAG, "destroy context %p", (void *)self);

  if (tcp_is_connected(self))
    tcp_disconnect(self);

  free(self);
}

void tcp_set_property(struct tcp_t *self, tcp_property_t prop, const char *value)
{
  self->properties[prop] = value;
}

const char * tcp_get_property(struct tcp_t *self, tcp_property_t prop)
{
  return self->properties[prop];
}

void tcp_connect(struct tcp_t *self)
{
  int ret;
 
  if (tcp_is_connected(self))
  {
    info(TAG, "context %p is already connected to a socket", (void *)self);
    return;
  }

  debug(TAG, "context %p connect", (void *)self);

  /* create socket and setup addr */
  if (getaddrinfo(self->properties[TCP_PROP_ADDRESS], self->properties[TCP_PROP_PORT], NULL, &self->server) != 0)
  {
    error(TAG, "failed to lookup hostname %s with reason; %s",
	  self->properties[TCP_PROP_ADDRESS], strerror(errno));
    return;
  }

  self->socket = socket(self->server->ai_family, self->server->ai_socktype, self->server->ai_protocol);
  if (self->socket < 0)
    error(TAG, "%s", "failed to create socket.");

  /* connect */
  ret = connect(self->socket, self->server->ai_addr, self->server->ai_addrlen);
  if (ret < 0)
  {
    error(TAG, "failed to connect to host '%s' with reason; %s",
	  self->properties[TCP_PROP_ADDRESS], strerror(errno));
    close(self->socket);
    self->socket = 0;
    return;
  }

  /* setup TLS if used */
#ifdef WITH_GNUTLS
  if (self->properties[TCP_PROP_USE_TLS] && atoi(self->properties[TCP_PROP_USE_TLS]) == 1)
  {

    /* setup x509 parts */
    gnutls_certificate_allocate_credentials (&self->creds);
    gnutls_certificate_set_x509_trust_file(self->creds, CAFILE, GNUTLS_X509_FMT_PEM);
    gnutls_certificate_set_verify_function(self->creds, _verify_certificate_callback);

#if 0
    /* setup x509 client certificate */
    gnutls_certificate_set_x509_key_file (self->creds,"cert.pem", "key.pem", GNUTLS_X509_FMT_PEM);
#endif

    gnutls_init (&self->session, GNUTLS_CLIENT);

    gnutls_priority_set_direct (self->session, "NORMAL", NULL);
    gnutls_credentials_set(self->session, GNUTLS_CRD_CERTIFICATE, self->creds);

    gnutls_transport_set_int(self->session, self->socket);
    gnutls_handshake_set_timeout(self->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
    do {
      ret = gnutls_handshake(self->session);
    } while(ret < 0 && gnutls_error_is_fatal(ret) == 0);

    if (ret < 0)
    {
      gnutls_deinit(self->session);
      error(TAG, "TLS handshake failed with reason; %s" , gnutls_strerror(ret));
      close(self->socket);
      self->socket = 0;
      return;
    }

    char *desc = gnutls_session_get_desc(self->session);
    info(TAG, "TLS session info: %s", desc);
    gnutls_free(desc);
  }
#endif

}

void tcp_disconnect(struct tcp_t *self)
{
  debug(TAG, "context %p disconnect", (void *)self);

#ifdef WITH_GNUTLS
  if (self->properties[TCP_PROP_USE_TLS] && atoi(self->properties[TCP_PROP_USE_TLS]) == 1)
  {
    gnutls_bye(self->session, GNUTLS_SHUT_RDWR);
    gnutls_certificate_free_credentials(self->creds);
  }
#endif
  shutdown(self->socket, SHUT_RDWR);
  self->socket = -1;
}

int tcp_is_connected(struct tcp_t *self)
{
  return (self->socket > 0);
}

int tcp_can_read(struct tcp_t *self)
{
  int ret;
  fd_set s;
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&s);
  FD_SET(self->socket, &s);
  ret = select(self->socket + 1, &s, NULL, NULL, &tv);
  if (ret < 0)
    return ret;

  return FD_ISSET(self->socket, &s);
}

int tcp_send(struct tcp_t *self, stream_t *stream)
{
  stream_seek_set(stream, 0);
  debug(TAG, "Sending %ld bytes to peer.", stream_length(stream));
#ifdef WITH_GNUTLS
  if (self->properties[TCP_PROP_USE_TLS] && atoi(self->properties[TCP_PROP_USE_TLS]) == 1)
  {
    return gnutls_record_send (self->session, stream->p, stream_length(stream));
  }
#endif

  return send(self->socket, stream->p, stream_length(stream), 0);
}

int tcp_recv(struct tcp_t *self, stream_t *stream, size_t length)
{
  int ret;
  debug(TAG, "Receiving %ld bytes from peer.", length);

#ifdef WITH_GNUTLS
  if (self->properties[TCP_PROP_USE_TLS] && atoi(self->properties[TCP_PROP_USE_TLS]) == 1)
    ret = gnutls_record_recv (self->session, stream->p, length);
  else
#endif
    ret = recv(self->socket, stream->p, length, 0);

  if (ret > 0)
    stream->p += ret;

  if (ret == 0)
    self->socket = 0;

  return ret;
}
