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


#ifndef _queue_h
#define _queue_h

struct queue_t;
struct queue_t* queue_new();
void queue_destroy(struct queue_t *self);
void queue_push(struct queue_t *self, void *data);
void *queue_pop(struct queue_t *self);

void queue_lock(struct queue_t *self);
void queue_unlock(struct queue_t *self);

#endif
