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
#include <pthread.h>

#include <i2cp/queue.h>

typedef struct _queue_item_t
{
  struct _queue_item_t *next;
  void *opaque;
} _queue_item_t;

typedef struct queue_t
{
  _queue_item_t *first;
  _queue_item_t *last;
  pthread_mutex_t lock;
} queue_t;

static _queue_item_t *_queue_item_ctor(void *data);
static void _queue_item_dtor(_queue_item_t *item);

static inline _queue_item_t *_queue_item_ctor(void *opaque)
{
  _queue_item_t *item;
  item = malloc(sizeof(_queue_item_t));
  item->next = NULL;
  item->opaque = opaque;
  return item;
}

static inline void _queue_item_dtor(_queue_item_t *item)
{
  free(item);
}

struct queue_t* queue_new()
{
  queue_t *queue;
  queue = malloc(sizeof(queue_t));
  memset(queue, 0, sizeof(queue_t));
  pthread_mutex_init(&queue->lock, NULL);
  return queue;
}

void queue_destroy(struct queue_t *self)
{
  _queue_item_t *item, *next;

  /* Free all items in queue */
  item = self->first;
  while(item)
  {
    next = item->next;
    /* TODO: add support for a free function to free up opaque data
             provided by host application. */
    free(item);
    item = next;
  }

  pthread_mutex_destroy(&self->lock);
}

void queue_push(struct queue_t *self, void *data)
{
  _queue_item_t *item;

  item = _queue_item_ctor(data);

  if (self->last) self->last->next = item;
  else self->first = item;
  self->last = item;
}

void *queue_pop(struct queue_t *self)
{
  void *opaque;
  _queue_item_t *item;

  if (self->first == NULL)
    return NULL;

  item = self->first;
  opaque = item->opaque;

  self->first = item->next;
  if (self->first == NULL)
    self->last = NULL;

  _queue_item_dtor(item);

  return opaque;
}

void queue_lock(struct queue_t *self)
{
  pthread_mutex_lock(&self->lock);
}

void queue_unlock(struct queue_t *self)
{
  pthread_mutex_unlock(&self->lock);
}
