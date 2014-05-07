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
#include <stdlib.h>
#include <memory.h>
#include <i2cp/intmap.h>

typedef struct _pair_t
{
  uint32_t key;
  void *opaque;
} _pair_t;

typedef struct _bucket_t
{
  uint32_t size;
  _pair_t *pairs;
} _bucket_t;

typedef struct intmap_t
{
  uint32_t size;
  _bucket_t *buckets;
} intmap_t;

static inline uint32_t _hash(uint32_t key)
{
  uint32_t x = key;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x);
  return x;
}

static inline const _pair_t *
_find_pair_by_key(const _bucket_t *bucket, uint32_t key)
{
  int i;

  if (bucket->size == 0)
    return NULL;

  for (i = 0; i < bucket->size; i++)
  {
    if (bucket->pairs[i].key == 0 || bucket->pairs[i].opaque == NULL)
      continue;

    if (bucket->pairs[i].key == key)
      return &bucket->pairs[i];
  }

  return NULL;
}

static inline const _pair_t *
_find_pair_empty_slot(const _bucket_t *bucket)
{
  int i;

  if (bucket->size == 0)
    return NULL;

  for (i = 0; i < bucket->size; i++)
  {
    if (bucket->pairs[i].key == 0)
      return &bucket->pairs[i];
  }

  return NULL;
}

intmap_t *
intmap_new(uint32_t capacity)
{
  intmap_t *im;

  im = malloc(sizeof(intmap_t));
  im->size = capacity;
  im->buckets = malloc(im->size * sizeof(_bucket_t));
  memset(im->buckets, 0, im->size * sizeof(_bucket_t));
  
  return im;
}

void
intmap_destroy(intmap_t *self)
{
  int i;

  for (i = 0; i < self->size; i++)
  {
    /* free pairs if any */
    if (self->buckets[i].pairs)
      free(self->buckets[i].pairs);
  }

  free(self->buckets);
  free(self);
}

const void *
intmap_get(intmap_t *self, const uint32_t key)
{
  uint32_t index;
  const _pair_t *pair;

  index = _hash(key) % self->size;
  pair = _find_pair_by_key(&self->buckets[index], key);
  if (!pair)
    return NULL;

  return pair->opaque;
}

void
intmap_put(intmap_t *self, const uint32_t key, void *opaque)
{
  uint32_t index;
  _pair_t *pair, *new_pairs;

  index = _hash(key) % self->size;
  pair = (_pair_t *)_find_pair_by_key(&self->buckets[index], key);

  /* if pair exists update value and return */
  if (pair)
  {
    pair->opaque = opaque;
    return;
  }

  /* find empty slot */
  pair = (_pair_t *)_find_pair_empty_slot(&self->buckets[index]);
  if (pair)
  {
    pair->key = key;
    pair->opaque = opaque;
    return;
  }

  /* realloc a new slot */
  self->buckets[index].size++;
  new_pairs = realloc(self->buckets[index].pairs, self->buckets[index].size * sizeof(_pair_t));
  self->buckets[index].pairs = new_pairs;
  
  /* append key/value to last pair in chain */
  pair = (_pair_t *)&self->buckets[index].pairs[self->buckets[index].size - 1];
  pair->key = key;
  pair->opaque = opaque;
}

void
intmap_remove(struct intmap_t *self, const uint32_t key)
{
  _pair_t *pair;
  uint32_t index;

  index = _hash(key) % self->size;
  pair = (_pair_t *) _find_pair_by_key(&self->buckets[index], key);
  if (!pair)
    return;

  pair->key = 0; 
  pair->opaque = NULL;
}

void
intmap_foreach(intmap_t *self, intmap_foreach_func *callback, void *opaque)
{
  int i, j;
  for (i = 0; i < self->size; i++)
  {
    /* free each key of every pairs */
    for (j = 0; j < self->buckets[i].size; j++)
    {
      /* if nothing stored in key run to next */
      if (self->buckets[i].pairs[j].key == 0)
	continue;

      /* dispatch on item to caller */
      callback(self->buckets[i].pairs[j].key, self->buckets[i].pairs[j].opaque, opaque);
    }
  }
}
