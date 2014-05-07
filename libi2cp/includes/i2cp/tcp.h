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


#ifndef _tcp_h
#define _tcp_h

#include <stdlib.h>
#include <inttypes.h>

#include <i2cp/config.h>

typedef enum tcp_property_t
{
  TCP_PROP_ADDRESS,
  TCP_PROP_PORT,

#ifdef WITH_GNUTLS
  TCP_PROP_USE_TLS,
  TCP_PROP_TLS_CLIENT_CERTIFICATE,
#endif

  NR_OF_TCP_PROPERTIES
} tcp_property_t;

struct tcp_t;

void tcp_init();
void tcp_deinit();

struct tcp_t *tcp_new();
void tcp_destroy(struct tcp_t *self);

void tcp_set_property(struct tcp_t *self, tcp_property_t prop, const char *value);
const char * tcp_get_property(struct tcp_t *self, tcp_property_t prop);

void tcp_connect(struct tcp_t *self);
void tcp_disconnect(struct tcp_t *self);
int tcp_is_connected(struct tcp_t *self);

int tcp_send(struct tcp_t *self, struct stream_t *stream);
int tcp_recv(struct tcp_t *self, struct stream_t *stream, size_t length);

int tcp_can_read(struct tcp_t *self);

#endif
