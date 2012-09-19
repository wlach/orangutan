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

static int
id_matches(const struct input_id *id,
           __u16 bustype, __u16 vendor,
           __u16 product, __u16 version)
{
  return (id->bustype == bustype) &&
         (id->vendor == vendor) &&
         (id->product == product) &&
         (id->version == version);
}

const struct orng_device_info *
orng_find_device(__u16 bustype, __u16 vendor, __u16 product, __u16 version)
{
  static const struct orng_device_info devinfo[] = {
    /* All device information are in a separate file. */
#include "devspec.h"
    /* The final entry of this array is the default. */
    {
      .id = {
        .bustype = 0,
        .vendor = 0,
        .product = 0,
        .version = 0
      },
      .name = "Generic touch screen",
      .evbit = {
        [BIT_WORD(EV_ABS)] = BIT_MASK(EV_ABS)
      },
      .absbit = {
        [0] = (ABS_X%BITS_PER_BYTE)|(ABS_Y%BITS_PER_BYTE),
        [5] = (ABS_MT_SLOT%BITS_PER_BYTE),
        [6] = (ABS_MT_POSITION_X%BITS_PER_BYTE)|(ABS_MT_POSITION_Y%BITS_PER_BYTE),
      },
      .absinfo = {
        [ABS_X] = {
          .value = 0,
          .minimum = SHRT_MIN,
          .maximum = SHRT_MAX,
          .fuzz = 0,
          .flat = 0,
          .resolution = 1
        },
        [ABS_Y] = {
          .value = 0,
          .minimum = SHRT_MIN,
          .maximum = SHRT_MAX,
          .fuzz = 0,
          .flat = 0,
          .resolution = 1
        }
      }
    }
  };

  size_t i;

  for (i = 0; i < ARRAY_SIZE(devinfo); ++i) {
    if (id_matches(&devinfo[i].id, bustype, vendor, product, version)) {
      return devinfo+i;
    }
  }

  return devinfo+ARRAY_SIZE(devinfo)-1;
}
