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

#ifndef _stream_h
#define _stream_h
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <memory.h>
#include "logger.h"

typedef struct stream_t {
  uint8_t *data;
  uint32_t size;
  uint8_t *p;
  uint8_t *end;
} stream_t;

#define stream_init(s, len) {			\
  (s)->data = malloc((len));			\
  memset((s)->data, 0, len);			\
  (s)->size = (len);				\
  stream_reset((s));				\
}

#define stream_destroy(s) { free((s)->data); }

#define stream_size(s)           ((s)->size)
#define stream_length(s)         ((s)->end - (s)->data)
#define stream_reset(s)          ((s)->end = (s)->p = (s)->data)
#define stream_seek_set(s, a)    ((s)->p = (s)->data + a)
#define stream_tell(s)           (((s)->p - (s)->data))
#define stream_mark_end(s)       ((s)->end = (s)->p)
#define stream_eof(s)            ((s)->p < (s)->end ? 0 : 1)

#define stream_debug(s) {						\
    fprintf(stderr, "STREAM: s->data = %p, s->p = %p, s->end = %p, s->size = %d, length = %ld\n", (s)->data, (s)->p, (s)->end, (s)->size, stream_length(s)); \
}

#if 1
static inline void stream_check(stream_t *s, int len) {
  if (s->p + len > s->data + s->size) {
    stream_debug(s);
    error(STREAM|FATAL, "write paste stream end by %d bytes.",
	  (s->p + len) - (s->data + s->size));
  };
}
#else
#define stream_check(s, len) {};
#endif

#define stream_dump(s, fn) {				\
  FILE *of = fopen(fn, "wb");				\
  fwrite((s)->data, stream_length(s), 1, of);		\
  fclose(of);						\
}

#define stream_skip(s, a)   { stream_check(s, a); (s)->p += a; }

#define stream_in_uint8(s,v)     { stream_check(s, 1); v = *((s)->p++); }
#define stream_in_uint8p(s,v,a)  { stream_check(s, a); memcpy(v, (s)->p, a); (s)->p += a; }
#define stream_in_uint16(s,v)    { stream_check(s, 2); v = (uint16_t)*((s)->p++) << 8 | *((s)->p++);}
#define stream_in_uint32(s,v)    { stream_check(s, 4); v = (uint32_t)*((s)->p++) << 24 | (uint32_t)*((s)->p++) << 16 | (uint32_t)*((s)->p++) << 8 | (uint32_t)*((s)->p++) << 0 ;}
#define stream_in_uint64(s,v)    { stream_check(s, 8); v = (uint64_t)*((s)->p++) << 56 | (uint64_t)*((s)->p++) << 48 | (uint64_t)*((s)->p++) << 40 | (uint64_t)*((s)->p++) << 32 | (uint64_t)*((s)->p++) << 24 | (uint64_t)*((s)->p++) << 16 | (uint64_t)*((s)->p++) << 8 | (uint64_t)*((s)->p++) << 0 ;}

#define stream_out_uint8(s,v)     { stream_check(s, 1); *((s)->p++) = v; }
#define stream_out_uint8p(s,v,a)  { stream_check(s, a); memcpy((s)->p, v, a); (s)->p += a; }
#define stream_out_uint16(s,v)    { stream_check(s, 2); *((s)->p++) = (v & 0xff00) >> 8; *((s)->p++) =(v & 0xff); }
#define stream_out_uint32(s,v)    { stream_check(s, 4); *((s)->p++) = (v & 0xff000000) >> 24; *((s)->p++) = (v & 0xff0000) >> 16; *((s)->p++) = (v & 0xff00) >> 8; *((s)->p++) =(v & 0xff); }
#define stream_out_uint64(s,v)    { stream_check(s, 8); *((s)->p++) = (v & 0xff00000000000000) >> 56; *((s)->p++) = (v & 0xff000000000000) >> 48; *((s)->p++) = (v & 0xff0000000000) >> 40; *((s)->p++) = (v & 0xff00000000) >> 32; *((s)->p++) = (v & 0xff000000) >> 24; *((s)->p++) = (v & 0xff0000) >> 16; *((s)->p++) = (v & 0xff00) >> 8; *((s)->p++) = (v & 0xff); }

#define stream_out_string(s,v,a)  {stream_check(s, a+1); *((s)->p++) = a; memcpy((s)->p, v, a); (s)->p += a; }
#define stream_out_stream(s,s2)   {stream_check((s), stream_length((s2))); memcpy((s)->p, (s2)->data, stream_length((s2))); (s)->p += stream_length((s2)); }


static inline void
stream_load(stream_t *s, const char *fn)
{
  int ret;
  FILE *in = fopen(fn, "rb");
  if (in == NULL)
  {
    error(STREAM, "File '%s' not found.", fn);
    return;
  }

  while(!feof(in))
  {
    ret = fread((s)->p, 1, 512, in);
    (s)->p += ret;
  }
  fclose(in);
  stream_mark_end(s);
  stream_seek_set(s, 0);
}

#endif
