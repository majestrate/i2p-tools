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

#ifndef _session_config_h
#define _session_config_h

#include "tcp.h"
struct i2cp_session_config_t;

/** \brief Properties of a session config.
 *  For information about each option see; http://www.i2p2.de/i2cp.html
 */
typedef enum i2cp_session_config_property_t
{
  SESSION_CONFIG_PROP_CRYPTO_LOW_TAG_THRESHOLD = 0,
  SESSION_CONFIG_PROP_CRYPTO_TAGS_TO_SEND,
  
  SESSION_CONFIG_PROP_I2CP_DONT_PUBLISH_LEASE_SET,
  SESSION_CONFIG_PROP_I2CP_FAST_RECEIVE,
  SESSION_CONFIG_PROP_I2CP_GZIP,
  SESSION_CONFIG_PROP_I2CP_MESSAGE_RELIABILITY,
  SESSION_CONFIG_PROP_I2CP_PASSWORD,
  SESSION_CONFIG_PROP_I2CP_USERNAME,

  SESSION_CONFIG_PROP_INBOUND_ALLOW_ZERO_HOP,
  SESSION_CONFIG_PROP_INBOUND_BACKUP_QUANTITY,
  SESSION_CONFIG_PROP_INBOUND_IP_RESTRICTION,
  SESSION_CONFIG_PROP_INBOUND_LENGTH,
  SESSION_CONFIG_PROP_INBOUND_LENGTH_VARIANCE,
  SESSION_CONFIG_PROP_INBOUND_NICKNAME,
  SESSION_CONFIG_PROP_INBOUND_QUANTITY,

  SESSION_CONFIG_PROP_OUTBOUND_ALLOW_ZERO_HOP,
  SESSION_CONFIG_PROP_OUTBOUND_BACKUP_QUANTITY,
  SESSION_CONFIG_PROP_OUTBOUND_IP_RESTRICTION,
  SESSION_CONFIG_PROP_OUTBOUND_LENGTH,
  SESSION_CONFIG_PROP_OUTBOUND_LENGTH_VARIANCE,
  SESSION_CONFIG_PROP_OUTBOUND_NICKNAME,
  SESSION_CONFIG_PROP_OUTBOUND_PRIORITY,
  SESSION_CONFIG_PROP_OUTBOUND_QUANTITY,

  NR_OF_SESSION_CONFIG_PROPERTIES
} i2cp_session_config_property_t;

/** \brief Constructs a session config instance.
 *  \param[in] dest_fname A filename to load destination for a file, NULL if a
 *              new destination should be generated.
 *  \return A new session config instance.
 */
struct i2cp_session_config_t *i2cp_session_config_new(const char *dest_fname);

/** \brief Destroys a session config instance. */
void i2cp_session_config_destroy(struct i2cp_session_config_t *self);

/** \brief Set a property of the session config instance.
 *  If a property is not set, the default value for the specific property is used.
 *  Default values are internals of the i2p router.
 *  \see i2cp_session_config_property_t
 */
void i2cp_session_config_set_property(struct i2cp_session_config_t *self,
				      i2cp_session_config_property_t prop,
				      const char *value);

/** \brief Writes protocol message of the session config instance.
 *  \param[in] self The instance of session config
 *  \param[out] stream The output stream for the session config data structure. 
 */
void i2cp_session_config_get_message(struct i2cp_session_config_t *self, stream_t *stream);

struct i2cp_destination_t *i2cp_session_config_get_destination(struct i2cp_session_config_t *self);
#endif
