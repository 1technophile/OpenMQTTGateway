============
pilight-send
============

Send data to a pilight daemon
-----------------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-send`` --protocol PROTOCOL [OPTION]...
| ``pilight-send`` [--server SERVER] [ --port PORT] --protocol PROTOCOL [OPTION]...

DESCRIPTION
===========

``pilight-send`` is a tool to send data to a pilight daemon instance. It is a more versatile tool than ``pilight-control``, because it does not require the device to be previously configured.

It can be used to send signals for any pilight protocol that supports sending.

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
| ``-p``, ``--protocol=PROTOCOL``
|  Protocol to be sent
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
| ``-U``, ``--uuid=UUID``
|  UUID to identify as when connecting to the pilight daemon

Further options are required, depending on the chosen protocol. Use ``pilight-send`` ``-p`` PROTOCOL ``--help`` to find which options are required for the protocol. Some long options differ between protocols. Protocol options include:

|
| ``-a``, ``--all``, ``--id=ALL``
|  Send command to all devices with the same ID
|
| ``-a``, ``--api=API_KEY``
|  Update a Weather Underground entry with the API code
|
| ``-b``, ``--battery=BATTERY``
|  Send battery state (0 = low, 1 = normal) for weather
|  stations
|
| ``-c``, ``--code=RAW_CODE``
|  Raw code to be transmitted, as emitted by
|  ``pilight-debug``
|
| ``-c``, ``--color=COLOR``
|  Set a label device to this text color
|
| ``-c``, ``--country=COUNTRY``
|  Update a Weather Underground entry with this country
|
| ``-d``, ``--dimlevel=DIMLEVEL``
|  Set a dimmer to a specific dimlevel
|
| ``-f``, ``--off``, ``--down``, ``--stopped``
|  Send an 'off' (for switches) or 'down' (for screens) or
|  'stopped' (for programs) signal
|
| ``-g``, ``--gpio=GPIO``
|  GPIO pin number a device (for example a relay) is
|  connected to.
|
| ``-h``, ``--humidity=HUMIDITY``
|  Send humidity value for a weather station
|
| ``-i``, ``--id=ID``
|  Send a code with this id
|
| ``-l``, ``--label=LABEL``
|  Set text of a label device to this text
|
| ``-l``, ``--learn``
|  Send  multiple streams when a device is in learning mode
|  to emulate the learning mode of a remote control.
|
| ``-l``, ``--location=LOCATION``
|  Update a Weather Underground entry with this location
|
| ``-n``, ``--name=NAME``
|  Name of the program to be controlled
|
| ``-n``, ``--num``
|  Use random (super)code sequence number 0..3 (7)
|
| ``-s``, ``--systemcode=SYSTEMCODE``
|  Send a code with this systemcode
|
| ``-s``, ``--super``
|  Send to all devices, regardless of id
|
| ``-t``, ``--on``, ``--up``, ``--running``
|  Send an 'on' (for switches) or 'up' (for screens) or
|  'running' (for programs) signal.
|
| ``-t``, ``--temperature=TEMPERATURE``
|  Send temperature value for a weather station
|
| ``-u``, ``--unit=UNITCODE``,
|   ``--unitcode=UNITCODE``,
|   ``--programcode=PROGRAMCODE``
|  Send a code this unitcode. Note that some protocols use
|  long option ``--unit`` while others use long option
|  ``--unitcode`` or long option ``--programcode``.
|
| ``-u``, ``--update``
|   Update a defined Weather Underground entry

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
| ``pilight-flash``
| ``pilight-raw``
| ``pilight-receive``
| ``pilight-sha256``
| ``pilight-uuid``
