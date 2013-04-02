/*
** Copyright (C) 2005 The Android Open Source Project
** Copyright 2012, Google, Inc.
** Copyright 2012-2013, Mozilla Foundation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>

#ifdef NDK_BUILD
#include "linux_input.h"
#else
#include <linux/input.h>
#endif

#include <sys/system_properties.h>

#define MAX_COMMAND_ARGS 16
#define MAX_COMMAND_LEN 256

#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

enum {
  /* The input device is a touchscreen or a touchpad (either single-touch or multi-touch). */
  INPUT_DEVICE_CLASS_TOUCH         = 0x00000004,

  /* The input device is a multi-touch touchscreen. */
  INPUT_DEVICE_CLASS_TOUCH_MT      = 0x00000010,

  /* The input device is a multi-touch touchscreen and needs MT_SYNC. */
  INPUT_DEVICE_CLASS_TOUCH_MT_SYNC = 0x00000200

};

static int global_tracking_id = 1;

void write_event(int fd, int type, int code, int value)
{
  struct input_event event;
  memset(&event, 0, sizeof(event));

  event.type = type;
  event.code = code;

  event.value = value;
  usleep(1000);

  ssize_t ret = 0;
  unsigned char *buf = (unsigned char*)&event;
  ssize_t buflen = (ssize_t)sizeof(event);

  do {
    ret = write(fd, buf, buflen);
    if (ret > 0) {
      buf += ret;
      buflen -= ret;
    }
  } while (((ret >= 0) && buflen) || ((ret < 0) && (errno == EINTR)));

  if (ret < 0) {
    fprintf(stderr, "write event failed, %s\n", strerror(errno));
    return;
  }
}

void execute_sleep(int duration_msec)
{
  usleep(duration_msec*1000);
}

void add_mt_tracking_id(int fd, uint32_t device_flags, int id)
{
  write_event(fd, EV_ABS, ABS_MT_TRACKING_ID, id);
}

void change_mt_slot(int fd, uint32_t device_flags, int slot)
{
  write_event(fd, EV_ABS, ABS_MT_SLOT, slot);
}

void remove_mt_tracking_id(int fd, uint32_t device_flags, int slot)
{
  write_event(fd, EV_ABS, ABS_MT_SLOT, slot);
  write_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
  write_event(fd, EV_SYN, SYN_REPORT, 0);
}

void execute_press(int fd, uint32_t device_flags, int x, int y)
{
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 32);
    write_event(fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 4);
    write_event(fd, EV_ABS, ABS_MT_PRESSURE, 90);
    write_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    write_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_ABS, ABS_X, x);
    write_event(fd, EV_ABS, ABS_Y, y);
    write_event(fd, EV_KEY, BTN_TOUCH, 1);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
}

void execute_move(int fd, uint32_t device_flags, int x, int y)
{
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 32);
    write_event(fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 4);
    write_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    write_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    write_event(fd, EV_ABS, ABS_MT_PRESSURE, 90);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_ABS, ABS_X, x);
    write_event(fd, EV_ABS, ABS_Y, y);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
}

void execute_move_unsynced(int fd, uint32_t device_flags,
                           int x, int y)
{
  write_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
  write_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
}

void execute_release(int fd, uint32_t device_flags)
{
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_PRESSURE,0);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_KEY, BTN_TOUCH, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
}

void execute_drag(int fd, uint32_t device_flags, int start_x,
                  int start_y, int end_x, int end_y, int num_steps,
                  int duration_msec)
{
  int delta[] = {(end_x-start_x)/num_steps, (end_y-start_y)/num_steps};
  int sleeptime = duration_msec / num_steps;
  int i;

  // press
  execute_press(fd, device_flags, start_x, start_y);

  // drag
  for (i=0; i<num_steps; i++) {
    execute_sleep(sleeptime);
    execute_move(fd, device_flags, start_x+delta[0]*i, start_y+delta[1]*i);
  }

  // release
  execute_release(fd, device_flags);

  // wait
  execute_sleep(100);
}

void execute_tap(int fd, uint32_t device_flags, int x, int y,
                 int num_times, int duration_msec)
{
  int i;

  for (i=0; i<num_times; i++) {
    // press
    execute_press(fd, device_flags, x, y);
    execute_sleep(duration_msec);

    // release
    execute_release(fd, device_flags);
    execute_sleep(100);

    // wait
    execute_sleep(50);
  }
}

void execute_pinch(int fd, uint32_t device_flags, int touch1_x1,
                   int touch1_y1, int touch1_x2, int touch1_y2, int touch2_x1,
                   int touch2_y1, int touch2_x2, int touch2_y2, int num_steps,
                   int duration_msec)
{
  int delta1[] = {(touch1_x2-touch1_x1)/num_steps, (touch1_y2-touch1_y1)/num_steps};
  int delta2[] = {(touch2_x2-touch2_x1)/num_steps, (touch2_y2-touch2_y1)/num_steps};
  int sleeptime = duration_msec / num_steps;
  int i;

  // press
  change_mt_slot(fd, device_flags, 0);
  add_mt_tracking_id(fd, device_flags, global_tracking_id++);
  execute_press(fd, device_flags, touch1_x1, touch1_y1);

  change_mt_slot(fd, device_flags, 1);
  add_mt_tracking_id(fd, device_flags, global_tracking_id++);
  execute_press(fd, device_flags, touch2_x1, touch2_y1);

  // drag
  for (i=0; i<num_steps; i++) {
    execute_sleep(sleeptime);

    change_mt_slot(fd, device_flags, 0);
    execute_move(fd, device_flags, touch1_x1+delta1[0]*i, touch1_y1+delta1[1]*i);

    change_mt_slot(fd, device_flags, 1);
    execute_move(fd, device_flags, touch2_x1+delta2[0]*i, touch2_y1+delta2[1]*i);

    //write_event(fd, EV_SYN, SYN_REPORT, 0);
  }

  // release
  change_mt_slot(fd, device_flags, 0);
  execute_release(fd, device_flags);

  change_mt_slot(fd, device_flags, 1);
  execute_release(fd, device_flags);

  remove_mt_tracking_id(fd, device_flags, 0);
  remove_mt_tracking_id(fd, device_flags, 1);

  // wait
  execute_sleep(100);

}

uint32_t figure_out_events_device_reports(int fd) {

  uint32_t device_classes = 0;

  uint8_t key_bitmask[(KEY_MAX + 1) / 8 + !!((KEY_MAX + 1) % 8)];
  uint8_t abs_bitmask[(ABS_MAX + 1) / 8 + !!((ABS_MAX + 1) % 8)];

  memset(key_bitmask, 0, sizeof(key_bitmask));
  memset(abs_bitmask, 0, sizeof(abs_bitmask));

  ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask);
  ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask);

  // See if this is a touch pad.
  // Is this a new modern multi-touch driver?
  if (test_bit(ABS_MT_POSITION_X, abs_bitmask)
          && test_bit(ABS_MT_POSITION_Y, abs_bitmask)) {
    // Some joysticks such as the PS3 controller report axes that conflict
    // with the ABS_MT range.  Try to confirm that the device really is
    // a touch screen.
    // Mozilla Bug 741038 - support GB touchscreen drivers
    //if (test_bit(BTN_TOUCH, device->keyBitmask) || !haveGamepadButtons) {
    device_classes |= INPUT_DEVICE_CLASS_TOUCH | INPUT_DEVICE_CLASS_TOUCH_MT;
    char device_name[80];

    if(ioctl(fd, EVIOCGNAME(sizeof(device_name) - 1), &device_name) < 1) {
      //fprintf(stderr, "could not get device name for %s, %s\n", device, strerror(errno));
      device_name[0] = '\0';
    }

    // the atmel touchscreen has a weird protocol which requires MT_SYN events
    // to be sent after every touch
    if(strcmp(device_name, "atmel-touchscreen") == 0) {
      device_classes |= INPUT_DEVICE_CLASS_TOUCH_MT_SYNC;
    }
    //}
  // Is this an old style single-touch driver?
  } else if (test_bit(BTN_TOUCH, key_bitmask)
          && test_bit(ABS_X, abs_bitmask)
          && test_bit(ABS_Y, abs_bitmask)) {
      device_classes |= INPUT_DEVICE_CLASS_TOUCH;
  }

  return device_classes;

}

int main(int argc, char *argv[])
{
  int i;
  int fd;
  int ret;

  int num_args = 0;
  int args[MAX_COMMAND_ARGS];
  char *line, *cmd, *arg;

  if(argc != 3) {
    fprintf(stderr, "Usage: %s <device> <script file>\n", argv[0]);
    return 1;
  }

  fd = open(argv[1], O_RDWR);
  if(fd < 0) {
    fprintf(stderr, "could not open %s, %s\n", argv[optind], strerror(errno));
    return 1;
  }

  uint32_t device_flags = figure_out_events_device_reports(fd);

  FILE *f = fopen(argv[2], "r");
  if (!f) {
    printf("Unable to read file %s", argv[1]);
    return 1;
  }

  line = malloc(sizeof(char)*MAX_COMMAND_LEN);
  while (fgets(line, MAX_COMMAND_LEN, f) != NULL) {
    // Support comments in orangutan scripts with lines starting with "#".
    if (strlen(line) > 0 && line[0] == "#")
      continue;

    cmd = strtok(line, " ");

    num_args = 0;
    while ((arg = strtok(NULL, " \n;")) != NULL) {
      assert(num_args < MAX_COMMAND_ARGS);

      args[num_args] = atoi(arg);
      num_args++;
    }

    if (strcmp(cmd, "tap") == 0) {
      assert(num_args == 4);
      execute_tap(fd, device_flags, args[0], args[1], args[2], args[3]);
    } else if (strcmp(cmd, "drag") == 0) {
      assert(num_args == 6);
      execute_drag(fd, device_flags, args[0], args[1], args[2],
                   args[3], args[4], args[5]);
    } else if (strcmp(cmd, "sleep") == 0) {
      assert(num_args == 1);
      execute_sleep(args[0]);
    } else if (strcmp(cmd, "pinch") == 0) {
      assert(num_args == 10);
      execute_pinch(fd, device_flags, args[0], args[1], args[2],
                    args[3], args[4], args[5], args[6], args[7], args[8],
                    args[9]);
    } else {
      printf("Unrecognized command: '%s'", cmd);
      return 1;
    }
  }
  free(line);

  return 0;
}
