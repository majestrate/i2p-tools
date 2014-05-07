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
#include <i2cp/version.h>
#include <i2cp/stream.h>
#include <i2cp/logger.h>

#define VER1 "1"
#define FVER1 "1.0.0.0"
#define VER2 "1.2"
#define FVER2 "1.2.0.0"
#define VER3 "1.2.3"
#define FVER3 "1.2.3.0"
#define VER4 "1.2.3.4"

void _do_test(const char *vstr, const char *fstr,
	      uint16_t maj, uint16_t min, uint16_t mic, uint16_t qual)
{
  stream_t s;
  struct i2cp_version_t *version;

  stream_init(&s, 128);
  stream_reset(&s);
  stream_out_uint8(&s, strlen(vstr));
  stream_out_uint8p(&s, vstr, strlen(vstr));
  stream_mark_end(&s);
  stream_seek_set(&s, 0);

  /* construct a version from stream */
  version = i2cp_version_new_from_stream(&s);
  if (version == NULL)
    fatal(VERSION, "Failed to create version instance of string '%s'", vstr);

  if (strcmp(i2cp_version_to_string(version), fstr) != 0)
    fatal(VERSION, "Failed to parse version string %s != %s",
	  i2cp_version_to_string(version), fstr);

  if (i2cp_version_cmp(version, maj, min, mic, qual) >= 0)
    fatal(VERSION, "Failed to compare version, %s >= %d.%d.%d.%d",
	  vstr, maj, min, mic, qual);

  i2cp_version_destroy(version);
  stream_destroy(&s);
}

int main(int argc, char **argv)
{
  _do_test(VER1, FVER1, 1, 1, 0, 0);
  _do_test(VER2, FVER2, 1, 2, 1, 0);
  _do_test(VER3, FVER3, 1, 2, 3, 1);
  _do_test(VER4,  VER4, 1, 2, 3, 5);
  return 0;
}
