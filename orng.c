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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
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

enum {
  ACTION_START = 0,
  ACTION_END = 1
};

static int print_actions = 0;
static int action_level = 0;

void print_action(int start_end, const char *action_desc,
                  const char *args_fmt, ...)
{
  const char *start_end_str;
  struct timeval tv;
  int current_arg = 0;
  int i, n;
  int arg;
  va_list ap;
  char *args_str = NULL, args_str2;
  int args_str_size = 128;

  if (!print_actions)
    return;

  gettimeofday(&tv, NULL);

  if (start_end == ACTION_START)
    start_end_str = "START";
  else {
    start_end_str = "END";
    action_level--;
  }

  if (args_fmt) {
    args_str = (char *)malloc(args_str_size);
    assert(args_str);

    va_start(ap, args_fmt);
    n = vsnprintf(args_str, args_str_size, args_fmt, ap);
    va_end(ap);
    if (n == -1 || n >= args_str_size) {
      args_str_size = n + 1;

      args_str = (char *)realloc(args_str, args_str_size);

      va_start(ap, args_fmt);
      vsnprintf(args_str, args_str_size, args_fmt, ap);
      va_end(ap);
    }
    printf("{ \"event\": \"%s\", \"type\": \"%s\", \"level\": %d, "
           "\"time\": %ld.%06ld, \"params\": { %s } }\n", start_end_str,
           action_desc, action_level, tv.tv_sec, tv.tv_usec, args_str);
  } else {
    printf("{ \"event\": \"%s\", \"type\": \"%s\", \"level\": %d, "
           "\"time\": %ld.%06ld }\n", start_end_str, action_desc, action_level,
           tv.tv_sec, tv.tv_usec);
  }

  if (start_end == ACTION_START)
    action_level++;

  free(args_str);
}

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
  print_action(ACTION_START, "sleep", "\"duration\": %d", duration_msec);
  usleep(duration_msec*1000);
  print_action(ACTION_END, "sleep", NULL);
}

void change_mt_slot(int fd, uint32_t device_flags, int slot)
{
  write_event(fd, EV_ABS, ABS_MT_SLOT, slot);
}

void execute_press(int fd, uint32_t device_flags, int x, int y)
{
  print_action(ACTION_START, "press", "\"x\": %d, \"y\": %d", x, y);
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_TRACKING_ID, global_tracking_id++);
    write_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    write_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    write_event(fd, EV_ABS, ABS_MT_PRESSURE, 127);
    write_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 127);
    write_event(fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 4);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_KEY, BTN_TOUCH, 1);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_ABS, ABS_X, x);
    write_event(fd, EV_ABS, ABS_Y, y);
    write_event(fd, EV_KEY, BTN_TOUCH, 1);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
  print_action(ACTION_END, "press", NULL);
}

void execute_move(int fd, uint32_t device_flags, int x, int y)
{
  print_action(ACTION_START, "move", "\"x\": %d, \"y\": %d", x, y);
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    write_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    write_event(fd, EV_ABS, ABS_MT_PRESSURE, 127);
    write_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 127);
    write_event(fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 4);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_ABS, ABS_X, x);
    write_event(fd, EV_ABS, ABS_Y, y);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
  print_action(ACTION_END, "move", NULL);
}

void execute_release(int fd, uint32_t device_flags)
{
  print_action(ACTION_START, "release", NULL);
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC)
      write_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    write_event(fd, EV_KEY, BTN_TOUCH, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_KEY, BTN_TOUCH, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
  }
  print_action(ACTION_END, "release", NULL);
}

double timediff_msec(struct timespec *t1, struct timespec *t2)
{
  return (((t2->tv_sec - t1->tv_sec) * 1000.0) +
          1e-6 * (t2->tv_nsec - t1->tv_nsec));
}

void execute_drag(int fd, uint32_t device_flags, int start_x,
                  int start_y, int end_x, int end_y, int num_steps,
                  int duration_msec)
{
  int delta[] = {(end_x-start_x)/num_steps, (end_y-start_y)/num_steps};
  double desired_interval_msec, avg_event_dispatch_time_msec;
  struct timespec start_time, current_time, time_before_last_move;
  int i;

  clock_gettime(CLOCK_MONOTONIC, &start_time);
  double start_nsecs = (double) (start_time.tv_sec * (1000*1000)) + start_time.tv_nsec;

  print_action(ACTION_START, "drag", "\"start_x\": %d, \"start_y\": %d, "
               "\"end_x\": %d, \"end_y\": %d, \"num_steps\": %d, "
               "\"duration_msec\": %d", start_x, start_y, end_x, end_y,
               num_steps, duration_msec);

  // press
  clock_gettime(CLOCK_MONOTONIC, &time_before_last_move);
  execute_press(fd, device_flags, start_x, start_y);

  // drag
  desired_interval_msec = duration_msec / num_steps;
  for (i=0; i<num_steps; i++) {
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    avg_event_dispatch_time_msec = ((avg_event_dispatch_time_msec * i) +
                                    timediff_msec(&time_before_last_move,
                                                  &current_time)) / i;
    if (desired_interval_msec > 0 &&
        avg_event_dispatch_time_msec < desired_interval_msec) {
      execute_sleep(desired_interval_msec - avg_event_dispatch_time_msec);
    }

    memcpy(&time_before_last_move, &current_time, sizeof(struct timespec));
    execute_move(fd, device_flags, start_x+delta[0]*i, start_y+delta[1]*i);
  }

  // release
  execute_release(fd, device_flags);

  // wait
  execute_sleep(100);

  print_action(ACTION_END, "drag", NULL);
}

void execute_tap(int fd, uint32_t device_flags, int x, int y,
                 int num_times, int duration_msec)
{
  int i;

  print_action(ACTION_START, "tap", "\"x\": %d, \"y\": %d, "
               "\"num_times\": %d, \"duration_msec\": %d", x, y, num_times,
               duration_msec);

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

  print_action(ACTION_END, "tap", NULL);
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

  print_action(ACTION_START, "pinch",
               "\"touch1_x1\": %d, \"touch1_y1\": %d, \"touch1_x2\": %d, "
               "\"touch1_y2\": %d, \"touch2_x1\": %d, \"touch2_y1\": %d, "
               "\"touch2_x2\": %d, \"touch2_y2\": %d, \"num_steps\": %d, "
               "\"duration_msec\": %d",
               touch1_x1, touch1_y1, touch1_x2, touch1_y2,
               touch2_x1, touch2_y1, touch2_x2, touch2_y2,
               num_steps, duration_msec);

  // press
  change_mt_slot(fd, device_flags, 0);
  execute_press(fd, device_flags, touch1_x1, touch1_y1);

  change_mt_slot(fd, device_flags, 1);
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

  // wait
  execute_sleep(100);

  print_action(ACTION_END, "pinch", NULL);
}

void execute_keyup(int fd, int key) {
  write_event(fd, EV_KEY, key, 0);
}

void execute_keydown(int fd, int key) {
  write_event(fd, EV_KEY, key, 1);
}

void execute_reset(int fd, uint32_t device_flags) {
  print_action(ACTION_START, "reset", NULL);
  if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
    write_event(fd, EV_ABS, ABS_MT_POSITION_X, 0);
    write_event(fd, EV_ABS, ABS_MT_POSITION_Y, 0);
    write_event(fd, EV_ABS, ABS_MT_PRESSURE, 0);
    write_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
    write_event(fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 0);
  } else if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
    write_event(fd, EV_ABS, ABS_X, 0);
    write_event(fd, EV_ABS, ABS_Y, 0);
  }
  print_action(ACTION_END, "reset", NULL);
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

    // some touchscreen devices expect MT_SYN events to be sent after every
    // touch
    if(strcmp(device_name, "atmel-touchscreen") == 0 ||
       strcmp(device_name, "nvodm_touch") == 0 ||
       strcmp(device_name, "elan-touchscreen") == 0 ||
       strcmp(device_name, "ft5x06_ts") == 0) {
      device_classes |= INPUT_DEVICE_CLASS_TOUCH_MT_SYNC;
    }

  // Is this an old style single-touch driver?
  } else if ((test_bit(BTN_TOUCH, key_bitmask)
              && test_bit(ABS_X, abs_bitmask)
              && test_bit(ABS_Y, abs_bitmask))) {
    device_classes |= INPUT_DEVICE_CLASS_TOUCH;
  }

  return device_classes;
}

int parseComment(const char *token, int lineCount)
{
  if (*token == '{') {
    char *comment = strstr(token, "}");
    if (comment == NULL)
      comment = strtok(NULL, "}");
    else
      *comment = '\0';
    if (comment == NULL) {
      printf("Missing '}' to end a comment block at line %d.\n", lineCount);
      return -1;
    }
    printf("{}: %s%s%s\n", &token[1], (token[1] && comment[0]) ? " " : "", comment);
    return 1;
  }
  return 0;
}

void checkArguments(const char *cmd, int argc, int expect, int lineCount)
{
  if (argc == expect)
    return;

  printf("At line %d, Command '%s' expect %d arguments, given %d.\n",
         lineCount, cmd, expect, argc);
  assert(0 && "Stop reading the assertion, read the previous message ...");
}

int main(int argc, char *argv[])
{
  int i;
  int fd;
  int ret;
  int c;
  int argcount;
  int print_device_diagnostics = 0;
  const char *device;
  const char *script_file;

  int num_args = 0;
  int args[MAX_COMMAND_ARGS];
  char *line, *cmd, *arg;

  while ((c = getopt (argc, argv, "it")) != -1) {
    if (c=='t') {
      print_actions = 1;
    } else if (c=='i') {
      print_device_diagnostics = 1;
    } else {
      fprintf(stderr, "Unknown option: -%c\n", c);
    }
  }

  argcount = (argc - optind);
  if ((print_device_diagnostics && argcount != 1) ||
      (!print_device_diagnostics && argcount != 2)) {
    fprintf(stderr, "Usage: %s [options] <device> [script file]\n\n"
            "Options:\n"
            "  -i                  print device information\n"
            "  -t                  print event timings\n", argv[0]);
    return 1;
  }
  device = argv[optind];
  script_file = argv[optind + 1];

  fd = open(device, O_RDWR);
  if(fd < 0) {
    fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
    return 1;
  }

  uint32_t device_flags = figure_out_events_device_reports(fd);

  if (print_device_diagnostics) {
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH) {
      printf("INPUT_DEVICE_CLASS_TOUCH\n");
    }
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT) {
      printf("INPUT_DEVICE_CLASS_TOUCH_MT\n");
    }
    if (device_flags & INPUT_DEVICE_CLASS_TOUCH_MT_SYNC) {
      printf("INPUT_DEVICE_CLASS_TOUCH_MT_SYNC\n");
    }

    // just exit
    return 0;
  }

  FILE *f = fopen(script_file, "r");
  if (!f) {
    printf("Unable to read file %s", script_file);
    return 1;
  }

  line = malloc(sizeof(char)*MAX_COMMAND_LEN);
  int lineCount = 0;
  while (fgets(line, MAX_COMMAND_LEN, f) != NULL) {
    // Remove end-of-line comments.
    char *comment = strstr(line, "#");
    if (comment != NULL)
      *comment = '\0';

    lineCount += 1;
    int hasNextCmd = 1;
    char *tempLine = line;
  commandLoop:
    while (hasNextCmd) {
      num_args = 0;
      hasNextCmd = 0;
      int errCode = 0;

      // Parse {-} comments before command names.
      do {
        if ((cmd = strtok(tempLine, " \n")) == NULL)
          goto commandLoop;
        tempLine = NULL;
      } while ((errCode = parseComment(cmd, lineCount)) == 1);
      if (errCode < 0)
        return 1;

      while ((arg = strtok(NULL, " \n")) != NULL) {
        // Parse comment {-} within arguments.
        if ((errCode = parseComment(arg, lineCount)) != 0) {
          if (errCode < 0)
            return 1;
          continue;
        }

        // If we enter a new command, we remember the position for the next iteration.
        if (*arg == ';') {
          hasNextCmd = 1;
          break;
        }

        assert(num_args < MAX_COMMAND_ARGS);

        args[num_args] = atoi(arg);
        num_args++;
      }

      if (strcmp(cmd, "tap") == 0) {
        checkArguments(cmd, num_args, 4, lineCount);
        execute_tap(fd, device_flags, args[0], args[1], args[2], args[3]);
      } else if (strcmp(cmd, "drag") == 0) {
        checkArguments(cmd, num_args, 6, lineCount);
        execute_drag(fd, device_flags, args[0], args[1], args[2],
                     args[3], args[4], args[5]);
      } else if (strcmp(cmd, "sleep") == 0) {
        checkArguments(cmd, num_args, 1, lineCount);
        execute_sleep(args[0]);
      } else if (strcmp(cmd, "pinch") == 0) {
        checkArguments(cmd, num_args, 10, lineCount);
        execute_pinch(fd, device_flags, args[0], args[1], args[2],
                      args[3], args[4], args[5], args[6], args[7], args[8],
                      args[9]);
      } else if (strcmp(cmd, "keyup") == 0) {
        checkArguments(cmd, num_args, 1, lineCount);
        execute_keyup(fd, args[0]);
      } else if (strcmp(cmd, "keydown") == 0) {
        checkArguments(cmd, num_args, 1, lineCount);
        execute_keydown(fd, args[0]);
      } else if (strcmp(cmd, "reset") == 0) {
        checkArguments(cmd, num_args, 0, lineCount);
        execute_reset(fd, device_flags);
      } else {
        printf("Unrecognized command at line %d: '%s'\n", lineCount, cmd);
        return 1;
      }
    }
  }
  free(line);

  return 0;
}
