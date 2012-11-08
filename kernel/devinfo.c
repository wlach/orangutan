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

#include "devinfo.h"

const struct orng_device_info *
orng_find_device(const void *param,
                 int (*cmp)(const struct orng_device_info*, const void*))
{
  static const struct orng_device_info devinfo[] = {
    /* All device information are in a separate file. */
#include "devspec.h"
    {
      .id = {
        .bustype = 0,
        .vendor = 0,
        .product = 0,
        .version = 0
      },
      .cname = "generic-720p_touchscreen",
      .name = "720p touchscreen",
      .evbit = {
        [BIT_WORD(EV_ABS)] = BIT_MASK(EV_ABS)
      },
      .absbit = {
        [6] = (1<<((ABS_MT_TOUCH_MAJOR)%BITS_PER_BYTE)) |
              (1<<((ABS_MT_WIDTH_MAJOR)%BITS_PER_BYTE)) |
              (1<<((ABS_MT_ORIENTATION)%BITS_PER_BYTE)) |
              (1<<((ABS_MT_POSITION_X)%BITS_PER_BYTE)) |
              (1<<((ABS_MT_POSITION_Y)%BITS_PER_BYTE)),
        [7] = (1<<((ABS_MT_TRACKING_ID)%BITS_PER_BYTE)) |
              (1<<((ABS_MT_PRESSURE)%BITS_PER_BYTE))
      },
      .absinfo = {
        [ABS_MT_TOUCH_MAJOR] = {
          .value = 0,
          .minimum = 0,
          .maximum = 255,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
        [ABS_MT_WIDTH_MAJOR] = {
          .value = 0,
          .minimum = 0,
          .maximum = 15,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
        [ABS_MT_ORIENTATION] = {
          .value = 0,
          .minimum = 0,
          .maximum = 0,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
        [ABS_MT_POSITION_X] = {
          .value = 0,
          .minimum = 0,
          .maximum = 719,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
        [ABS_MT_POSITION_Y] = {
          .value = 0,
          .minimum = 0,
          .maximum = 1279,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
        [ABS_MT_TRACKING_ID] = {
          .value = 0,
          .minimum = 0,
          .maximum = 10,
          .fuzz = 0,
          .flat = 0,
          .resolution = 0
        },
			  [ABS_MT_PRESSURE] = {
				  .value = 0,
				  .minimum = 0,
				  .maximum = 255,
				  .fuzz = 0,
				  .flat = 0,
				  .resolution = 0
			  }
      }
    }
  };

  size_t i;

  for (i = 0; i < ARRAY_SIZE(devinfo); ++i) {
    if (!cmp(devinfo+i, param)) {
      return devinfo+i;
    }
  }

  return NULL;
}

static int
cnamecmp_adapter(const struct orng_device_info *devinfo, const void *cname)
{
  return strcmp(devinfo->cname, cname);
}

const struct orng_device_info *
orng_find_device_by_cname(const char *cname)
{
  return orng_find_device(cname, cnamecmp_adapter);
}

static int
namecmp_adapter(const struct orng_device_info *devinfo, const void *name)
{
  return strcmp(devinfo->name, name);
}

const struct orng_device_info *
orng_find_device_by_name(const char *name)
{
  return orng_find_device(name, namecmp_adapter);
}

static long long
lldiff(long long lhs, long long rhs)
{
  return lhs - rhs;
}

static int
llcmp(long long diff)
{
  /*
   * branch-less implementation of
   *
   *  if (diff > 0) {
   *    return 1;
   *  } else if (diff < 0) {
   *    return -1;
   *  } else {
   *    return 0;
   *  }
   */
  return (diff > 0) - (diff < 0);
}

static int
idcmp(const struct input_id *lhs, const struct input_id *rhs)
{
  int cmp = llcmp(lldiff(lhs->bustype, rhs->bustype));

  if (cmp) {
    return cmp;
  }

  cmp = llcmp(lldiff(lhs->vendor, rhs->vendor));

  if (cmp) {
    return cmp;
  }

  cmp = llcmp(lldiff(lhs->product, rhs->product));

  if (cmp) {
    return cmp;
  }

  return llcmp(lldiff(lhs->version, rhs->version));
}

static int
idcmp_adapter(const struct orng_device_info *devinfo, const void *id)
{
  return idcmp(&devinfo->id, id);
}

const struct orng_device_info *
orng_find_device_by_id(__u16 bustype, __u16 vendor, __u16 product, __u16 version)
{
  struct input_id id = {
    .bustype = bustype,
    .vendor = vendor,
    .product = product,
    .version = version
  };

  return orng_find_device(&id, idcmp_adapter);
}
