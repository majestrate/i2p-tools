i2cp C Library
==============
License: GPLv3
Author: Oliver Queen <oliver@mail.i2p>


Overview
--------
A C library implementation of the I2CP protocol.

> The I2P Client Protocol (I2CP) exposes a strong separation of concerns
> between the router and any client that wishes to communicate over the
> network. It enables secure and asynchronous messaging by sending and
> receiving messages over a single TCP socket, yet never exposing any
> private keys and authenticating itself to the router only through
> signatures. With I2CP, a client application tells the router who they
> are (their "destination"), what anonymity, reliability, and latency
> tradeoffs to make, and where to send messages. In turn the router uses
> I2CP to tell the client when any messages have arrived, and to request
> authorization for some tunnels to be used.

This is the lowest external client protocol stack that can communicate
with the i2p router. Datagrams, streaming, bob, sam, streamr etc. is
layered on top of this protocol stack.

This library supports raw and repliable datagrams, streams will be
supported in the streaming library.

libi2cp does internally setup hardcoded default options for i2p router
connection, as follow:

    i2cp.tcp.host = 127.0.0.1
	i2cp.tcp.port = 7654
	i2cp.tcp.SSL = 0

then it tries to read a user configuration file `~/.i2cp.conf` which
will override the hardcoded defaults. This is done by the libi2cp
regardless of what package is using it such as `libstreamr` or alike.

Using bundled tools
-------------------
libi2cp comes with a few bundled tools which usage is described below.

The tool `i2cp-lookup` is used to lookup destinations of b32 adresses.
Version 0.9.10 of the i2p router supports a new lookup message which also
can resolve registered hostnames to destinations.

`i2cp-echo` is an echo datagram service/client example tool included, which
shows how to use the api and communicate using repliable datagrams over i2p.
Starting the tool without argument will start the echo service. The b32 address
of the service is written to the console. Provide a b32 address to the commandline
for making the tool become a echo client and a `hello world` message will be sent
and echoed back from the service.

Who is using i2cp
-----------------
Project|Description
:---|:---
libstreamr|A C library implemenation of Streamr. This library contains a tool named Streamr which is used as a consumer or a producer of a audio/video stream. More information about this project is found at [libstreamr.git](http://git.repo.i2p/w/libstreamr.git) repositor.

Building the source
-------------------
Requires: cmake, libnettle
Optional: gnutls >= 3.1.4

Prepare compilation of library with the following command in the
source tree.

`mkdir build && cd build && cmake ..`

And build the library and tools with a simple:

`make`


