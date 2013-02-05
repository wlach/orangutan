Orangutan
=========

Orangutan is intended as replacement for Android's monkey/monkeyrunner tool,
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

An additional advantage of Orangutan is that it should run/work on *any* system
which provides the /dev/input interface. This means that it should be possible
to use Orangutan on Mozilla's B2G operating system (which reuses much of Android's
kernel and userspace functionality).

# Requirements

* To build, you'll need a copy of the Android NDK. You can get a copy from:

    http://developer.android.com/sdk/ndk/index.html

* To run, any rooted device running Android 4.0 will work (support for devices
  running 2.2 and 2.3 coming soon).

# Building / Installing

Assuming you have installed the Android NDK, you can configure and build
Orangutan by running:

    ./configure $PATH_TO_ANDROID_NDK
    make

For example, on my machine I downloaded and extracted the NDK to
$HOME/opt/android-ndk-r6. So In my case I would run:

    ./configure $HOME/opt/android-ndk-r6
    make

To install on your device (assuming it's connected via USB and developer mode
is enabled), run:

    make push

If you want to use Orangutan with Eideticker, you'll need to install it in
/system/xbin. Since this directory is normally readonly, you'll need to remount it.
This can usually be done using something resembling the following procedure (assuming
you already have a copy in /data):

    adb shell su -c 'mount -o remount,rw /dev/block/mmcblk0p1 /system'
    adb shell su -c 'cp /data/orng /system/xbin/orng'

Exact instructions may vary depending on the phone you're working with and its
partition layout.

# Kernel support

Orangutan comes with a Linux kernel module that lets you emulate arbitrary
input devices. The use of the module is optional.

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

An example script file which fairly simulates a double tap, then a pan gesture,
then a sleep for two seconds on a Galaxy Nexus in landscape mode might be:

    tap 175 630 2 200
    drag 200 200 600 200 10 100
    sleep 2000

To execute a script file, simply copy it onto the device, and run orng utility
against it as follows.

    /system/xbin/orng [device name] [script file]

The device name varies per device, you can generally figure it out by running
"getevent" on the device and seeing what device corresponds to the touch
screen. On the Galaxy Nexus

For example, if your script file was '/mnt/sdcard/script' and you were
running on a Galaxy Nexus, you would run the following command from an adb
shell:

    /system/xbin/orng /dev/input/event1 /mnt/sdcard/script
