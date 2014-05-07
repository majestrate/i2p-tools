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
#include <i2cp/stringmap.h>
#include <i2cp/logger.h>

#define COUNT 1000

static void _foreach_func(const char *key, void *value, void *opaque)
{
  uint32_t *counter = (uint32_t *)opaque;
  (*counter)++;
}

int main(int argc, char **argv)
{
  int i;
  uint32_t counter;
  char key[64];
  long value;

  struct stringmap_t *sm = stringmap_new(100);

  /* populate map */
  for (i = 0; i < COUNT; i++)
  {
    sprintf(key, "%d", i);
    stringmap_put(sm, key, (void *)(long)i);
  }

  /* random gets */
  for (i = 0; i < 100; i++)
  {
    int r = rand() % COUNT;
    sprintf(key, "%d", r);
    value = (long)stringmap_get(sm, key);
    if (r != value)
      fatal(STRINGMAP, " %s(%d) != %d", key, r, value);
  }

  /* test foreach func */
  counter = 0;
  stringmap_foreach(sm, _foreach_func, (void *)&counter);
  if (counter != COUNT)
    fatal(STRINGMAP, " foreach %d != %d", COUNT, counter);

  /* remove all but a single item */
  for (i = 0; i < COUNT-1; i++)
  {
    sprintf(key, "%d", i);
    stringmap_remove(sm, key);
  }

  /* verify last item */
  sprintf(key, "%d", COUNT-1);
  value = (long)stringmap_get(sm, key);
  if (COUNT-1 != value)
    fatal(STRINGMAP, " %s(%d) != %d", key, COUNT-1, value);

  /* remove last */
  stringmap_remove(sm, key);

  stringmap_destroy(sm);

  return 0;
}
