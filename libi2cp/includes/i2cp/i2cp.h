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


#ifndef _i2cp_h
#define _i2cp_h

#include <stdlib.h>
#include <i2cp/tcp.h>
#include <i2cp/client.h>
#include <i2cp/session.h>
#include <i2cp/session_config.h>
#include <i2cp/datagram.h>
#include <i2cp/destination.h>
#include <i2cp/certificate.h>
#include <i2cp/lease.h>
#include <i2cp/crypto.h>
#include <i2cp/session_config.h>
#include <i2cp/stringmap.h>
#include <i2cp/queue.h>


void i2cp_init();
void i2cp_deinit();

#endif
