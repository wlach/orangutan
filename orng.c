/*
** Copyright 2012, Google, Inc.
** Copyright 2012, Mozilla Foundation
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

#include "inputdefs.h"

#define FROYO_EVENT_PROTO 65536
#define ICS_EVENT_PROTO 65537

#define MAX_COMMAND_ARGS 16
#define MAX_COMMAND_LEN 256

void write_event(int fd, int type, int code, int value)
{
  struct input_event event;
  memset(&event, 0, sizeof(event));

  event.type = type;
  event.code = code;

  event.value = value;
  usleep(1000);

  int ret = write(fd, &event, sizeof(event));
  if(ret < sizeof(event)) {
    fprintf(stderr, "write event failed, %s\n", strerror(errno));
    return;
  }
}

void execute_sleep(int duration_msec)
{
  usleep(duration_msec*1000);
}

void execute_press(int fd, int version, int x, int y)
{
  if (version == ICS_EVENT_PROTO) {
    write_event(fd, 3, ABS_MT_PRESSURE, 90);
    write_event(fd, 3, ABS_MT_POSITION_X, x);
    write_event(fd, 3, ABS_MT_POSITION_Y, y);
    write_event(fd, 0, 0, 0);
  } else if (version == FROYO_EVENT_PROTO) {
    write_event(fd, 3, ABS_MT_POSITION_X, x);
    write_event(fd, 3, ABS_MT_POSITION_Y, y);
    write_event(fd, 3, ABS_MT_TOUCH_MAJOR, 33);
    write_event(fd, 3, ABS_MT_WIDTH_MAJOR, 4);
    write_event(fd, 0, 2, 0);
    write_event(fd, 0, 0, 0);
  }
}

void execute_move(int fd, int version, int x, int y)
{
  if (version == ICS_EVENT_PROTO) {
    write_event(fd, 3, ABS_MT_POSITION_X, x);
    write_event(fd, 3, ABS_MT_POSITION_Y, y);
    write_event(fd, 0, 0, 0);
  } else if (version == FROYO_EVENT_PROTO) {
    write_event(fd, 3, ABS_MT_POSITION_X, x);
    write_event(fd, 3, ABS_MT_POSITION_Y, y);
    write_event(fd, 3, ABS_MT_TOUCH_MAJOR, 33);
    write_event(fd, 3, ABS_MT_WIDTH_MAJOR, 4);
    write_event(fd, 0, 2, 0);
    write_event(fd, 0, 0, 0);
  }
}

void execute_release(int fd, int version)
{
  if (version == ICS_EVENT_PROTO) {
    write_event(fd, 3, ABS_MT_PRESSURE,0);
    write_event(fd, 0, 0, 0);
  } else if (version == FROYO_EVENT_PROTO) {
    write_event(fd, 0, 2, 0);
    write_event(fd, 0, 0, 0);
  }
}

void execute_drag(int fd, int version, int start_x, int start_y, int end_x,
                  int end_y, int num_steps, int duration_msec)
{
    int delta[] = {(end_x-start_x)/num_steps, (end_y-start_y)/num_steps};
    int sleeptime = duration_msec / num_steps;
    int i;

    // press
    execute_press(fd, version, start_x, start_y);

    // drag
    for (i=0; i<num_steps; i++) {
      execute_sleep(sleeptime);
      execute_move(fd, version, start_x+delta[0]*i, start_y+delta[1]*i);
    }

    // release
    execute_release(fd, version);

    // wait
    execute_sleep(100);
}

void execute_tap(int fd, int version, int x, int y, int num_times)
{
  int i;

  for (i=0;i<num_times;i++) {
    // press
    execute_press(fd, version, x, y);
    execute_sleep(100);

    // release
    execute_release(fd, version);
    execute_sleep(100);

    // wait
    execute_sleep(50);
  }
}

int main(int argc, char *argv[])
{
    int i;
    int fd;
    int ret;
    int version;

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
    if (ioctl(fd, EVIOCGVERSION, &version)) {
        fprintf(stderr, "could not get driver version for %s, %s\n", argv[optind], strerror(errno));
        return 1;
    }

    FILE *f = fopen(argv[2], "r");
    if (!f) {
      printf("Unable to read file %s", argv[1]);
      return 1;
    }

    line = malloc(sizeof(char)*MAX_COMMAND_LEN);
    while (fgets(line, MAX_COMMAND_LEN, f) != NULL) {
      cmd = strtok(line, " ");

      num_args = 0;
      while ((arg = strtok(NULL, " \n")) != NULL) {
        assert(num_args < MAX_COMMAND_ARGS);

        args[num_args] = atoi(arg);
        num_args++;
      }

      if (strcmp(cmd, "tap") == 0) {
        assert(num_args == 3);
        execute_tap(fd, version, args[0], args[1], args[2]);
      } else if (strcmp(cmd, "drag") == 0) {
        assert(num_args == 6);
        execute_drag(fd, version, args[0], args[1], args[2], args[3], args[4], args[5]);
      } else if (strcmp(cmd, "sleep") == 0) {
        assert(num_args == 1);
        execute_sleep(args[0]);
      } else {
        printf("Unrecognized command: '%s'", cmd);
        return 1;
      }
    }
    free(line);

    return 0;
}
