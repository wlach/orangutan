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
  int i;
  char buf[1024];
  __u32 sc;
  __u16 j;
  int res = 0;
  __u32 nsc;

  memset(devinfo, 0, sizeof(*devinfo));

  /* device identifier */

  if (ioctl(fd, EVIOCGID, &devinfo->id) < 0) {
    fprintf(stderr, "ioctl(EVIOCGID): %s\n", strerror(errno));
    goto err_ioctl;
  }

  /* event bits */

  if (ioctl(fd, EVIOCGBIT(0, sizeof(devinfo->evbit)), devinfo->evbit) < 0) {
    fprintf(stderr, "ioctl(EVIOCGBIT(0)): %s\n", strerror(errno));
    goto err_ioctl;
  }

  /* keys */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(devinfo->keybit)), devinfo->keybit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_KEY)): %s\n", strerror(errno));
      goto err_ioctl;
    }

    /* key state */

    if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
      if (ioctl(fd, EVIOCGKEY(sizeof(devinfo->key)), devinfo->key) < 0) {
        fprintf(stderr, "ioctl(EVIOCGKEY(%zu)): %s\n",
                sizeof(buf), strerror(errno));
        goto err_ioctl;
      }
    }

    /* read mapping between scan codes and key codes */

    if (with_scancodes) {
      nsc = 1ul<<((CHAR_BIT*sizeof(devinfo->keymap[0][0]))-1);

      for (sc = 0, j = 0; sc < nsc; ++sc) {

        int map[2] = {sc, 0};

        int res = ioctl(fd, EVIOCGKEYCODE, map);

        if (res < 0) {
          if (errno != EINVAL) {
            fprintf(stderr, "ioctl: %s\n", strerror(errno));
            goto err_ioctl;
          }
        } else {
          /* save mapping */

          devinfo->keymap[j][0] = map[0]; /* scan code */
          devinfo->keymap[j][1] = map[1]; /* key code */
          ++j;

          if (j >= sizeof(devinfo->keymap)/sizeof(devinfo->keymap[0])) {
            break;
          }
        }
      }
    }
  }

  /* relative positioning */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REL)) {
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(devinfo->relbit)), devinfo->relbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_REL)): %s\n", strerror(errno));
      goto err_ioctl;
    }
  }

  /* absolute positioning */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_ABS)) {
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(devinfo->absbit)), devinfo->absbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_ABS)): %s\n", strerror(errno));
      goto err_ioctl;
    }

    /* limits */

    for (i = 0; i <= ABS_MAX; ++i) {
      if (devinfo->absbit[i/8] & (i%8)) {
        if (ioctl(fd, EVIOCGABS(i), devinfo->absinfo+i) < 0) {
          fprintf(stderr, "ioctl(EVIOCGABS(%lu)): %s\n", strerror(errno), i);
          goto err_ioctl;
        }
      }
    }
  }

  /* misc */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_MSC)) {
    if (ioctl(fd, EVIOCGBIT(EV_MSC, sizeof(devinfo->mscbit)), devinfo->mscbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_MSC)): %s\n", strerror(errno));
      goto err_ioctl;
    }
  }

  /* LEDs */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_LED)) {
    if (ioctl(fd, EVIOCGBIT(EV_LED, sizeof(devinfo->ledbit)), devinfo->ledbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_LED)): %s\n", strerror(errno));
      goto err_ioctl;
    }

    /* LED state */

    if (TEST_ARRAY_BIT(devinfo->evbit, EV_LED)) {
      if (ioctl(fd, EVIOCGLED(sizeof(devinfo->led)), devinfo->led) < 0) {
        fprintf(stderr, "ioctl(EVIOCGLED(%zu)): %s\n",
                sizeof(buf), strerror(errno));
        goto err_ioctl;
      }
    }
  }

  /* sound */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SND)) {
    if (ioctl(fd, EVIOCGBIT(EV_SND, sizeof(devinfo->sndbit)), devinfo->sndbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_SND)): %s\n", strerror(errno));
      goto err_ioctl;
    }

    /* sound state */

    if (TEST_ARRAY_BIT(devinfo->evbit, EV_SW)) {
      if (ioctl(fd, EVIOCGSND(sizeof(devinfo->snd)), devinfo->snd) < 0) {
        fprintf(stderr, "ioctl(EVIOCGSND(%zu)): %s\n",
                sizeof(buf), strerror(errno));
        goto err_ioctl;
      }
    }
  }

  /* force feedback */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_FF)) {
    if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(devinfo->ffbit)), devinfo->ffbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_FF)): %s\n", strerror(errno));
      goto err_ioctl;
    }
  }

  /* switches */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SW)) {
    if (ioctl(fd, EVIOCGBIT(EV_SW, sizeof(devinfo->swbit)), devinfo->swbit) < 0) {
      fprintf(stderr, "ioctl(EVIOCGBIT(EV_SW)): %s\n", strerror(errno));
      goto err_ioctl;
    }

    /* switch state */

    if (TEST_ARRAY_BIT(devinfo->evbit, EV_SW)) {
      if (ioctl(fd, EVIOCGSW(sizeof(devinfo->sw)), devinfo->sw) < 0) {
        fprintf(stderr, "ioctl(EVIOCGSW(%zu)): %s\n",
                sizeof(buf), strerror(errno));
        goto err_ioctl;
      }
    }
  }

  /* auto repeat */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REP)) {
    if (ioctl(fd, EVIOCGREP, devinfo->rep) < 0) {
      fprintf(stderr, "ioctl(EVIOCGREP): %s\n", strerror(errno));
      goto err_ioctl;
    }
  }

  /* name */

  memset(buf, 0, sizeof(buf));

  do {
    res = ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
  } while ((res < 0) && (errno == EINTR));

  if (res >= 0) {
    devinfo->name = strndup(buf, sizeof(buf)-1);

    if (!devinfo->name) {
      fprintf(stderr, "strdup: %s\n", strerror(errno));
      goto err_strdup_name;
    }
  } else if (errno != ENOENT) {
    fprintf(stderr, "ioctl(EVIOCGPHYS(%lu)): %s\n",
            (unsigned int)sizeof(buf), strerror(errno));
    goto err_ioctl;
  }

  /* physical location */

  memset(buf, 0, sizeof(buf));

  do {
    res = ioctl(fd, EVIOCGPHYS(sizeof(buf)), buf);
  } while ((res < 0) && (errno == EINTR));

  if (res >= 0) {
    devinfo->phys = strndup(buf, sizeof(buf)-1);

    if (!devinfo->phys) {
      fprintf(stderr, "strdup: %s\n", strerror(errno));
      goto err_strdup_phys;
    }
  } else if (errno != ENOENT) {
    fprintf(stderr, "ioctl(EVIOCGPHYS(%lu)): %s\n",
            (unsigned int)sizeof(buf), strerror(errno));
    goto err_ioctl;
  }

  /* unique identifier */

  memset(buf, 0, sizeof(buf));

  do {
    res = ioctl(fd, EVIOCGUNIQ(sizeof(buf)), buf);
  } while ((res < 0) && (errno == EINTR));

  if (res >= 0) {
    devinfo->uniq = strndup(buf, sizeof(buf)-1);

    if (!devinfo->uniq) {
      fprintf(stderr, "strdup: %s\n", strerror(errno));
      goto err_strdup_uniq;
    }
  } else if (errno != ENOENT) {
    fprintf(stderr, "ioctl(EVIOCGUNIQ(%lu)): %s\n",
            (unsigned int)sizeof(buf), strerror(errno));
    goto err_ioctl;
  }

  return devinfo;

err_strdup_uniq:
  free(devinfo->phys);
err_strdup_phys:
err_ioctl_gphys:
  free(devinfo->name);
err_strdup_name:
err_ioctl:
  return NULL;
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
