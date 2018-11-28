Settings
========

- `Introduction`_
- `Core`_
   - `port`_
   - `standalone`_
   - `pid-file`_
   - `pem-file`_
   - `log-file`_
   - `log-level`_
   - `whitelist`_
   - `stats-enable`_
   - `watchdog-enable`_
   - `gpio-platform`_
- `Webserver`_
   - `webgui-websockets`_
   - `webserver-authentication`_
   - `webserver-cache`_
   - `webserver-enable`_
   - `webserver-http-port`_
   - `webserver-https-port`_
   - `webserver-root`_
   - `webserver-user`_
- `SMTP`_
   - `smtp-sender`_
   - `smtp-user`_
   - `smtp-password`_
   - `smtp-host`_
   - `smtp-port`_
   - `smtp-ssl`_
- `Miscellaneous`_
   - `ntp-servers`_
- `Firmware`_
   - `firmware-gpio-miso`_
   - `firmware-gpio-mosi`_
   - `firmware-gpio-reset`_
   - `firmware-gpio-sck`_
- `Module Paths`_
   - `action-root`_
   - `function-root`_
   - `hardware-root`_
   - `operator-root`_
   - `protocol-root`_

Introduction
------------

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.

The way pilight functions can be altered by changing various settings. All pilight settings will be described in this chapter grouped by their respective category.

Core
----

.. _port:
.. rubric:: port

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "port": 5000 }

By default, pilight uses a random port for its socket server. Use the port setting if you want to set this to a fixed port.

.. _standalone:
.. rubric:: standalone

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "standalone": 0 }

When pilight starts, it will first check if there are other instances running inside the same network. If it does, it will connect to the other pilight instance and will join or create the AdHoc network. If you want to force pilight into running as a standalone daemon, you can set the standalone setting to 1. When running pilight in standalone mode, the SSDP server will also be disabled. This means that other pilight clients will not be able to discover it using SSDP.

.. _pid-file:
.. rubric:: pid-file

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "pid-file": "/var/run/pilight.pid" }

The pid-file is used by pilight to save the process id number of the pilight-daemon. pilight itself uses this information as one of the ways to determine if pilight is already running or not. This setting must contain a valid path to store the pid-file.

.. _pem-file:
.. rubric:: pem-file

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "pem-file": "/etc/pilight/pilight.pem" }

.. note::

   Windows

.. code-block:: json
   :linenos:

   { "pem-file": "c:/pilight/pilight.pem" }

The pem-file is used by pilight for the secure https webserver. Using the default pilight pemfile makes the secure webserver still insecure, so users are adviced to generate a custom pem file.

.. code-block:: console

   pi@pilight ~# openssl req -x509 -newkey rsa:4096 -sha256 -nodes -keyout pilight.key -out pilight.crt -subj "/CN=pilight.org" -days 3650
   pi@pilight ~# cat pilight.key pilight.crt > /etc/pilight/pilight.pem

.. _log-file:
.. rubric:: log-file

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "log-file": "/var/log/pilight.log" }

.. note::

   Windows

.. code-block:: json
   :linenos:

   { "log-file": "c:/pilight/pilight.log" }

The log-file is used by pilight to various information gathered while pilight is running. This information can be used to debug errors or gather information about triggered actions. This setting must contain a valid path to store the log-file.

.. _log-level:
.. rubric:: log-level

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "log-level": 4 }

The log-level tells pilight what messages it should log into the log-file. The higher the log-level the more messages are logged into the log-file. The highest log-level is 6 and the lowest is 0. These log-levels correspond to the following log types:

0 = emergency, 1 = alert, 2 = critical, 3 = , 4 = warning,
5 = notification, 6 = information

.. _whitelist:
.. rubric:: whitelist

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "whitelist": [ "*.*.*.*" ] }

All software able to use socket connections can communicate to pilight. Such software can be *pilight-receive*, a user visiting the webGUI, or external pilight plugins such a provided by FHEM. If you want to limit the computers in your network that can connect to pilight, you set up a whitelist. This setting should contain a list of valid IPv4 addresses that are allowed to connect to pilight. All other IPs will be blocked. If you want to allow IPv4 ranges, you can specify them by using wildcards. For example, if we want to allow all IP addresses ranging from 192.168.1.0 to 192.168.1.254 we can add the IP address 192.168.1.* to the list. If we want to allow all IP addresses ranging from 10.0.0.0 to 10.0.254.254 we can add the IP address 10.0.*.* to the list. Each whitelist entry should contain a valid IPv4 address with or without using wildcards.

.. _stats-enable:
.. rubric:: stats-enable

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "stats-enable": 1 }

pilight monitors its own CPU and RAM resource usage. This information can be shared with external clients and is shared by default with the websockets connections. If you want to disable the display of the CPU and RAM statistics and/or want to disable the communication of these statistics over the websocket connection you can set this to 0. This setting can be either 0 or 1.

.. _watchdog-enable:
.. rubric:: watchdog-enable

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "watchdog-enable": 1 }

pilight monitors its own CPU and RAM resource usage. This information is used to shutdown or terminate pilight when it uses too much CPU or RAM. If want to disable this watchdog feature and therefor the automatic termination of pilight when needed, you can set this setting to 0. This setting can be either 0 or 1.

.. _gpio-platform:
.. rubric:: gpio-platform

.. versionadded:: 8.0

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "gpio-platform": "raspberrypi2" }

pilight can be ran on various GPIO compatible platforms. However, it is impossible to reliably distinguish them all. Therefor, you must specify on which platform you are running pilight. Under the hood, pilight uses the wiringX library to interface with the GPIO of your platform. Only those platforms that are supported by wiringX are also supported by pilight. A full and recent list of supported platforms retrieved by calling the following command:

.. code-block:: console

   # ./pilight-daemon -H
   Usage: pilight-daemon [options]
            -H --help                      display usage summary
            -V --version                   display version
            -C --config                    config file
            -S --server=x.x.x.x            connect to server address
            -P --port=xxxx                 connect to server port
            -F --foreground                do not daemonize
            -D --debug                     do not daemonize and
                                           show debug information
               --stacktracer               show internal function calls
               --threadprofiler            show per thread cpu usage
               --debuglevel                show additional development info

            The following GPIO platforms are supported:
              - none
              - odroidxu4
              - odroidc2
              - odroidc1
              - raspberrypi3
              - raspberrypi2
              - raspberrypizero
              - raspberrypi1b+
              - raspberrypi1b2
              - raspberrypi1b1
              - hummingboard_edge_dq
              - hummingboard_gate_dq
              - hummingboard_edge_sdl
              - hummingboard_gate_sdl
              - hummingboard_base_dq
              - hummingboard_pro_dq
              - hummingboard_base_sdl
              - hummingboard_pro_sdl
              - orangepipc+
              - bananapim2
              - bananapi1
              - pcduino1

If you are running on a platform that doesn't support GPIO, you can either use ``none`` as the ``gpio-platform`` or remove the setting altogether.

Webserver
---------

The following settings change the way the internal webserver will serve the internal pilight webGUI or it can be disabled altogether.

.. _webgui-websockets:
.. rubric:: webgui-websockets

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webgui-websockets": 1 }

By default the webGUI communicates to pilight by using websockets. This is a relatively new technique that allows us to receive all changes from pilight instead of having to poll pilight for changes. The problem is that  some older devices and browsers do not support websockets, but they do support the polling technique. So to disable the websockets and use polling instead we set webgui-websockets setting to 0. This setting can be either 0 or 1.

.. _webserver-authentication:
.. rubric:: webserver-authentication

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-authentication": [ "username", "password" ] }

By default, pilight can be controlled from the webGUI by anyone that can access it. If you want to prevent this, you can secure the webGUI with a username and password. The username should be in plain text, the password is SHA256 encrypted. Use pilight-sha256 to create the encrypted password hash. Regular SHA256 encryption tools will not work because pilight hashes the password several thousand times.

.. code-block:: console

   pi@pilight ~# pilight-sha256 -p admin
   4f32102debed8dabd87e88cf84c752ccb23a74b29f90b42edde05cbc7be41f80

So if we want to use a username user and password admin the values should look like this:

.. code-block:: json
   :linenos:

   { "webserver-authentication": [ "user", "4f32102debed8dabd87e88cf84c752ccb23a74b29f90b42edde05cbc7be41f80" ] }

.. _webserver-cache:
.. rubric:: webserver-cache

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-cache": 1 }

pilight has the ability to cache all files used for the webGUI. This reduces the amount of reads done from the SD card on devices like the Raspberry Pi and Hummingboard, and makes it faster to load the webGUI from devices with a slow internal storage such as routers. This setting can be either 0 or 1.

.. _webserver-enable:
.. rubric:: webserver-enable

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-enable": 1 }

The pilight webserver can be turned off as a whole. This could be useful if you do not want to use the webGUI at all or if you want to use your own webserver implementation. This setting can be either 0 or 1.

.. _webserver-http-port:
.. rubric:: webserver-http-port

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-http-port": 5001 }

The pilight webserver runs by default on the non-standard port 5001. This is done to prevent interference with other webservers running on the default HTTP port 80. If you do want to run the webserver on port 80 or any other port, you can change this setting. The port specified must be a valid and unused port.

.. _webserver-https-port:
.. rubric:: webserver-https-port

.. deprecated:: 8.0

.. versionadded:: 8.0.3

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-https-port": 5002 }

The webserver does not allow secure connections by default. Currently the only way to get HTTPS support is by manually compiling pilight. The pilight secure webserver runs by default on the non-standard port 5002. This is done to prevent interference with other webservers running on the default HTTPS port 443. If you do want to run the secure webserver on port 443 or any other port, you can change this setting. The port specified must be a valid and unused port.

.. _webserver-root:
.. rubric:: webserver-root

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "webserver-root": "/usr/local/share/pilight" }

.. versionchanged:: 8.0

.. code-block:: json
   :linenos:

   { "webserver-root": "/usr/local/share/pilight/webgui" }

The webserver root tells pilight where it should look for all files that should be served by the webserver.  This setting must contain a valid path.

.. _webserver-user:
.. rubric:: webserver-user

.. note::

   Linux

.. code-block:: json
   :linenos:

   { "webserver-user": "www-root" }

.. note::

   \*BSD

.. code-block:: json
   :linenos:

   { "webserver-user": "www" }

The webserver runs by default as a non-root user. This to prevent the execution of malicious code. If you want to force the webserver to run as the root user or any other system user, you can change this setting accordingly. This setting needs to contain a valid system user.

SMTP
----

pilight has the capability to communicate with several types of mail servers. This offers pilight the possibility to use for example mail actions inside our event rules, so email messages can be sent in case of a certain event. Most users will have an email account from their internet hosting provider or free mail solutions can be used like gmail. In case of a (non-existing) gmail account named: pilight@gmail.com with password: foobar, the SMTP settings should be configured like this:

.. code-block:: json
   :linenos:

   {
     "smtp-sender": "pilight@gmail.com",
     "smtp-host": "smtp.gmail.com",
     "smtp-port": 465,
     "smtp-user": "pilight@gmail.com",
     "smtp-password": "foobar",
     "smtp-ssl": 1
   }

.. _smtp-sender:
.. rubric:: smtp-sender

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-sender": "...@....com" }

The smtp-sender requires a valid e-mail address. As soon as a mail is sent by pilight, this e-mail address will be used as the address from with the mail was sent.

.. _smtp-user:
.. rubric:: smtp-user

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-user": "...@....com" }

The smtp-user requires a valid e-mail address. This e-mail address is used to validate the account details at the e-mail provider.

.. versionchanged:: 8.0 Allow any string for smtp-user

The smtp-user can be any string, and doesn't have to be an valid e-mail address anymore.

.. _smtp-password:
.. rubric:: smtp-password

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-password": "..." }

The smtp-password is used to validate the account details at the e-mail provider together with the smtp-user setting. The password should therefore be a valid password for this e-mail account.

.. _smtp-host:
.. rubric:: smtp-host

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-host": "smtp.foo.com" }

The smtp-host setting should contain a valid mail server hostname. Normally, the host name is similar to name of the internet hosting provider.

.. _smtp-port:
.. rubric:: smtp-port

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-port": 25 }

.. deprecated:: 8.0.3

The smtp-port should contain a valid smtp server port. This can currently be either 25, 465, or 587. pilight will communicate over a secure connection when using port 465, when using port 25 or 587 it will depend on the server how pilight will set-up the connection.

.. versionchanged:: 8.0.3

The smtp-port should contain a valid smtp server port. This can be any port. SSL connections should be explicitly defined in the smtp-ssl setting.

.. versionadded:: 8.0.3

.. _smtp-ssl:
.. rubric:: smtp-ssl

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "smtp-ssl": 0 }

The smtp-ssl tells pilight if the mailserver uses a secure SSL communication from the start. If server change to SSL while initializing a connection, pilight will do so automatically as well. In those cases, this setting can be set to zero.

Miscellaneous
-------------

.. _ntp-servers:
.. rubric:: ntp-servers

.. note::

   Linux, \*BSD, and Windows

.. code-block:: json
   :linenos:

   { "ntp-servers": [ "eu.pool.ntp.org", "uk.pool.ntp.org" ] }

One important feature of any automation setup is the ability to trigger time based actions. However, these events greatly rely on a correct date and time. Problems occur when the system time is not set to the correct time (for our specific timezone). This can happen on systems like the Raspberry Pi which does not have a RTC that allows it to keep track of time when turned off. To overcome this problem pilight has the ability to retrieve the correct time by synchronizing with NTP servers. You can pick any server from http://www.pool.ntp.org/. Any number of servers can be added to the ntp-servers list. pilight will first try to synchronize with the first server. If this fails it will try the second server etc. It will continue this process until an actual response was received.

Firmware
--------

pilight provides an easy tool to flash the firmware of several microcontrollers in the form of pilight-flash. pilight flashes microcontrollers by using either bitbanging or through USB. To use bitbanging we need four GPIOs. These GPIOs are by default configured for usage on a Raspberry Pi. They can however, be changed to other GPIOs according to the device you want to use. Each GPIO is named according to the SPI requirements, but any GPIO can be used because we are not actually using SPI to communicate with our microcontrollers. In pilight the SPI identifiers MISO, MOSI, Reset and SCK are only used to separate the different GPIO.

If you are unsure what valid GPIOs are on your platform,
please refer to the http://www.wiringx.org documentation.

.. _firmware-gpio-miso:
.. rubric:: firmware-gpio-miso

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "firmware-gpio-miso": 13 }

This setting defines the GPIO pin to be used as MISO. Any valid GPIO for your platform can be used.

.. _firmware-gpio-mosi:
.. rubric:: firmware-gpio-mosi

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "firmware-gpio-mosi": 12 }

This setting defines the GPIO pin to be used as MOSI. Any valid GPIO for your platform can be used.

.. _firmware-gpio-reset:
.. rubric:: firmware-gpio-reset

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "firmware-gpio-reset": 10 }

This setting defines the GPIO pin to be used as Reset. Any valid GPIO for your platform can be used.

.. _firmware-gpio-sck:
.. rubric:: firmware-gpio-sck

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "firmware-gpio-sck": 14 }

This setting defines the GPIO pin to be used as SCK. Any valid GPIO for your platform can be used.

Module Paths
------------

pilight has the possibility to load various external modules to enhance its functionality. These modules are single files and should be placed in fixed folders. However, these folders locations can be changed by altering one of the following settings.

.. warning::

   There is generally no reason to load external modules.
   The latest pilight version always contains the latest
   protocols, hardware drivers, event operators, functions,
   and actions.

.. _action-root:
.. rubric:: action-root

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "action-root": "/usr/local/lib/pilight/action" }

pilight event actions are loaded from the action-root folder. The action-root setting must contain a valid path.

.. _function-root:
.. rubric:: function-root

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "function-root": "/usr/local/lib/pilight/function" }

pilight event actions are loaded from the function-root folder. The function-root setting must contain a valid path.

.. _hardware-root:
.. rubric:: hardware-root

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "hardware-root": "/usr/local/lib/pilight/hardware" }

pilight event actions are loaded from the hardware-root folder. The hardware-root setting must contain a valid path.

.. _operator-root:
.. rubric:: operator-root

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "operator-root": "/usr/local/lib/pilight/operator" }

pilight event actions are loaded from the operator-root folder. The operator-root setting must contain a valid path.

.. _protocol-root:
.. rubric:: protocol-root

.. note::

   Linux and \*BSD

.. code-block:: json
   :linenos:

   { "protocol-root": "/usr/local/lib/pilight/protocol" }

pilight event actions are loaded from the protocol-root folder. The protocol-root setting must contain a valid path.
