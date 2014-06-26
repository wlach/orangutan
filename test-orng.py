#!/usr/bin/env python

import mozdevice
import optparse
import os
import sys
import tempfile

class DeviceController(object):

    def __init__(self, dimensions, swipe_padding, input_device, orng_path):
        self.dimensions = dimensions
        self.swipe_padding = swipe_padding
        self.dm = mozdevice.DeviceManagerADB()
        self.input_device = input_device
        self.orng_path = orng_path

    def get_drag_event(self, touchstart_x1, touchstart_y1, touchend_x1,
                       touchend_y1, duration=1000, num_steps=5):

        return "drag %s %s %s %s %s %s" % (int(touchstart_x1), int(touchstart_y1),
                                           int(touchend_x1), int(touchend_y1),
                                           num_steps, duration)

    def get_sleep_event(self, duration=1.0):
        return "sleep %s" % int(float(duration) * 1000.0)

    def get_tap_event(self, x, y, times=1):
        return "tap %s %s %s 100" % (int(x), int(y), times)

    def get_scroll_events(self, direction, numtimes=1, numsteps=10, duration=100):
        events = []
        x = int(self.dimensions[0] / 2)
        ybottom = self.dimensions[1] - self.swipe_padding[3]
        ytop = self.swipe_padding[0]
        (p1, p2) = ((x, ybottom), (x, ytop))
        if direction == "up":
            (p1, p2) = (p2, p1)
        for i in range(int(numtimes)):
            events.append(self.get_drag_event(p1[0], p1[1], p2[0], p2[1], duration,
                                              int(numsteps)))
        return events

    def get_swipe_events(self, direction, numtimes=1, numsteps=10, duration=100):
        events = []
        y = (self.dimensions[1] / 2)
        (x1, x2) = (self.swipe_padding[2],
                    self.dimensions[0] - self.swipe_padding[0])
        if direction == "left":
            (x1, x2) = (x2, x1)
        for i in range(int(numtimes)):
            events.append(self.get_drag_event(x1, y, x2, y, duration,
                                              int(numsteps)))
        return events

    def get_pinch_event(self, touch1_x1, touch1_y1, touch1_x2, touch1_y2,
                       touch2_x1, touch2_y1, touch2_x2, touch2_y2,
                       numsteps=10, duration=1000):
        return "pinch %s %s %s %s %s %s %s %s %s %s" % (touch1_x1, touch1_y1,
                                                        touch1_x2, touch1_y2,
                                                        touch2_x1, touch2_y1,
                                                        touch2_x2, touch2_y2,
                                                        numsteps,
                                                        duration)

    def get_cmd_events(self, cmd, args):
        if cmd == "scroll_down":
            cmdevents = self.get_scroll_events("down", *args)
        elif cmd == "scroll_up":
            cmdevents = self.get_scroll_events("up", *args)
        elif cmd == "swipe_left":
            cmdevents = self.get_swipe_events("left", *args)
        elif cmd == "swipe_right":
            cmdevents = self.get_swipe_events("right", *args)
        elif cmd == "drag":
            cmdevents = [self.get_drag_event(*args)]
        elif cmd == "tap":
            cmdevents = [self.get_tap_event(*args)]
        elif cmd == "double_tap":
            cmdevents = [self.get_tap_event(*args, times=2)]
        elif cmd == "pinch":
            cmdevents = [self.get_pinch_event(*args)]
        elif cmd == "keydown":
            cmdevents = ["keydown %s" % args[0]]
        elif cmd == "keyup":
            cmdevents = ["keyup %s" % args[0]]
        elif cmd == "sleep":
            if len(args):
                cmdevents = [self.get_sleep_event(duration=args[0])]
            else:
                cmdevents = [self.get_sleep_event()]
        else:
            raise Exception("Unknown command")

        return cmdevents

    def execute_script(self, events):
        '''Executes a set of orangutan commands on the device'''
        with tempfile.NamedTemporaryFile() as f:
            f.write("\n".join(events) + "\n")
            f.flush()
            remotefilename = os.path.join(self.dm.getDeviceRoot(),
                                          os.path.basename(f.name))
            self.dm.pushFile(f.name, remotefilename)
            self.dm.shellCheckOutput([self.orng_path, self.input_device,
                                   remotefilename])
            self.dm.removeFile(remotefilename)

    def execute_command(self, cmd, args):
        cmdevents = self.get_cmd_events(cmd, args)
        if cmdevents:
            self.execute_script(cmdevents)


def main(args=sys.argv[1:]):
    parser = optparse.OptionParser(usage="usage: %prog")
    parser.add_option("--device-dimensions", dest="device_dimensions",
                      help="device dimensions in array format [x,y]",
                      default="[320,480]")
    parser.add_option("--swipe-padding", dest="swipe_padding",
                      help="swipe padding in array format [x1,y1,x2,y2]",
                      default="[40,40,40,40]")
    parser.add_option("--input-device", dest="input_device",
                      help="input device to use (default: /dev/input/event2)",
                      default="/dev/input/event2")
    parser.add_option("--orng-path", dest="orng_path",
                      help="path to orng executable (default: /data/local/orng)",
                      default="/data/local/orng")

    options, args = parser.parse_args()

    controller = DeviceController(eval(options.device_dimensions),
                                  eval(options.swipe_padding),
                                  options.input_device, options.orng_path)

    print "READY"
    sys.stdout.flush()

    while 1:
        try:
            line = sys.stdin.readline()
        except KeyboardInterrupt:
            break

        if not line:
            break

        tokens = line.rstrip().split()
        if len(tokens) < 1:
            raise Exception("No command")

        (cmd, args) = (tokens[0], tokens[1:])
        controller.execute_command(cmd, args)

if __name__ == '__main__':
    main()
