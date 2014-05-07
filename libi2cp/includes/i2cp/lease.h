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

#ifndef _lease_h
#define _lease_h

#include <i2cp/stream.h>

struct i2cp_lease_t;

struct i2cp_lease_t *i2cp_lease_new_from_stream(stream_t *stream);
void i2cp_lease_destroy(struct i2cp_lease_t *lease);
void i2cp_lease_get_message(struct i2cp_lease_t *lease, stream_t *stream);
#endif
