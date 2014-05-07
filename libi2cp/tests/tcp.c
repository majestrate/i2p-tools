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

#include <stdio.h>
#include <string.h>

#include <i2cp/stream.h>

#include <i2cp/tcp.h>

int main(int argc, char **argv)
{
  char msg[2048] = {0};
  stream_t req, res;
  struct tcp_t *tcp;
  const char * address = "www.google.com";
  const char * port = "80";

  if (argv[1])
    address = argv[1];

  if (argv[2])
    port = argv[2];

  sprintf(msg, "GET / HTTP/1.1\nHost: %s\n\n", address);

  /* allocate and setup io streams*/
  stream_init(&req, 4096);
  stream_init(&res, 4096);

  stream_out_uint8p(&req, msg, strlen(msg));
  stream_mark_end(&req);

  /* initialize tcp */
  tcp_init();

  /* setup tcp and connect*/
  tcp = tcp_new();
  tcp_set_property(tcp, TCP_PROP_ADDRESS, address);
  tcp_set_property(tcp, TCP_PROP_PORT, port);

#ifdef WITH_GNUTLS
  tcp_set_property(tcp, TCP_PROP_USE_TLS, atoi(port) == 443 ? "1" : "0");
#endif

  tcp_connect(tcp);

  /* write http request and recv response */
  tcp_send(tcp, &req);

  tcp_recv(tcp, &res, 4096);

  fprintf(stdout,"%s", res.data);

  tcp_disconnect(tcp);

  tcp_destroy(tcp);

  tcp_deinit();

  return 0;
}
