# aerotools: tools for the aquaero(R) 4.0 USB device
(C) 2010 by lynix <lynix47@gmail.com>

[![Build Status](https://travis-ci.org/lynix/aerotools.svg?branch=master)](https://travis-ci.org/lynix/aerotools)

## SUMMARY

_aerotools_ is a collection of Linux programs for getting status information via
USB from the _aquaero(R)_ 4.0 USB device (see AQUAERO). It currently consists of
a CLI (see AEROCLI), a daemon (see AEROD) and a library for the device
communication, which the cli and daemon are built on top of.


## REQUIREMENTS

The device library depends on _libusb_ version 1.0 (which is the current stable
version, according to http://www.libusb.org). There are no other dependencies -
except glibc, of course.


## AEROCLI

_aerocli_ can be used to query all useful information the aquaero(R) device
holds ("useful" is subjective here, of course) in a non-interactive way, and
thus may be easily integrated in scipts.


## AEROD

The daemon, _aerod_, was written to integrate the data read from the aquaero(R)
device in any application that can query _hddtemp_ (e.g. using '${hddtemp}' in
conky). Sensor names are represented as virtual nodes under /dev (e.g.
/dev/temp3) for the third temperature sensor.

As the hddtemp protocol only supports integers, the reported values are not as
precise as the aquaero(R) provides them.

aerod can act as proxy for an existing hddtemp instance, so you don't loose your
hddtemp data when using aerod.

aerod comes with a [Munin](http://munin-monitoring.org) plugin that can be used
to graph fan speeds, flow rate and temperatures:

![](https://github.com/lynix/aerotools/blob/master/munin-plugin/aerod-fan.png)

![](https://github.com/lynix/aerotools/blob/master/munin-plugin/aerod-flow.png)

![](https://github.com/lynix/aerotools/blob/master/munin-plugin/aerod-temp.png)


## AQUAERO

The aquaero(R) 4.0 USB device is an awesome fan controlling and hardware
monitoring device for water-cooled computer systems, created by the German
company 'Aqua Computer' (http://www.aqua-computer.de). You can find more
information about the device on their website.


## AEINFO

_aeinfo_ is the original Linux software the aquaero(R) comes shipped with. It
was written in C++ by Christian Unger <coder@breakbe.at>. The development seems
currently freezed.

You can find more information about aeinfo on the development website,
http://breakbe.at/development/aquaero/. Unlike aerotools, it was designed to
allow not only querying but also changing settings on the aquaero(R)
device.


## DEVELOPMENT

This project started early 2010. Since then I first switched over to an
aquaero(R) 5.0 (which marked the birth of
[aerotools-ng](https://github.com/lynix/aerotools-ng)) and then back to air
cooling with my latest rig.

Development of _aerotools_ is therefore considered discontinued, as I can no
longer test anything on real hardware. However, pull requests are still welcome
in case you wish to continue work on the project.


## BUGS / CONTACT

Feel free to report any bugs you find using the _Issues_ function on GitHub. If
you wish to contact the author, you can drop him an email to lynix47@gmail.com.
You might wish to use his GPG key, #4804CCA9 on keys.gnupg.net.


## LICENSE

This program is published under the terms of the GNU General Public License,
version 3. See the file 'LICENSE' for more information.
