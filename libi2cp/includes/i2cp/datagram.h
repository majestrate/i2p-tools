#ifndef _datagram_h
#define _datagram_h

#include "stream.h"

struct i2cp_session_t;
struct i2cp_datagram_t;
struct i2cp_datagram_t *i2cp_datagram_new();
void i2cp_datagram_destroy(struct i2cp_datagram_t *self);

void i2cp_datagram_for_session(struct i2cp_datagram_t *self, struct i2cp_session_t *session, stream_t *payload);
void i2cp_datagram_from_stream(struct i2cp_datagram_t *self, stream_t *message);

/** \brief get the payload of a datagram message.
    This is used for getting the actual payload data from a datagram
    initialized from a stream.
    \param[out] payload
    \see i2cp_datagram_from_stream
*/
void i2cp_datagram_get_payload(struct i2cp_datagram_t *self, stream_t *payload);

/** \brief set the payload of the datagram message.
    \param[in] payload
    \see i2cp_datagram_from_stream
*/
void i2cp_datagram_set_payload(struct i2cp_datagram_t *self, stream_t *payload);

/** \brief get a signed datagram message into stream.
    This is used by a datagram created for a session.
    \see i2cp_datagram_for_session
 */
void i2cp_datagram_get_message(struct i2cp_datagram_t *self, stream_t *out);

const struct i2cp_destination_t *i2cp_datagram_destination(struct i2cp_datagram_t *self);

/** \brief get the payload size of datagram */
size_t i2cp_datagram_payload_size(struct i2cp_datagram_t *self);

#endif
