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

#ifndef _client_h
#define _client_h

#include "logger.h"

struct i2cp_session_t;
struct i2cp_client_t;

/** \brief i2cp client context properties */
typedef enum i2cp_client_property_t
{
  CLIENT_PROP_ROUTER_ADDRESS,
  CLIENT_PROP_ROUTER_PORT,
  CLIENT_PROP_ROUTER_USE_TLS,
  CLIENT_PROP_USERNAME,
  CLIENT_PROP_PASSWORD,
  NR_OF_I2CP_CLIENT_PROPERTIES
} i2cp_client_property_t;

typedef struct i2cp_client_callbacks_t
{
  void *opaque;
  void (*on_disconnect)(struct i2cp_client_t *client, const char *reason, void *opaque);
  void (*on_log)(struct i2cp_client_t *client, i2cp_logger_tags_t tags, const char *message, void *opaque);
} i2cp_client_callbacks_t;

typedef enum i2cp_protocol_t
{
  PROTOCOL_STREAMING = 6,
  PROTOCOL_DATAGRAM = 17,
  PROTOCOL_RAW_DATAGRAM = 18
} i2cp_protocol_t;

/** \brief A i2cp client context.
 * The i2cp client is used to connect to an i2p router
 * and manage sessions.
 * 
 */
struct i2cp_client_t;

/** \brief Constructs a new i2cp client context.
 *  \param[in] callbacks A pointer to initialized i2cp_client_callbacks_t.
 *  \see i2cp_client_t i2cp_client_callback_t
 */
struct i2cp_client_t *i2cp_client_new(i2cp_client_callbacks_t *callbacks);

/** \brief Destroys a i2cp client context. 
 * The conetext is freed and shouldn't be used after it's destroyed.
 */
void i2cp_client_destroy(struct i2cp_client_t *self);

/** \brief Set a property of the i2cp client context. 
 *  \param[in] self
 *  \param[in] property
 *  \param[in] value 
 *  \see i2cp_client_property_t
 */
void i2cp_client_set_property(struct i2cp_client_t *self, i2cp_client_property_t property, const char *value);

/** \brief Get value of a property of the i2cp client context.
    \ses i2cp_client_property_t
 */
const char * i2cp_client_get_property(struct i2cp_client_t *self, i2cp_client_property_t property);

/** \brief Establishes i2cp connection of the i2cp client context.*/
void i2cp_client_connect(struct i2cp_client_t *self);

/** \brief Disconnects the i2cp connection of the i2cp client context.*/
void i2cp_client_disconnect(struct i2cp_client_t *self);

/** \brief Disconnects the i2cp connection of the i2cp client context.*/
int i2cp_client_is_connected(struct i2cp_client_t *self);

/** \brief Creates specified session in the i2cp client context. 
 *  The session should be fully configured before passed to this function.
 *  \see i2cp_session_t i2cp_session_config_t
 */
void i2cp_client_create_session(struct i2cp_client_t *self, struct i2cp_session_t *session);

/** \brief Process i2cp io
 */
int i2cp_client_process_io(struct i2cp_client_t *self);

/** \brief Lookup an i2p address
    \param[in] self
    \param[in] address An i2p address eg. base32 encoded hash with suffix ".b32.i2p"
               or registered hostname
    \param[in] session A session that wants the response of the lookup.
    \return A request id for matching response with lookup.
 */
uint32_t i2cp_client_destination_lookup(struct i2cp_client_t *self,
					struct i2cp_session_t *session, const char *address);

#endif
