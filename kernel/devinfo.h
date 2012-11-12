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

#ifdef __KERNEL__
#include <linux/input.h>
#else

#include "../linux_input.h"

#define BITS_PER_BYTE CHAR_BIT

#endif

#define NBYTES_OF_NBITS(bits) \
  (((bits)/BITS_PER_BYTE) + !!((bits)%BITS_PER_BYTE))

#define NBITS_OF_NBYTES(bytes) \
  ((bytes)*BITS_PER_BYTE)

#define NBITS_IN_ARRAY(array) \
  NBITS_OF_NBYTES(sizeof(array)*BITS_PER_BYTE)

#define TEST_ARRAY_BIT(array, bit) \
  ((array)[(bit)/BITS_PER_BYTE] & (1ul<<((bit)%BITS_PER_BYTE)))

struct orng_device_info {
  struct input_id id;
  char *cname; /* canonical name with device name and s/ /_/g */
  char *name;
  char *phys;
  char *uniq;

  __u8 evbit[NBYTES_OF_NBITS(EV_MAX+1)];
  __u8 keybit[NBYTES_OF_NBITS(KEY_MAX+1)];
  __u8 relbit[NBYTES_OF_NBITS(REL_MAX+1)];
  __u8 absbit[NBYTES_OF_NBITS(ABS_MAX+1)];
  __u8 mscbit[NBYTES_OF_NBITS(MSC_MAX+1)];
  __u8 ledbit[NBYTES_OF_NBITS(LED_MAX+1)];
  __u8 sndbit[NBYTES_OF_NBITS(SND_MAX+1)];
  __u8 ffbit[NBYTES_OF_NBITS(FF_MAX+1)];
  __u8 swbit[NBYTES_OF_NBITS(SW_MAX+1)];

  __u32 rep[REP_MAX+1];

  __u8 key[NBYTES_OF_NBITS(KEY_MAX+1)];
  __u8 led[NBYTES_OF_NBITS(LED_MAX+1)];
  __u8 snd[NBYTES_OF_NBITS(SND_MAX+1)];
  __u8 sw[NBYTES_OF_NBITS(SW_MAX+1)];

  __u32 keymap[KEY_MAX+1][2];

  struct input_absinfo absinfo[ABS_MAX+1];
};

#ifdef __KERNEL__

const struct orng_device_info *
orng_find_device_by_cname(const char *cname);

const struct orng_device_info *
orng_find_device_by_name(const char *name);

const struct orng_device_info *
orng_find_device_by_id(__u16 bustype, __u16 vendor,
                       __u16 product, __u16 version);

#endif
