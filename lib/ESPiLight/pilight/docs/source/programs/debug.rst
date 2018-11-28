=============
pilight-debug
=============

Decode incoming raw streams into logical pulsetrains
----------------------------------------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-debug``
| ``pilight-debug`` [OPTION]

DESCRIPTION
===========

``pilight-debug`` is a tool to decode incoming raw streams into logical pulsetrains. It also prints certain calculated parameters of the caught pulsetrain.

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
| ``pilight-flash``
| ``pilight-raw``
| ``pilight-receive``
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
