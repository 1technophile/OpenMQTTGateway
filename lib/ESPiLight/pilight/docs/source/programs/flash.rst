=============
pilight-flash
=============

Flash hardware with pilight firmware
------------------------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-flash`` --file FIRMWARE_FILE
| ``pilight-flash`` [OPTION]... --file FIRMWARE_FILE

DESCRIPTION
===========

``pilight-flash`` is a tool to flash pilight firmware to ATMel chips (such as the ATTiny and ATMega).

The pilight ATMel firmware pre-filters streams and prevents noise from reaching the pilight daemon. This
lowers the CPU usage of the pilight daemon greatly, because the pilight daemon will not continuously
attempt to interpret noise as valid protocols. Currently supported chips are the ATTiny25, ATTiny45,
ATTiny85, and ATMega328P.

Besides acting as a pre-filter, the firmware also allows using pilight with senders and receivers on non
GPIO-capable hardware (such as most desktops, servers, etc.) The flashed ATMega chip is connected via USB,
and the sender and receiver are wired to the GPIO pins on the chip.  Consult the pilight manual <https://manual.pilight.org/> for wiring schematics.

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
| ``-C``, ``--config=PATH_TO_CONFIG``
|  Path to configuration file
|
| ``-f``, ``--file=PATH_TO_FIRMWARE``
|   Firmware file to flashed

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

| ``pilight-control``
| ``pilight-daemon``
| ``pilight-debug``
| ``pilight-raw``
| ``pilight-receive``
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
