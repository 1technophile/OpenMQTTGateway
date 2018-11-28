===============
pilight-receive
===============

Show datagrams received and transmitted by pilight
--------------------------------------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-receive``
| ``pilight-receive`` [OPTION]...
| ``pilight-receive`` [--server SERVER] [ --port PORT] [OPTION]... --device DEVICE

DESCRIPTION
===========

``pilight-receive`` is a tool to show all data that is sent and received by a pilight daemon instance or a pilight adhoc network. All data is shown as-is in JSON format.

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
| ``-s``, ``--stats``
|  show CPU and RAM statistics in output
|
| ``-F``, ``--filter=PROTOCOL(S)``
|  Specify protocols that should not be printed in the
|  output stream. This is useful when trying to find
|  IDs of devices such as weather stations or remote
|  controls, but certain protocols are spamming the
|  output. For example, datetime protocol data is
|  transmitted every second, and is likely to displace
|  useful information in the output stream. It can
|  then be hidden with --filter=datetime

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
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
