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

#include <ctype.h>
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
      if (TEST_ARRAY_BIT(devinfo->absbit, i)) {
        if (ioctl(fd, EVIOCGABS(i), devinfo->absinfo+i) < 0) {
          fprintf(stderr, "ioctl(EVIOCGABS(%d)): %s\n", i, strerror(errno));
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
            (unsigned long)sizeof(buf), strerror(errno));
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
            (unsigned long)sizeof(buf), strerror(errno));
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
            (unsigned long)sizeof(buf), strerror(errno));
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

static void
write_u8_buf(const char *name, const __u8 *buf, size_t len, FILE *f)
{
  fprintf(f, ",\n"
             "\t\t.%s = {", name);

  size_t i, j;
  int firstcol;
  int firstrow = 1;

  for (i = 0; i < len; ++i) {
    if (firstrow) {
      firstrow = 0;
    } else {
      fprintf(f, ",");
    }

    fprintf(f, "\n\t\t\t");

    firstcol = 1;

    for (j = 0; (j < 8) && (i < len); ++i, ++j) {
      if (firstcol) {
        firstcol = 0;
      } else {
        fprintf(f, ", ");
      }
      fprintf(f, "0x%.2x", buf[i]);
    }
  }

  fprintf(f, "\n\t\t}");
}

static void
write_u32_buf(const char *name, const __u32 *buf, size_t len, FILE *f)
{
  fprintf(f, ",\n"
             "\t\t.%s = {", name);

  size_t i, j;
  int firstcol;
  int firstrow = 1;

  for (i = 0; i < len; ++i) {
    if (firstrow) {
      firstrow = 0;
    } else {
      fprintf(f, ",");
    }

    fprintf(f, "\n\t\t\t");

    firstcol = 1;

    for (j = 0; (j < 8) && (i < len); ++i, ++j) {
      if (firstcol) {
        firstcol = 0;
      } else {
        fprintf(f, ", ");
      }
      fprintf(f, "0x%.8x", buf[i]);
    }
  }

  fprintf(f, "\n\t\t}");
}

static const struct orng_device_info *
write_devinfo(const char *cname, const struct orng_device_info *devinfo, int with_scancodes, FILE *f)
{
  size_t i;
  int firstrow;

  fprintf(f, "\n/*\n"
             " * Append the output of this program to the file 'devspec.h' in\n"
             " * orangutan's source tree, and rebuild the kernel module.\n"
             " */\n\n");

  fprintf(f, "\t{\n");

  /* device identifier */

  fprintf(f, "\t\t.id = {\n"
             "\t\t\t.bustype = %u,\n"
             "\t\t\t.vendor = %u,\n"
             "\t\t\t.product = %u,\n"
             "\t\t\t.version = %u\n"
             "\t\t}", devinfo->id.bustype, devinfo->id.vendor,
             devinfo->id.product, devinfo->id.version);

  /* canonical and device name */

  if (cname) {
    fprintf(f, ",\n"
               "\t\t.cname = \"%s\"", cname);
  }

  if (devinfo->name) {
    fprintf(f, ",\n"
               "\t\t.name = \"%s\"", devinfo->name);
  }

  /* physical location */

  if (devinfo->phys) {
    fprintf(f, ",\n"
               "\t\t.phys = \"%s\"", devinfo->phys);
  }

  /* unique identifier */

  if (devinfo->uniq) {
    fprintf(f, ",\n"
               "\t\t.uniq = \"%s\"", devinfo->uniq);
  }

  /* feature bits */

  write_u8_buf("evbit", devinfo->evbit,
               sizeof(devinfo->evbit)/sizeof(devinfo->evbit[0]), f);

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
    write_u8_buf("keybit", devinfo->keybit,
                 sizeof(devinfo->keybit)/sizeof(devinfo->keybit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REL)) {
    write_u8_buf("relbit", devinfo->relbit,
                 sizeof(devinfo->relbit)/sizeof(devinfo->relbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_ABS)) {
    write_u8_buf("absbit", devinfo->absbit,
                 sizeof(devinfo->absbit)/sizeof(devinfo->absbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_MSC)) {
    write_u8_buf("mscbit", devinfo->mscbit,
                 sizeof(devinfo->mscbit)/sizeof(devinfo->mscbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_LED)) {
    write_u8_buf("ledbit", devinfo->ledbit,
                 sizeof(devinfo->ledbit)/sizeof(devinfo->ledbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SND)) {
    write_u8_buf("sndbit", devinfo->sndbit,
                 sizeof(devinfo->sndbit)/sizeof(devinfo->sndbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_FF)) {
    write_u8_buf("ffbit", devinfo->keybit,
                 sizeof(devinfo->ffbit)/sizeof(devinfo->ffbit[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SW)) {
    write_u8_buf("swbit", devinfo->swbit,
                 sizeof(devinfo->swbit)/sizeof(devinfo->swbit[0]), f);
  }

  /* auto repeat */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REP)) {
    fprintf(f, ",\n"
               "\t\t.rep = {\n"
               "\t\t\t%ld, %ld\n"
               "\t\t}", (long)devinfo->rep[0], (long)devinfo->rep[1]);
  }

  /* state */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
    write_u8_buf("key", devinfo->key,
                 sizeof(devinfo->key)/sizeof(devinfo->key[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_LED)) {
    write_u8_buf("led", devinfo->led,
                 sizeof(devinfo->led)/sizeof(devinfo->led[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->snd, EV_SND)) {
    write_u8_buf("snd", devinfo->snd,
                 sizeof(devinfo->snd)/sizeof(devinfo->snd[0]), f);
  }

  if (TEST_ARRAY_BIT(devinfo->sw, EV_SW)) {
    write_u8_buf("sw", devinfo->sw,
                 sizeof(devinfo->sw)/sizeof(devinfo->sw[0]), f);
  }

  /* scancodes */

  if (with_scancodes) {
    if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
      write_u32_buf("keymap", devinfo->keymap[0],
                    sizeof(devinfo->keymap)/sizeof(devinfo->keymap[0][0]), f);
    }
  }

  /* abs limits */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_ABS)) {
    fprintf(f, ",\n"
               "\t\t.absinfo = {");
    for (i = 0, firstrow = 1;
         i < sizeof(devinfo->absinfo)/sizeof(devinfo->absinfo[0]); ++i) {
      if (TEST_ARRAY_BIT(devinfo->absbit, i)) {
        if (firstrow) {
          firstrow = 0;
        } else {
          fprintf(f, ",");
        }
        fprintf(f, "\n\t\t\t[%lu] = {\n"
                   "\t\t\t\t.value = %d,\n"
                   "\t\t\t\t.minimum = %d,\n"
                   "\t\t\t\t.maximum = %d,\n"
                   "\t\t\t\t.fuzz = %d,\n"
                   "\t\t\t\t.flat = %d,\n"
                   "\t\t\t\t.resolution = %d\n"
                   "\t\t\t}", (unsigned long)i,
                   devinfo->absinfo[i].value, devinfo->absinfo[i].minimum,
                   devinfo->absinfo[i].maximum, devinfo->absinfo[i].fuzz,
                   devinfo->absinfo[i].flat, devinfo->absinfo[i].resolution);
      }
    }
    fprintf(f, "\n"
               "\t\t}");
  }

  fprintf(f, "\n"
             "\t},\n");

  return devinfo;
}

static char *
cname_of_dev(const char *dev, const char *devname)
{
  char *pos;
  size_t devlen = strlen(dev);
  size_t devnamelen = strlen(devname);

  char *cname = malloc(devlen + sizeof('-') + devnamelen + sizeof('\0'));

  if (!cname) {
    perror("malloc");
    goto err_malloc;
  }

  pos = ((char*)memcpy(cname, dev, devlen)) + devlen;
  *(pos++) = '-';
  pos = ((char*)memcpy(pos, devname, devnamelen)) + devnamelen;
  *(pos++) = '\0';

  while (pos > cname) {
    --pos;
    if (isspace(*pos))
      *pos = '_';
    else
      *pos = tolower(*pos);
  }

  return pos;

err_malloc:
    return NULL;
}

int
main(int argc, char *argv[])
{
  static const char optstring[] =
    "d:i:s" /* device string and input file */;

  const char *dev = NULL;
  const char *in = NULL;
  int with_scancodes = 0;
  int opt;
  int infd;
  struct orng_device_info devinfo;
  char *cname;
  int res;

  while ((opt = getopt(argc, argv, optstring)) != -1) {

    switch (opt) {
      case 'd':
        dev = optarg;
        break;
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

  if (!dev) {
    fprintf(stderr, "No device name given. Specify with '-d <string>'\n");
    goto err_in;
  }

  if (!in) {
    fprintf(stderr, "No input file given.  Specify with '-i <filename>'\n");
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

  cname = cname_of_dev(dev, devinfo.name);

  if (!cname)
    goto err_cname_of_dev;

  if (!write_devinfo(cname, &devinfo, with_scancodes, stdout)) {
    goto err_write_devinfo;
  }

  do {
    res = close(infd);
  } while ((res < 0) && (errno == EINTR));

  if (res < 0) {
    fprintf(stderr, "close: %s\n", strerror(errno));
    /* don't fail */
  }

  free(cname);

  exit(EXIT_SUCCESS);

err_write_devinfo:
  free(cname);
err_cname_of_dev:
err_read_devinfo:
err_open:
err_in:
  exit(EXIT_FAILURE);
}
