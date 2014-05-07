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


#ifndef _stringmap_h
#define _stringmap_h

#include <inttypes.h>

struct stringmap_t;

typedef void (stringmap_foreach_func)(const char *key, void *value, void *opaque);

struct stringmap_t *stringmap_new(uint32_t capacity);
void stringmap_destroy(struct stringmap_t *self);

const void *stringmap_get(struct stringmap_t *self, const char *key);
void stringmap_put(struct stringmap_t *self, const char *key, void *opaque);
void stringmap_remove(struct stringmap_t *self, const char *key);

void stringmap_foreach(struct stringmap_t *self, stringmap_foreach_func *callback, void *opaque);


#endif
