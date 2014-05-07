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
#include <i2cp/stringmap.h>

typedef struct _pair_t
{
  char *key;
  void *opaque;
} _pair_t;

typedef struct _bucket_t
{
  uint32_t size;
  _pair_t *pairs;
} _bucket_t;

typedef struct stringmap_t
{
  uint32_t size;
  _bucket_t *buckets;
} stringmap_t;

static inline uint32_t _hash(const char *str)
{
  uint8_t c;
  uint32_t hash = 5381;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c;

  return hash;
}

static inline const _pair_t *
_find_pair_by_key(const _bucket_t *bucket, const char *key)
{
  int i;

  if (bucket->size == 0)
    return NULL;

  for (i = 0; i < bucket->size; i++)
  {
    if (bucket->pairs[i].key == NULL || bucket->pairs[i].opaque == NULL)
      continue;

    if (strcmp(bucket->pairs[i].key, key) == 0)
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
    if (bucket->pairs[i].key == NULL)
      return &bucket->pairs[i];
  }

  return NULL;
}

stringmap_t *
stringmap_new(uint32_t capacity)
{
  stringmap_t *sm;

  sm = malloc(sizeof(stringmap_t));
  sm->size = capacity;
  sm->buckets = malloc(sm->size * sizeof(_bucket_t));
  memset(sm->buckets, 0, sm->size * sizeof(_bucket_t));
  
  return sm;
}

void
stringmap_destroy(stringmap_t *self)
{
  int i, j;

  for (i = 0; i < self->size; i++)
  {
    /* free each key of every pairs */
    for (j = 0; j < self->buckets[i].size; j++)
      free(self->buckets[i].pairs[j].key);

    /* free pairs if any */
    if (self->buckets[i].pairs)
      free(self->buckets[i].pairs);
  }

  free(self->buckets);
  free(self);
}

const void *
stringmap_get(stringmap_t *self, const char *key)
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
stringmap_put(stringmap_t *self, const char *key, void *opaque)
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
    pair->key = strdup(key);
    pair->opaque = opaque;
    return;
  }

  /* realloc a new slot */
  self->buckets[index].size++;
  new_pairs = realloc(self->buckets[index].pairs, self->buckets[index].size * sizeof(_pair_t));
  self->buckets[index].pairs = new_pairs;
  
  /* append key/value to last pair in chain */
  pair = (_pair_t *)&self->buckets[index].pairs[self->buckets[index].size - 1];
  pair->key = strdup(key);
  pair->opaque = opaque;
}

void
stringmap_remove(struct stringmap_t *self, const char *key)
{
  _pair_t *pair;
  uint32_t index;

  index = _hash(key) % self->size;
  pair = (_pair_t *) _find_pair_by_key(&self->buckets[index], key);
  if (!pair)
    return;

  free(pair->key);
  pair->key = pair->opaque = NULL;
}

void
stringmap_foreach(stringmap_t *self, stringmap_foreach_func *callback, void *opaque)
{
  int i, j;
  for (i = 0; i < self->size; i++)
  {
    /* free each key of every pairs */
    for (j = 0; j < self->buckets[i].size; j++)
    {
      /* if nothing stored in key run to next */
      if (self->buckets[i].pairs[j].key == NULL)
	continue;

      /* dispatch on item to caller */
      callback(self->buckets[i].pairs[j].key, self->buckets[i].pairs[j].opaque, opaque);
    }
  }
}
