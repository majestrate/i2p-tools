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


#include <memory.h>
#include <inttypes.h>
#include <sys/time.h>

#include <i2cp/i2cp.h>


#define TAG SESSION_CONFIG

static const char *_i2cp_session_option[NR_OF_SESSION_CONFIG_PROPERTIES] =
{
  "crypto.lowTagThreshold",
  "crypto.tagsToSend",

  "i2cp.dontPublishLeaseSet",
  "i2cp.fastReceive",
  "i2cp.gzip",
  "i2cp.messageReliability",
  "i2cp.password",
  "i2cp.username",

  "inbound.allowZeroHop",
  "inbound.backupQuantity",
  "inbound.IPRestriction",
  "inbound.length",
  "inbound.lengthVariance",
  "inbound.nickname",
  "inbound.quantity",

  "outbound.allowZeroHop",
  "outbound.backupQuantity",
  "outbound.IPRestriction",
  "outbound.length",
  "outbound.lengthVariance",
  "outbound.nickname",
  "outbound.priority",
  "outbound.quantity",

};

typedef struct i2cp_session_config_t
{
  const char *properties[NR_OF_SESSION_CONFIG_PROPERTIES];
  uint64_t date;
  struct i2cp_destination_t *destination;

} i2cp_session_config_t;

const char *
_session_config_option_lookup(i2cp_session_config_t *self, i2cp_session_config_property_t prop)
{
  return _i2cp_session_option[prop];
}

void
_session_config_mapping_get_message(i2cp_session_config_t *self, stream_t *stream)
{
  int i, cnt;
  stream_t is;
  const char *option;

  stream_init(&is, 0xffff);

  cnt = 0;
  for (i = 0; i < NR_OF_SESSION_CONFIG_PROPERTIES; i++)
  {
    /* if no value is assigned to option, skip to next */
    if (self->properties[i] == NULL)
      continue;
    
    /* get option string from property */
    option = _session_config_option_lookup(self, i);
    if (option == NULL)
      continue;

    /* for now its safe to use strlen() dues to options are pure ACSII
       which is 1byte per character. utf8 is backwards compatible with ASCII.
    */
    stream_out_string(&is, option, strlen(option));
    stream_out_uint8(&is, '=');
    stream_out_string(&is, self->properties[i], strlen(self->properties[i]));
    stream_out_uint8(&is, ';');

    cnt++;
  }
  stream_mark_end(&is);

  debug(TAG, "writing %d options to mapping table.", cnt);

  /* write mapping table to output stream */
  stream_out_uint16(stream, stream_length(&is));
  if (stream_length(&is))
    stream_out_uint8p(stream, is.data, stream_length(&is));

  free(is.data);
}

static void
_session_config_file_parse_callback(const char *name, const char *value, void *opaque)
{
  i2cp_session_config_t *self;
  self = (i2cp_session_config_t *)opaque;

  if (strcmp(name, "i2cp.username") == 0)
    self->properties[SESSION_CONFIG_PROP_I2CP_USERNAME] = strdup(value);
  else if (strcmp(name, "i2cp.password") == 0)
    self->properties[SESSION_CONFIG_PROP_I2CP_PASSWORD] = strdup(value);
}

i2cp_session_config_t *i2cp_session_config_new(const char *dest_fname)
{
  char *home;
  char cfgfile[4096];
  stream_t file;
  struct config_file_t *cfg;
  i2cp_session_config_t *config;

  config = malloc(sizeof(i2cp_session_config_t));
  memset(config, 0, sizeof(i2cp_session_config_t));

  /* if destination file specified, try load it */
  if (dest_fname != NULL)
    config->destination = i2cp_destination_new_from_file(dest_fname);

  if (dest_fname != NULL && config->destination == NULL)
    warning(TAG, "Failed to load destination from file '%s', a new destination will be generated.",
	    dest_fname);

  /* otherwise just generate a new */
  if (config->destination == NULL)
    config->destination = i2cp_destination_new();

  /* save destination to file for future use */
  if (dest_fname != NULL)
    i2cp_destination_save(config->destination, dest_fname);

  /* load options from user configuration */
  home = getenv("HOME");
   if (home == NULL)
    return config;

  sprintf(cfgfile,"%s/.i2cp.conf", home);
  cfg = config_file_new(cfgfile);
  if (cfg == NULL)
    return config;

  /* parse config file*/
  config_file_parse(cfg, _session_config_file_parse_callback, config);

  config_file_destroy(cfg);

  return config;
}

void i2cp_session_config_destroy(struct i2cp_session_config_t *self)
{
  free(self);
}

void i2cp_session_config_set_property(struct i2cp_session_config_t *self,
				      i2cp_session_config_property_t prop,
				      const char *value)
{
  self->properties[prop] = value;
}

void i2cp_session_config_get_message(struct i2cp_session_config_t *self, stream_t *stream)
{
  struct timeval tp;

  /* get session destination into stream */
  i2cp_destination_get_message(self->destination, stream);

  /* get option mapping into stream */
  _session_config_mapping_get_message(self, stream);

  /* get current date into stream */
  gettimeofday(&tp, NULL);
  stream_out_uint64(stream, (tp.tv_sec * 1000 + tp.tv_usec / 1000));
  stream_mark_end(stream);

  /* sign stream content and append digest to stream*/
  i2cp_crypto_sign_stream(i2cp_crypto_instance(), i2cp_destination_signature_keypair(self->destination), stream);

}

struct i2cp_destination_t *
i2cp_session_config_get_destination(struct i2cp_session_config_t *self)
{
  return self->destination;
}
