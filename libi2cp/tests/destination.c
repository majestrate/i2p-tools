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

#include <i2cp/destination.h>
#include <i2cp/stream.h>

#define TAG TEST

int _test_random_destination()
{
  struct i2cp_destination_t *dest[2];

  dest[0] = i2cp_destination_new();
  dest[1] = i2cp_destination_new();

  if (strcmp(i2cp_destination_b32(dest[0]), i2cp_destination_b32(dest[1])) == 0)
    fatal(TAG, "%s", "Random dest[0] == dest[1]");
  
  i2cp_destination_destroy(dest[0]);
  i2cp_destination_destroy(dest[1]);
  return 1;
}

int _test_destination_from_stream()
{
  stream_t stream;
  const char *sb, *sa;
  struct i2cp_destination_t *db, *da;

  stream_init(&stream, 4096);

  /* create initial random destination */
  db = i2cp_destination_new();
  sb = i2cp_destination_b32(db);

  /* get destination into stream */
  i2cp_destination_get_message(db, &stream);
  stream_seek_set(&stream, 0);

  /* create new destination from stream*/
  da = i2cp_destination_new_from_message(&stream);
  if (da == NULL)
    fatal(TAG, "%s", "Failed to create destination from stream.");
  sa = i2cp_destination_b32(da);

  /* does the b32 addres of before and after differ ? */
  if (strcmp(sb, sa) != 0)
    fatal(TAG, "%s != %s", sb, sa);

  i2cp_destination_destroy(da);
  i2cp_destination_destroy(db);

  stream_destroy(&stream);
  return 1;
}

int _test_destination_from_base64()
{
  stream_t stream;
  const char *sb, *sa;
  struct i2cp_destination_t *db, *da;

  stream_init(&stream, 4096);

  /* create initial random destination */
  db = i2cp_destination_new();
  sb = i2cp_destination_b64(db);

  /* get destination into stream */
  i2cp_destination_get_message(db, &stream);
  stream_seek_set(&stream, 0);

  /* create new destination from base64 */
  da = i2cp_destination_new_from_base64(sb);
  if (da == NULL)
    fatal(TAG, "%s", "Failed to create destination from base64.");
  sa = i2cp_destination_b64(da);

  /* does the b64 address of before and after differ ? */
  if (strcmp(sb, sa) != 0)
    fatal(TAG, "%s != %s", sb, sa);

  i2cp_destination_destroy(da);
  i2cp_destination_destroy(db);

  stream_destroy(&stream);
  return 1;
}

int main(int argc, char **argv)
{
  /* test creating random destinations */
  if (_test_random_destination() == 0)
    fatal(TAG, "%s", "Failed to randomize destinations.");

  /* verify creating destination from stream */
  if (_test_destination_from_stream() == 0)
    fatal(TAG, "%s", "Failed to create destination from stream.");

  /* verify creating destination from stream */
  if (_test_destination_from_base64() == 0)
    fatal(TAG, "%s", "Failed to create destination from stream.");

  return 0;
}
