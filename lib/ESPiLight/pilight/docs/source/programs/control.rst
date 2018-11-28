===============
pilight-control
===============

Control pilight devices
-----------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-control`` --device DEVICE --state STATE
| ``pilight-control`` [OPTION]... --device DEVICE --values VALUES
| ``pilight-control`` [--server SERVER] [ --port PORT] [OPTION]... --device DEVICE

DESCRIPTION
===========

``pilight-control`` is a tool to control a device configured in a pilight daemon configuration. It supports switching the state and/or modifying other values (such as dimlevel, temperature, or humidity) of the device.

OPTIONS
=======

Mandatory arguments to long options are mandatory for short options too.

|
| ``-H``, ``--help``
|  Print allowed options and exit
|
| ``-V``, ``--version``
|  Print version information and exit
|
| ``-S``, ``--server=x.x.x.x``
|  Connect to a pilight daemon at this address. Requires -P
|
| ``-P``, ``--port=xxxx``
|  Connect to a pilight daemon at this port. Requires -S
|
| ``-C``, ``--config=PATH_TO_CONFIG``
|  Path to configuration file
|
| ``-d``, ``--device=DEVICE_NAME``
|  Name of the configured device to be controlled
|
| ``-s``, ``--state=STATE``
|  State to switch DEVICE to
|
| ``-v``, ``--values=VALUES``
|  Supply other values for DEVICE, such as dimlevel=10

BUGS
====

Please report all bugs on GitHub <https://github.com/pilight/pilight/>.

AUTHOR
======

Curlymo <info@pilight.org> and contributors.

This man page was originally written by pilino1234 <pilino1234@zoho.eu>.

WWW
===

https://www.pilight.org/

SEE ALSO
========

| ``pilight-daemon``
| ``pilight-debug``
| ``pilight-flash``
| ``pilight-raw``
| ``pilight-receive``
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
