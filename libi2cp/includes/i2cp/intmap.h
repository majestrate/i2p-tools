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


#ifndef _intmap_h
#define _intmap_h

#include <inttypes.h>

struct intmap_t;

typedef void (intmap_foreach_func)(const uint32_t key, void *value, void *opaque);

struct intmap_t *intmap_new(uint32_t capacity);
void intmap_destroy(struct intmap_t *self);

const void *intmap_get(struct intmap_t *self, const uint32_t key);
void intmap_put(struct intmap_t *self, const uint32_t key, void *opaque);
void intmap_remove(struct intmap_t *self, const uint32_t key);

void intmap_foreach(struct intmap_t *self, intmap_foreach_func *callback, void *opaque);


#endif
