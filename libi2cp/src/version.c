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

#include <inttypes.h>

#include <i2cp/version.h>
#include <i2cp/stream.h>

/** Represents a router version.
    \todo Add support for matching protcol message to a version so i2cp library
          can work transparent with the changes over time.
	  First off add something like: int i2cp_version_can_message(self, I2CP_MSG_HOST_LOOKUP)
	  which will only return != 0 if router version is >= 0.9.10
*/
typedef struct i2cp_version_t {
  uint16_t major;
  uint16_t minor;
  uint16_t micro;
  uint16_t qualifier;

  char *version;

} i2cp_version_t;

static void
_version_string_parse(i2cp_version_t *self, char *string)
{
  int i;
  char *p[4];
  char *d;

  i = 0;
  p[0] = p[1] = p[2] = p[3] = NULL;
  d = string;
  p[i++] = string;
  while (*d != 0)
  {
    if (*d == '.')
    {
      *d = '\0';
      p[i] = d+1;
      i++;
    }
    d++;
  }
  
  self->major = atoi(p[0]);
  if (p[1])
     self->minor = atoi(p[1]);
  if (p[2])
    self->micro = atoi(p[2]);
  if (p[3])
    self->qualifier = atoi(p[3]);
}

static void
_version_to_string(i2cp_version_t *self)
{
  self->version = (char *)malloc((4*4) + 1);
  snprintf(self->version, (4*4)+1, "%d.%d.%d.%d",
	   self->major, self->minor, self->micro, self->qualifier);
}

i2cp_version_t *
i2cp_version_new_from_stream(stream_t *stream)
{
  uint8_t len;
  i2cp_version_t *version;
  char version_string[256];

  version = malloc(sizeof(i2cp_version_t));
  memset(version, 0, sizeof(i2cp_version_t));
  memset(version_string, 0, 256);

  stream_in_uint8(stream, len);
  stream_in_uint8p(stream, version_string, len);

  /* parse string into version */
  _version_string_parse(version, version_string);

  /* parse version into string representation */
  _version_to_string(version);

  return version;
}


void
i2cp_version_destroy(i2cp_version_t *self)
{
  free(self->version);
  free(self);
}


const char *
i2cp_version_to_string(struct i2cp_version_t *self)
{
  return self->version;
}

int
i2cp_version_cmp(struct i2cp_version_t *self, uint16_t major, uint16_t minor,
		     uint16_t micro, uint16_t qualifier)
{
  if (self->major != major)
    return (self->major - major) > 0 ? 1 : -1;

  if (self->minor != minor)
    return (self->minor - minor) > 0 ? 1 : -1;

  if (self->micro != micro)
    return (self->micro - micro) > 0 ? 1 : -1;

  if (self->qualifier != qualifier)
    return (self->qualifier - qualifier) > 0 ? 1 : -1;

  /* full match */
  return 0;
}
