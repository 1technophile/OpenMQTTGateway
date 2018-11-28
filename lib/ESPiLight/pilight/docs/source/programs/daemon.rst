==============
pilight-daemon
==============

The pilight daemon
------------------

:Date:           2017
:Copyright:      MPLv2
:Version:        7.0
:Manual section: 1
:Manual group:   pilight 7.0 man pages

SYNOPSIS
========

| ``pilight-daemon``
| ``pilight-daemon`` [OPTION]...
| ``pilight-daemon`` [--server SERVER] [ --port PORT] [OPTION]...

DESCRIPTION
===========

``pilight-daemon`` is the main software of the pilight project. It is responsible for handling and interpreting received signals, as well as generating signals to communicate with and control various devices.

A configuration file is required. By default, ``/etc/pilight/config.json`` is used.  Configuration files are written in the JSON format.

Consult the pilight manual <https://manual.pilight.org/> for details on how to configure and use pilight.

By default, the pilight daemon hosts a webgui which allows controlling devices through a clickable interface in a webbrowser. The pilight daemon runs a socket server and provides a REST API to interface with the pilight daemon.

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
|  Specify to use configuration file at PATH rather than standard configuration
|
| ``-S``, ``--server=x.x.x.x``
|  Connect to a pilight daemon at this address. Requires -P
|
| ``-P``, ``--port=xxxx``
|  Connect to a pilight daemon at this port. Requires -S
|
| ``-F``, ``--foreground``
|  Do not daemonize
|
| ``-D``, ``--debug``
|  Do not daemonize and print more debug information

Debugging options:

|
| ``--stacktracer``
|  Show internal function calls
|
| ``--threadprofiler``
|  Show per thread CPU usage
|
| ``--debuglevel``
|  Show additional development information

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
| ``pilight-debug``
| ``pilight-flash``
| ``pilight-raw``
| ``pilight-receive``
| ``pilight-send``
| ``pilight-sha256``
| ``pilight-uuid``
