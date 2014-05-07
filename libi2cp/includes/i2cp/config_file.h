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

#ifndef _config_file_h
#define _config_file_h

typedef void (config_file_parse_callback_t)(const char *name, const char *value, void *opaque);

struct config_file_t;

struct config_file_t *config_file_new(const char *filename);
void config_file_destroy(struct config_file_t *self);
void config_file_parse(struct config_file_t *self, config_file_parse_callback_t *cb, void *opaque);

#endif
