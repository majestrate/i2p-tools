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
#include <stdarg.h>

#include <i2cp/logger.h>

typedef struct i2cp_logger_priv_t
{
  i2cp_logger_tags_t filter;
} i2cp_logger_priv_t;

i2cp_logger_t _logger;

void _i2cp_logger_log(i2cp_logger_t *self, i2cp_logger_tags_t tags, const char *format, ...)
{
  va_list args;
  char buf[4096];

  /* filter messages */
  if ((tags & (self->priv->filter & LEVEL_MASK)) == 0)
    return;

  /* create message to be logged */
  va_start(args, format);
  vsnprintf(buf, 4096, format, args);
  va_end(args);

  if (self->callbacks && self->callbacks->on_log)
    self->callbacks->on_log(self, tags, buf, self->callbacks->opaque);
  else
    fprintf(stderr, "%d, %s\n",tags, buf);

  /* if fatal log message, abort */
  if (tags & FATAL)
    abort();
}

void i2cp_logger_init(i2cp_logger_callbacks_t *cb)
{
  /* setup logger */
  _logger.priv = malloc(sizeof(i2cp_logger_priv_t));
  _logger.priv->filter = (ALL&TAG_MASK)|INFO|WARNING|ERROR|FATAL|(ALL&COMPONENT_MASK);
  _logger.callbacks = cb;
  _logger.log = _i2cp_logger_log;
}

i2cp_logger_t *i2cp_logger_instance()
{
  if (_logger.priv == NULL)
  {
    i2cp_logger_init(NULL);
  }

  return &_logger;
}

void i2cp_logger_set_filter(i2cp_logger_t *self, i2cp_logger_tags_t tags)
{
  self->priv->filter = tags;
}
