/*
 * Copyright 2012, Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kernel/devinfo.h"

static struct orng_device_info *
read_devinfo(struct orng_device_info *devinfo, int with_scancodes, int fd)
{
  return devinfo;
}

static const struct orng_device_info *
write_devinfo(const struct orng_device_info *devinfo, int with_scancodes, FILE *f)
{
  return devinfo;
}

int
main(int argc, char *argv[])
{
  static const char optstring[] =
    "i:s" /* input file */;

  const char *in = NULL;
  int with_scancodes = 0;
  int opt;
  int infd;
  struct orng_device_info devinfo;
  int res;

  while ((opt = getopt(argc, argv, optstring)) != -1) {

    switch (opt) {
      case 'i':
        in = optarg;
        break;
      case 's':
        with_scancodes = 1;
        break;
      default:
        break;
    }
  }

  if (!in) {
    fprintf(stderr, "No input file given.\n");
    goto err_in;
  }

  do {
    infd = open(in, O_RDONLY);
  } while ((infd < 0) && (errno == EINTR));

  if (infd < 0) {
    fprintf(stderr, "open: %s\n", strerror(errno));
    goto err_open;
  }

  if (!read_devinfo(&devinfo, with_scancodes, infd)) {
    goto err_read_devinfo;
  }

  if (!write_devinfo(&devinfo, with_scancodes, stdout)) {
    goto err_write_devinfo;
  }

  do {
    res = close(infd);
  } while ((res < 0) && (errno == EINTR));

  if (res < 0) {
    fprintf(stderr, "close: %s\n", strerror(errno));
    /* don't fail */
  }

  exit(EXIT_SUCCESS);

err_write_devinfo:
err_read_devinfo:
err_open:
err_in:
  exit(EXIT_FAILURE);
}
