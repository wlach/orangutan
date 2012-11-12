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

#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "devinfo.h"

#define ORNG_NDEVICES  16

char *devices[ORNG_NDEVICES];
int   ndevices = ARRAY_SIZE(devices);

char *names[ORNG_NDEVICES];
int   nnames = ARRAY_SIZE(names);

__u16 bustype = 0;
__u16 vendor = 0;
__u16 product = 0;
__u16 version = 0;

module_param_array(devices, charp, &ndevices, S_IRUGO);
module_param_array(names, charp, &nnames, S_IRUGO);

module_param(bustype, ushort, S_IRUGO);
module_param(vendor, ushort, S_IRUGO);
module_param(product, ushort, S_IRUGO);
module_param(version, ushort, S_IRUGO);

static struct input_dev *indev[ORNG_NDEVICES];

static struct input_dev *
orng_setup_device(struct input_dev *indev, const struct orng_device_info *devinfo)
{
  size_t i;

  /* device identifier */

  indev->id.bustype = devinfo->id.bustype;
  indev->id.vendor = devinfo->id.vendor;
  indev->id.product = devinfo->id.product;
  indev->id.version = devinfo->id.version;

  /* device name */

  if (devinfo->name) {
    indev->name = devinfo->name;
  }

  /* physical location */

  if (devinfo->phys) {
    indev->phys = devinfo->phys;
  }

  /* unique identifier */

  if (devinfo->uniq) {
    indev->uniq = devinfo->uniq;
  }

  /* event bits */

  for (i = 0; i < NBITS_IN_ARRAY(indev->evbit); ++i) {
    if (TEST_ARRAY_BIT(devinfo->evbit, i)) {
      indev->evbit[BIT_WORD(i)] |= BIT_MASK(i);
    }
  }

  /* keys */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_KEY)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->keybit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->keybit, i)) {
        indev->keybit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }

    /* key states */

    for (i = 0; i < NBITS_IN_ARRAY(devinfo->key); ++i) {
      if (TEST_ARRAY_BIT(devinfo->key, i)) {
        indev->key[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* relative positioning */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REL)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->relbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->relbit, i)) {
        indev->relbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* absolute positioning */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_ABS)) {
    indev->absinfo =
      kzalloc(sizeof(*indev->absinfo)*NBITS_IN_ARRAY(devinfo->absbit),
              GFP_KERNEL);

    if (!indev->absinfo) {
      printk(KERN_ERR "kzalloc failed");
      goto err_kmalloc;
    }

    for (i = 0; i < NBITS_IN_ARRAY(devinfo->absbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->absbit, i)) {
        indev->absbit[BIT_WORD(i)] |= BIT_MASK(i);

        /* abs limits */
        memcpy(indev->absinfo+i, devinfo->absinfo+i,
               sizeof(devinfo->absinfo[i]));
      }
    }
  }

  /* misc */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_MSC)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->mscbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->mscbit, i)) {
        indev->mscbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* LEDs */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_LED)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->ledbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->ledbit, i)) {
        indev->ledbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }

    /* LED states */

    for (i = 0; i < NBITS_IN_ARRAY(devinfo->led); ++i) {
      if (TEST_ARRAY_BIT(devinfo->led, i)) {
        indev->led[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* sound */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SND)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->sndbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->sndbit, i)) {
        indev->sndbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }

    /* sound states */

    for (i = 0; i < NBITS_IN_ARRAY(devinfo->snd); ++i) {
      if (TEST_ARRAY_BIT(devinfo->snd, i)) {
        indev->snd[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* force feedback */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_FF)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->ffbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->ffbit, i)) {
        indev->ffbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* switches */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_SW)) {
    for (i = 0; i < NBITS_IN_ARRAY(devinfo->swbit); ++i) {
      if (TEST_ARRAY_BIT(devinfo->swbit, i)) {
        indev->swbit[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }

    /* switch states */

    for (i = 0; i < NBITS_IN_ARRAY(devinfo->sw); ++i) {
      if (TEST_ARRAY_BIT(devinfo->sw, i)) {
        indev->sw[BIT_WORD(i)] |= BIT_MASK(i);
      }
    }
  }

  /* auto repeat */

  if (TEST_ARRAY_BIT(devinfo->evbit, EV_REP)) {
    indev->rep[REP_DELAY]  = devinfo->rep[0];
    indev->rep[REP_PERIOD] = devinfo->rep[1];
  }

  return indev;

err_kmalloc:
  return NULL;
}

static struct input_dev *
add_device(const struct orng_device_info *devinfo)
{
  struct input_dev *indev;
  int res;

  indev = input_allocate_device();

  if (!indev) {
    printk(KERN_ERR "input_allocate_device failed");
    goto err_input_allocate_device;
  }

  if (!orng_setup_device(indev, devinfo)) {
    goto err_setup_device;
  }

  res = input_register_device(indev);

  if (res < 0) {
    printk(KERN_ERR "input_register_device failed with error %d", -res);
    goto err_input_register_device;
  }

  return indev;

err_input_register_device:
  kfree(indev->absinfo);
  indev->absinfo = NULL;
err_setup_device:
  input_free_device(indev);
err_input_allocate_device:
  return NULL;
}

static int __init
orng_init(void)
{
  const struct orng_device_info *devinfo;
  int res;
  int i;

  if (devices[0]) {
    for (i = 0; i < ndevices; ++i) {
      devinfo = orng_find_device_by_cname(devices[i]);

      if (!devinfo) {
        res = -EINVAL;
        goto cleanup_devices;
      }

      indev[i] = add_device(devinfo);

      if (!indev[i]) {
        res = -ENOMEM;
        goto cleanup_devices;
      }
    }
  } else if (names[0]) {
    for (i = 0; i < nnames; ++i) {
      devinfo = orng_find_device_by_name(names[i]);

      if (!devinfo) {
        res = -EINVAL;
        goto cleanup_devices;
      }

      indev[i] = add_device(devinfo);

      if (!indev[i]) {
        res = -ENOMEM;
        goto cleanup_devices;
      }
    }
  } else {
    devinfo = orng_find_device_by_id(bustype, vendor, product, version);

    if (!devinfo) {
      res = -EINVAL;
      goto cleanup_devices;
    }

    indev[0] = add_device(devinfo);

    if (!indev[0]) {
      res = -ENOMEM;
      goto cleanup_devices;
    }
  }

  return 0;

cleanup_devices:
  i = ORNG_NDEVICES;
  while (i) {
    --i;
    if (indev[i]) {
      input_unregister_device(indev[i]);
    }
  }
  return res;
}

static void __exit
orng_exit(void)
{
  int i = ORNG_NDEVICES;

  while (i) {
    --i;
    if (indev[i]) {
      input_unregister_device(indev[i]);
    }
  }
  /* don't free indev */
}

module_init(orng_init);
module_exit(orng_exit);

MODULE_LICENSE("GPL and additional rights");
