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

struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};

#define EVIOCGVERSION		_IOR('E', 0x01, int)			/* get driver version */
#define EVIOCGID		_IOR('E', 0x02, struct input_id)	/* get device ID */
#define EVIOCGKEYCODE		_IOR('E', 0x04, int[2])			/* get keycode */
#define EVIOCSKEYCODE		_IOW('E', 0x04, int[2])			/* set keycode */

#define EVIOCGNAME(len)		_IOC(_IOC_READ, 'E', 0x06, len)		/* get device name */
#define EVIOCGPHYS(len)		_IOC(_IOC_READ, 'E', 0x07, len)		/* get physical location */
#define EVIOCGUNIQ(len)		_IOC(_IOC_READ, 'E', 0x08, len)		/* get unique identifier */

#define EVIOCGKEY(len)		_IOC(_IOC_READ, 'E', 0x18, len)		/* get global keystate */
#define EVIOCGLED(len)		_IOC(_IOC_READ, 'E', 0x19, len)		/* get all LEDs */
#define EVIOCGSND(len)		_IOC(_IOC_READ, 'E', 0x1a, len)		/* get all sounds status */
#define EVIOCGSW(len)		_IOC(_IOC_READ, 'E', 0x1b, len)		/* get all switch states */

#define EVIOCGBIT(ev,len)	_IOC(_IOC_READ, 'E', 0x20 + ev, len)	/* get event bits */
#define EVIOCGABS(abs)		_IOR('E', 0x40 + abs, struct input_absinfo)		/* get abs value/limits */
#define EVIOCSABS(abs)		_IOW('E', 0xc0 + abs, struct input_absinfo)		/* set abs value/limits */

#define EVIOCSFF		_IOC(_IOC_WRITE, 'E', 0x80, sizeof(struct ff_effect))	/* send a force effect to a force feedback device */
#define EVIOCRMFF		_IOW('E', 0x81, int)			/* Erase a force effect */
#define EVIOCGEFFECTS		_IOR('E', 0x84, int)			/* Report number of effects playable at the same time */

#define EVIOCGRAB		_IOW('E', 0x90, int)			/* Grab/Release device */

// end <linux/input.h>

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

void execute_drag(int fd, int start_x, int start_y, int end_x, int end_y,
                  int num_steps, int duration_msec)
{
    int delta[] = {(end_x-start_x)/num_steps, (end_y-start_y)/num_steps};
    int sleeptime_usec = (duration_msec * 1000 / num_steps);
    int i;

    // press
    write_event(fd,3,58,90);
    write_event(fd,3,53,start_x);
    write_event(fd,3,54,start_y);
    write_event(fd,0,0,0);

    // drag
    for (i=0; i<num_steps; i++) {
      usleep(sleeptime_usec);
      write_event(fd,3,53,start_x+delta[0]*i);
      write_event(fd,3,54,start_x+delta[1]*i);
      write_event(fd,0,0,0);
    }

    // release
    write_event(fd, 3,58,0);
    write_event(fd, 0,0,0);

    // wait
    usleep(100*1000);
}

void execute_tap(int fd, int x, int y, int num_times)
{
  int i;

  for (i=0;i<num_times;i++) {
    // press
    write_event(fd,3,58,90);
    write_event(fd,3,53,x);
    write_event(fd,3,54,y);
    write_event(fd,0,0,0);
    usleep(100*1000);

    // release
    write_event(fd,3,58,0);
    write_event(fd,0,0,0);
    usleep(100*1000);

    // wait
    usleep(100*500);
  }
}

void execute_sleep(int duration_msec)
{
  usleep(duration_msec*1000);
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
        execute_tap(fd, args[0], args[1], args[2]);
      } else if (strcmp(cmd, "drag") == 0) {
        assert(num_args == 6);
        execute_drag(fd, args[0], args[1], args[2], args[3], args[4], args[5]);
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
