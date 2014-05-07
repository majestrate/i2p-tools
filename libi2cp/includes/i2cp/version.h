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

#ifndef _version_h
#define _version_h

#include <inttypes.h>

struct stream_t;
struct i2cp_version_t;

/** \brief Constructs a version instance from stream. */
struct i2cp_version_t *i2cp_version_new_from_stream(struct stream_t *stream);

/** \brief Destroys a version instance. */
void i2cp_version_destroy(struct i2cp_version_t *self);

/** \brief Get the string represenetation of the version instance.*/
const char *i2cp_version_to_string(struct i2cp_version_t *self);

/** \brief Compare a version.
 \returns < 0 if version is smaller, == 0 if version is equal, > 0 if version is larger. */
int i2cp_version_cmp(struct i2cp_version_t *self, uint16_t major, uint16_t minor,
		     uint16_t micro, uint16_t qualifier);

#endif
