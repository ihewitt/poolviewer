** At the moment this is just the content from google code dumped over, please excuse the disorganised pages! **

# PoolViewer #
Cross platform Data downloader and viewer for [Swimovate](http://www.swimovate.com/) [PoolMatePro](http://www.swimovate.com/poolmatepro.html) swim watches for Linux and Windows.

Uses [Qt](http://qt.nokia.com/products/) and [libusb](http://www.libusb.org/wiki/libusb-1.0).

# Screenshots #
| ![https://lh4.googleusercontent.com/-TPhRLqYgm7o/TvnXQ8-j2kI/AAAAAAAABv4/ZnML1VezwGQ/s400/linux-tech2.png](https://lh4.googleusercontent.com/-TPhRLqYgm7o/TvnXQ8-j2kI/AAAAAAAABv4/ZnML1VezwGQ/s400/linux-tech2.png) | ![https://lh4.googleusercontent.com/-b03COsx-lGU/TvnXSDXeKpI/AAAAAAAABwI/0UzG3jc20aA/s400/linux-vol2.png](https://lh4.googleusercontent.com/-b03COsx-lGU/TvnXSDXeKpI/AAAAAAAABwI/0UzG3jc20aA/s400/linux-vol2.png) 

# Documentation #
Overview of the user interface [Overview.pdf](http://poolviewer.googlecode.com/files/Overview.pdf)

This is a cross platform driver and viewer application for the Swimovate
Poolmate Pro swimming watch.
The poolmate dongle is actually a TI3410 USB-serial interface.
There is already a kernel driver for the TI3410 - ti_usb_3410_5052 which could
probably be used instead. However trying to get it working just resulted in
failure to download firmware errors, despite pointing it at the right firmware
from the windows driver. Also the linux driver doesn't seem to detect when
there are two configurations (i.e. the firmware is already loaded) and just
select the second without attempting to download.
Therefore.... for now will stick to the simplistic included userspace USB
driver instead although now changed to just dump down the raw data so might
add an option to download from a ttyUSB device instead just incase I get the
serial driver working.
The data protocol is pretty straightforward, the watch just serially dumps a
binary representation of its contents when it's placed onto the dock.

For a Poolmate Type A pod the ftdi_sio driver can be used however the dongle won't be detected until the vendor id's are sent to the driver such as:

```
$ modprobe ftdi_sio
$ echo 0403 8b30 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id
```
To make this persistent add a udev rule such as:

```
/etc/udev/rules.d/99-ftdi.rules
SUBSYSTEMS=="usb" ATTRS{idVendor}=="0403" ATTRS{idProduct}=="8b30" MODE:="0666"
ACTION=="add", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="8b30", RUN+="/sbin/modprobe ftdi_sio" RUN+="/bin/sh -c 'echo 0403 8b30 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id'"
```

For the original non-type A pod, the application uses libusb to talk directly to the device, make sure that you have permissions for the device with a udev rule such as:

```
/etc/udev/rules.d/51-poolmate.rules
SUBSYSTEMS=="usb" ATTRS{idVendor}=="0451" ATTRS{idProduct}=="5051" MODE:="0666"
```

## Binary packages ##
**Note** - the Linux binary installs currenly just install the executable "poolview" in /usr/bin no desktop or menu item is added. This will change at some point.

# License #
PoolViewer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

PoolViewer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
