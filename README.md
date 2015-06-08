Orangutan
=========

Orangutan is a replacement for Android's monkey/monkeyrunner tool,
intended to overcome the following limitations:

* Lack of ability to realistically simulate input events: it is difficult to
  impossible to accurately simulate touch events in terms of downtime and
  event time. The TCP/IP protocol does not currently set the down time for
  events correctly, which can result in swipes being misinterpreted as long
  presses. Running a monkey script these values are set automatically, but
  not in a way that all applications can understand: for example, Google
  Chrome for Android totally ignores a monkey-generated drag event.
* Difficult to impossible to fix bugs. Theoretically it may be possible to
  submit patches upstream, but good luck getting a patched version of
  monkeyrunner on a device running Android 2.x.

Instead of hooking into Android's java-based windowing/event system like Monkey,
Orangutan takes a different approach: injecting events directly into the kernel
/dev/input system. From the point of view of the Android system, this is totally
indistinguishable from actual touch events. Parameters such as the duration
of a press, speed of the swipe, etc. are inferred at *run time* by the Android
system.

An additional advantage of Orangutan is that it works on *any* system which provides
the /dev/input interface. In particular, this means you can use Orangutan on Mozilla's
FirefoxOS operating system.

# Requirements

* To build, you'll need a copy of the Android NDK. You can get a copy from:

    http://developer.android.com/sdk/ndk/index.html

* To run, any rooted device running Android 4.0 will work (support for devices
  running 2.2 and 2.3 coming soon).

# Building / Installing

Assuming you have installed the Android NDK, you can configure and build
Orangutan by running:

    ./configure --ndkroot=$PATH_TO_ANDROID_NDK
    make

Furthermore, you can also specify the target CPU (--arch=$TARGET_CPU), the Floating Point Unit 
(--fpu=$FPU) and the Float ABI (--float-abi=$FLOAT_ABI). If you don't specify these values, 
by default, target CPU is set to armv7-a, FPU is set to neon and the Float ABI is set to softfp. 
These are the most common values for the hardware found nowadays.

For example, on my machine I downloaded and extracted the NDK to
$HOME/opt/android-ndk-r6. So In my case I would run:

    ./configure --ndkroot=$HOME/opt/android-ndk-r6
    make

But if I wanted to target a new armv7 device with a non-NEON FPU, I should run:

    ./configure --ndkroot=$HOME/opt/android-ndk-r6 --arch=armv7 --fpu=vfpv3
    make

To install on your device (assuming it's connected via USB and developer mode
is enabled), run:

    make push

# Kernel support

Orangutan comes with a Linux kernel module that lets you emulate arbitrary
input devices. This can be useful for using Orangutan with devices which lack
a physical touchscreen, for example the pandaboard. If you just want to use
Orangutan with a normal Android or FirefoxOS device, you shouldn't need to
bother with this.

A guide on how to build kernel modules for binary-only kernels is available
at

    http://glandium.org/blog/?p=2664

The resulting binary module is named 'orng.ko' and resides in the subdirectory
'kernel/'. To copy the binary module to your device, execute

    adb push kernel/orng.ko /system/orng/orng.ko

and load the module with

    adb shell insmod /system/orng/orng.ko <module parameter>

Module parameters are either

    'names' - a comma-separated list of up to 16 device names (corresponding
              to the 'name' field in the kernel device specification), or
    'devices' - a comma-separated list of up to 16 device names (corresponding
              to the 'cname' field in the kernel device specification), or
    'bustype', 'vendor', 'product', and 'version' - a device id.

Names take precedence over device ids.

For example, to load a simulated 720p touchscreen on the pandaboard, you would
load the orangutan kernel module with:

    adb shell insmod /system/lib/modules/orng.ko devices=generic-720p_touchscreen

# Using

Orangutan currently just executes "script" files containing a sequence of
gestures. Currently the following are supported:

* Drag: Simulates a drag (panning) gesture. Syntax:

    drag [start x] [start y] [end x] [end y] [num steps] [duration in msec]

* Tap: Simulates a sequence of taps. Syntax:

    tap [x] [y] [num times] [duration of each tap in msec]

* Sleep: Sleeps for a specified period of time. Syntax:

    sleep [duration in msec]

* Key down: Simulates a press down of the specified key. Syntax:

    keydown [key number]

* Key up: Simulates a release of the specified key. Syntax:

    keyup [key number]

* Reset: Send events to reset kernel cached values to 0. Syntax:

    reset

An example script file which fairly simulates a double tap, then a pan gesture,
then a sleep for two seconds on a Galaxy Nexus in landscape mode might be:

    tap 175 630 2 200
    drag 200 200 600 200 10 100
    sleep 2000

Comments can also be added, either on a line by itself, e.g.:

    # This is a comment.

Or at the end of the line:

    sleep 2000 # This is a comment.

Block comments are also allowed at various positions:

    { This is a comment. } tap 175 630 2 200
    tap 175 630 { This is a comment. } 2 200
    tap 175 630 2 200 { This is a comment. }

Block comments will also output their contents to adb stdout prefixed by "{}: ",
and will be printed before the execution of the command:

 {}: This is a comment.

We can also chain commands together with ";", linking them this way:

    { before tap } tap 175 630 2 200 ; { after tap } sleep 2000 # This is a comment.

To execute a script file, simply copy it onto the device, and run orng utility
against it as follows.

    /data/local/orng [device name] [script file]

The device name varies per device, you can generally figure it out by running
"getevent" on the device and seeing what device corresponds to the touch
screen. On the Galaxy Nexus for example, we see the following output:

    add device 1: /dev/input/event4
      name:     "lightsensor-level"
    add device 2: /dev/input/event3
      name:     "proximity"
    add device 3: /dev/input/event2
      name:     "tuna-gpio-keypad"
    add device 4: /dev/input/event0
      name:     "barometer"
    add device 5: /dev/input/event5
      name:     "Tuna Headset Jack"
    add device 6: /dev/input/event1
      name:     "Melfas MMSxxx Touchscreen"

We can guess the /dev/input/event1 is the touchscreen by its description.
Thus, if your script file was stored in '/mnt/sdcard/script' you would run the
following command from an adb shell:

    /data/local/orng /dev/input/event1 /mnt/sdcard/script
