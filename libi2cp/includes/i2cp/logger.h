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


#ifndef _logging_h
#define _logging_h

#include <inttypes.h>

/* Define logger tags to be used */
#define PROTOCOL     (1 << 0)
#define LOGIC        (1 << 1)

#define DEBUG        (1 << 4)
#define INFO         (1 << 5)
#define WARNING      (1 << 6)
#define ERROR        (1 << 7)
#define FATAL        (1 << 8)

#define STRINGMAP       (1 << 9)
#define INTMAP          (1 << 10)
#define QUEUE           (1 << 11)
#define STREAM          (1 << 12)
#define CRYPTO          (1 << 13)
#define TCP             (1 << 14)
#define CLIENT          (1 << 15)
#define CERTIFICATE     (1 << 16)
#define LEASE           (1 << 17)
#define DESTINATION     (1 << 18)
#define SESSION         (1 << 19)
#define SESSION_CONFIG  (1 << 20)
#define TEST            (1 << 21)
#define DATAGRAM        (1 << 22)
#define CONFIG_FILE     (1 << 23)
#define VERSION         (1 << 24)

#define TAG_MASK        0x0000000f
#define LEVEL_MASK      0x000001f0
#define COMPONENT_MASK  0xfffffe00

#define ALL   0xffffffff

typedef uint32_t i2cp_logger_tags_t;

#define debug(tags, ...) { i2cp_logger_instance()->log(i2cp_logger_instance(), tags|DEBUG, __VA_ARGS__); }
#define info(tags, ...) { i2cp_logger_instance()->log(i2cp_logger_instance(), tags|INFO, __VA_ARGS__); }
#define warning(tags, ...) { i2cp_logger_instance()->log(i2cp_logger_instance(), tags|WARNING, __VA_ARGS__); }
#define error(tags, ...) { i2cp_logger_instance()->log(i2cp_logger_instance(), tags|ERROR, __VA_ARGS__); }
#define fatal(tags, ...) { i2cp_logger_instance()->log(i2cp_logger_instance(), tags|FATAL, __VA_ARGS__); }


struct i2cp_logger_priv_t;

typedef struct i2cp_logger_t
{
  struct i2cp_logger_priv_t *priv;
  struct i2cp_logger_callbacks_t *callbacks;
  void (*log)(struct i2cp_logger_t *self, i2cp_logger_tags_t tags, const char *format, ...);
} i2cp_logger_t;

typedef struct i2cp_logger_callbacks_t
{
  void *opaque;
  void (*on_log)(i2cp_logger_t *self, i2cp_logger_tags_t tags, char *message, void *opaque);
} i2cp_logger_callbacks_t;

/** \brief Initialize logger */
void i2cp_logger_init(i2cp_logger_callbacks_t *cb);

/** \brief Get logger instance */
struct i2cp_logger_t *i2cp_logger_instance();
void i2cp_logger_set_filter(i2cp_logger_t *self, i2cp_logger_tags_t tags);

#endif
