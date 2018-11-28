===========
pilight-raw
===========

Print received raw streams
--------------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-raw``
| ``pilight-raw`` [OPTION]...

DESCRIPTION
===========

``pilight-raw`` is a tool to print raw streams as they are picked up by the receiver hardware.

Prints data to STDOUT until interrupted with ^C.

This tool requires root privileges.

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
| ``-L``, ``--linefeed``
|  Structure raw printout with newlines between (probably) separate pulsetrains.

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
| ``pilight-receive``
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
