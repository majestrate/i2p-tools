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
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <sys/stat.h>

#include <i2cp/logger.h>
#include <i2cp/config_file.h>

#define TAG CONFIG_FILE

typedef struct config_file_t
{
  const char *filename;
} config_file_t;


static void
_config_file_line_parser(char *line, char **name, char **value)
{
  size_t len;
  char *sep, *ns, *vs;

  len = strlen(line);

  /* find separator */
  sep = line;
  while(*sep != 0 && *sep != '=')
    sep++;

  if (*sep == 0)
    return;

  /* null terminate name string, trim left and find start */
  ns = sep;
  while(ns >= line && (*ns == ' ' || *ns == '='))
  {
    *ns = 0;
    ns--;
  }
  ns = line;
  while(*ns != 0 && *ns == ' ')
    ns++;

  /* null terminate value string, trim right and find start */
  vs = line + len - 1;
  while(vs >= line && (*vs == ' ' || *vs == '\n' || *vs == '\r'))
  {
    *vs = 0;
    vs--;
  }
  vs = sep + 1;
  while(*vs != 0 && *vs == ' ')
    vs++;

  /* setup values */
  *name = ns;
  *value = vs;
}


config_file_t *
config_file_new(const char *filename)
{
  struct stat sb;
  config_file_t *cfg;

  /* verify that file exists */
  if (stat(filename, &sb) != 0)
  {
    warning(TAG, "%s", strerror(errno));
    return NULL;
  }

  /* TODO: verifiy 0600 rights, quit if not... */
  
  cfg = (config_file_t *)malloc(sizeof(config_file_t));
  memset(cfg, 0, sizeof(config_file_t));

  cfg->filename = strdup(filename);

  return cfg;
}

void
config_file_destroy(struct config_file_t *self)
{
  free((char *)self->filename);
  free(self);
}

void
config_file_parse(struct config_file_t *self, config_file_parse_callback_t *cb, void *opaque)
{
  FILE *file;
  char *name, *value;
  char line[4096];

  if (cb == NULL)
    fatal(TAG, "Callback func pointer is NULL.");

  /* open file */
  file = fopen(self->filename, "rt");
  if (file == NULL)
  {
    warning(TAG, "Failed to open file '%s' with reason: %s", strerror(errno));
    return;
  }

  /* parse each line by line */
  while(!feof(file))
  {
    if (fgets(line, 4096, file) == NULL)
      break;

    /* parse name and value out of line */
    name = value = NULL;
    _config_file_line_parser(line, &name, &value);

    /* if didnt parsed the line as a name=vale lets skip it */
    if (name == NULL || value == NULL)
      continue;
    
    /* pass name and value to callback */
    cb(name, value, opaque);
  }

  fclose(file);
}
