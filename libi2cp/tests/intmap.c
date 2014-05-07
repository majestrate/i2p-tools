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
#include <i2cp/intmap.h>
#include <i2cp/logger.h>

#define COUNT 1000

static void _foreach_func(const uint32_t key, void *value, void *opaque)
{
  uint32_t *counter = (uint32_t *)opaque;
  (*counter)++;
}

int main(int argc, char **argv)
{
  int i;
  uint32_t counter;
  long value;

  struct intmap_t *sm = intmap_new(100);

  /* populate map */
  for (i = 1; i <= COUNT; i++)
    intmap_put(sm, i, (void *)(long)i);

  /* random gets */
  for (i = 0; i < 100; i++)
  {
    uint32_t r = rand() % COUNT;
    value = (long)intmap_get(sm, r);
    if (r != value)
      fatal(INTMAP, "%d != %d", r, value);
  }

  /* test foreach func */
  counter = 0;
  intmap_foreach(sm, _foreach_func, (void *)&counter);
  if (counter != COUNT)
    fatal(INTMAP, " foreach %d != %d", COUNT, counter);

  /* remove all but a single item */
  for (i = 0; i < COUNT-1; i++)
    intmap_remove(sm, i);

  /* verify last item */
  value = (long)intmap_get(sm, COUNT-1);
  if (COUNT-1 != value)
    fatal(INTMAP, "%d != %d", COUNT-1, value);

  /* remove last */
  intmap_remove(sm, COUNT);

  intmap_destroy(sm);

  return 0;
}
