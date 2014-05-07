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

#include <i2cp/lease.h>

/** \brief Defines the authorization for a particular tunnel to
    receive message targeting a destination.
*/
typedef struct i2cp_lease_t {
  /* sha256 of the RouterIdentity of the tunnel gateway */
  uint8_t tunnel_gw[32];
  /* tunnel id */
  uint32_t tunnel_id;
  /* end date */
  uint64_t end_date;
} i2cp_lease_t;

i2cp_lease_t *
i2cp_lease_new_from_stream(stream_t *stream)
{
  i2cp_lease_t *lease;
  lease = malloc(sizeof(i2cp_lease_t));
  memset(lease, 0, sizeof(i2cp_lease_t));

  stream_in_uint8p(stream, lease->tunnel_gw, 32);
  stream_in_uint32(stream, lease->tunnel_id);
  stream_in_uint64(stream, lease->end_date);

  return lease;
}

void
i2cp_lease_destroy(i2cp_lease_t *lease)
{
  free(lease);
}

void
i2cp_lease_get_message(i2cp_lease_t *lease, stream_t *stream)
{
  /* write lease into stream */
  stream_out_uint8p(stream, lease->tunnel_gw, 32);
  stream_out_uint32(stream, lease->tunnel_id);
  stream_out_uint64(stream, lease->end_date);

  /* mark end of stream */
  stream_mark_end(stream);
}
