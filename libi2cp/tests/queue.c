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
#include <i2cp/queue.h>
#include <i2cp/logger.h>

#define COUNT 1000
#define TAG QUEUE

int main(int argc, char **argv)
{
  long i;

  struct queue_t *q = queue_new();

  for (i = 0; i < COUNT; i++)
  {
    queue_push(q, (void *)i);
  }

  for (i = 0; i < COUNT; i++)
  {
    if ((long)queue_pop(q) != i)
      fatal(TAG, "Failed to verify data in queue.");
  }

  queue_destroy(q);

  return 0;
}
