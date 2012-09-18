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
#include <linux/module.h>

__u16 bustype = 0;
__u16 vendor = 0;
__u16 product = 0;
__u16 version = 0;

module_param(bustype, ushort, S_IRUGO);
module_param(vendor, ushort, S_IRUGO);
module_param(product, ushort, S_IRUGO);
module_param(version, ushort, S_IRUGO);

static int __init
orng_init(void)
{
  return 0;
}

static void __exit
orng_exit(void)
{
  return;
}

module_init(orng_init);
module_exit(orng_exit);

MODULE_LICENSE("GPL and additional rights");
