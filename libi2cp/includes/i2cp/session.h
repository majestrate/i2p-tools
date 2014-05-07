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


#ifndef _session_h
#define _session_h

#include <i2cp/client.h>
#include <i2cp/stream.h>

struct i2cp_session_t;
struct i2cp_session_config_t;
struct i2cp_destination_t;

/* \brief message status codes
   \see http://www.i2p2.de/i2cp_spec.html#msg_SendMessage for reference
 */
typedef enum i2cp_session_message_status_t {
  I2CP_MSG_STATUS_AVAILABLE = 0,
  I2CP_MSG_STATUS_ACCEPTED,
  I2CP_MSG_STATUS_BEST_EFFORT_SUCCESS,
  I2CP_MSG_STATUS_BEST_EFFORT_FAILURE,
  I2CP_MSG_STATUS_GUARANTEED_SUCCESS,
  I2CP_MSG_STATUS_GUARANTEED_FAILURE,
  I2CP_MSG_STATUS_LOCAL_SUCCESS,
  I2CP_MSG_STATUS_LOCAL_FAILURE,
  I2CP_MSG_STATUS_ROUTER_FAILURE,
  I2CP_MSG_STATUS_NETWORK_FAILURE,
  I2CP_MSG_STATUS_BAD_SESSION,
  I2CP_MSG_STATUS_BAD_MESSAGE,
  I2CP_MSG_STATUS_OVERFLOW_FAILURE,
  I2CP_MSG_STATUS_MESSAGE_EXPIRED,
  I2CP_MSG_STATUS_MESSAGE_BAD_LOCAL_LEASESET,
  I2CP_MSG_STATUS_MESSAGE_NO_LOCAL_TUNNELS,
  I2CP_MSG_STATUS_MESSAGE_UNSUPPORTED_ENCRYPTION,
  I2CP_MSG_STATUS_MESSAGE_BAD_DESTINATION,
  I2CP_MSG_STATUS_MESSAGE_BAD_LEASESET,
  I2CP_MSG_STATUS_MESSAGE_EXPIRED_LEASESET,
  I2CP_MSG_STATUS_MESSAGE_NO_LEASESET
} i2cp_session_message_status_t;

/** \brief Session status messages.
    These status codes are sent through the on_status() callback in
    the i2cp_session_callbacks_t which indicates the actual status of
    a session.
*/
typedef enum i2cp_session_status_t {
  I2CP_SESSION_STATUS_DESTROYED,
  I2CP_SESSION_STATUS_CREATED,
  I2CP_SESSION_STATUS_UPDATED,
  I2CP_SESSION_STATUS_INVALID
} i2cp_session_status_t;

/** \brief Session instance callbacks
    A session is created with a set of callbacks to handle different events
    from the underlaying core. Those is required to get a session working and
    actually implements the logic for a session.
*/
typedef struct i2cp_session_callbacks_t {

  /** \brief An opaque data passed along the callbacks. */
  void * opaque;

  /** \brief Incoming message for the session */
  void (*on_message) (struct i2cp_session_t *session,
		      i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
		      stream_t *payload, void *opaque);

  /** \brief Status change for the session.
      \param[in] session The session instance.
      \param[in] status A session status.
      \see i2cp_session_t i2cp_session_status_t
   */
  void (*on_status) (struct i2cp_session_t *session, i2cp_session_status_t status, void *opaque);

  /** \brief Destination lookup reply the session.
      This callback is fired when a destination lookup of a b32 address got a response.
      \param[in] session The session which performed the lookup.
      \param[in] address The b32 address that the session performed a lookup on.
      \param[in] destination A destination of the b32 address, if NULL the lookup failed.
   */
  void (*on_destination) (struct i2cp_session_t *session, uint32_t request_id, const char *address,
			  struct i2cp_destination_t *destination, void *opaque);

} i2cp_session_callbacks_t;

/** \brief Creates a new session object 
    \param[in] client A i2cp_client_t instance which this session belongs to.
    \param[in] cb Configured callback vector for the session object.
    \param[in] dest_fname Filename of stored destination.
    \todo Make session object only instantiable from client factory function
*/
struct i2cp_session_t *i2cp_session_new(struct i2cp_client_t *client,
					struct i2cp_session_callbacks_t *cb,
					const char *dest_fname);

/** \brief Destroys a session object */
void i2cp_session_destroy(struct i2cp_session_t *self);

/** \brief Get session config from session.
    \see i2cp_session_config_t
*/
struct i2cp_session_config_t *i2cp_session_get_config(struct i2cp_session_t *self);

/** \brief Get session id from session.
    \param[in] self
    \return the i2cp session id of the session object.
*/
uint16_t i2cp_session_get_id(struct i2cp_session_t *self);

/** \brief Sends a message to destination.
    \param[in] destination A destination of the message
    \param[in] protocol Specify the protocol to be used
    \param[in] src_port Send message from port
    \param[in] dest_port Send message to port
    \param[in] payload A stream with the payload to send
    \param[in] nonce A nonce of the message
    \see i2cp_destination_t i2cp_protocol_t
*/
void i2cp_session_send_message(struct i2cp_session_t *self,
			       const struct i2cp_destination_t *destination,
			       i2cp_protocol_t protocol, uint16_t src_port, uint16_t dest_port,
			       stream_t *payload, uint32_t nonce);

/** \brief Get the destination of the session. */
const struct i2cp_destination_t *i2cp_session_get_destination(struct i2cp_session_t *self);
#endif
