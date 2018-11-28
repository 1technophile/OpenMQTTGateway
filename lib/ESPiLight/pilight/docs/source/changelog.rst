Changelog
=========

.. versionadded:: 8.1.0

.. rubric:: Improvements

- Ported events operators and functions to LUA
- Complete rewritten event parser with:
   - (nested) IF/ELSE/THEN support
   - case insensitive
   - operator precedence compliant
   - better error messages
   - string concatenation
   - calculations inside actions
   - etc.

.. rubric:: bugfixes

- webGUI: Take `stats-enable` into account
- webGUI: Warp long labels into multiple lines
- webGUI: Keep values together on small screens
- webGUI: Fix dimmer element being too small

.. versionadded:: 8.0.10

.. rubric:: Bugfixes

- Ignore dst in DATE_FORMAT function to keep input datetime unaffected by dst.

.. versionadded:: 8.0.9

.. rubric:: Bugfixes

- Properly reset time variable in DATE_FORMAT function to correctly handle dst.

.. versionadded:: 8.0.8

.. rubric:: Bugfixes

- --debuglevel=2 replaced --debuglevel=1. Now both are allowed.

.. versionadded:: 8.0.7

.. rubric:: Change functionality

- Openweathermap now requires a personal API key. Until now, a free pilight organization API key was used, but that exceeded the maximum allowed requests per minute.

.. rubric:: Bugfixes

- Actually allow --debuglevel=2 to see additional mail logging

.. versionadded:: 8.0.6

.. rubric:: Bugfixes

- Readded min / max dimlevel in generic_dimmer, but validation is still disabled

.. versionadded:: 8.0.5

.. rubric:: New functionality

- IPv6 support for all in library clients
- Disable min / max dimlevel in generic_dimmer

.. rubric:: Internal core changed

- Callback in mail library when host is not reachable
- Shutdown on more signals

.. rubric:: Bugfixes

- Fixed pilight removal with apt
- Calling REST API with socket API
- Prevent using ``.`` as message body
- Segfault due to wrong memory freeing

.. versionadded:: 8.0.4

.. rubric:: New functionality

- webGUI long labels are wrapped 

.. rubric:: Internal core changed

- Olsen timezone database is now used for timezone parsing
- 433.92Mhz now respect the UUID setting
- improved datetime, openweathermap, weatherunderground, and sunriseset library

.. rubric:: Bugfixes

- config not being saved at shutdown

.. versionadded:: 8.0.3

.. rubric:: Bugfixes

- webgui labels are word-wrapped into multiple lines on small screens
- webgui takes ``stats-enable`` into account by hiding CPU stats
- memory usage statistics has been removed because they were unreliable
- arctech_dimmer signals sent by pilight are now correctly received by pilight as well

.. rubric:: webserver, mail and http library

At this moment the https, mail, and webserver module and the full ``pilight-sha256`` program has been backported from rewrite. The asynchronous I/O library libuv has been added as well as the new SSL and eventpool module. The openweathermap and weather underground protocols have been adapted to use this new code as well as the pushbullet and pushover event actions.

- pilight now supports a HTTPS webserver which can be configured in the settings:

   .. code-block:: json

      { "webserver-https-port": 5002 }

- pilight also stopped detecting if the mailserver you have configures requires an SSL connection. To tell pilight about the SSL requirement of a mail server a new setting has been added. Servers that switch from a plain connection to SSL require a 0 value here:

   .. code-block:: json

      { "smtp-ssl": 1 }

.. versionadded:: 8.0.2

.. rubric:: Bugfixes

- wiringX log was scrambled

.. versionadded:: 8.0.1

.. rubric:: Bugfixes

- gpio_switch protocol that stops working after a while
- pilight not starting at boottime

.. versionadded:: 8.0

.. note::

   Not all changes from development where ported to stable. Especially the rules can break when upgraded from the latest development to the latest stable. Porting the new eventing code is in the planning real soon, but until then, just stick with development.

.. rubric:: Breaking changes

- The PHP parsing functionality has been removed.

.. rubric:: New functionality

- Proper REST API through the webserver. See `Development -> API <https://manual.pilight.org/development/api.html#webserver>`_.

- Protocol names cannot be used anymore as configuration device names.

- Supporting flashing the Arduino Uno.

- Added TFA 30.X weather stations. FIXME
- Added Quigg GT-9000 protocol. See FIXME.
- Added Secudo / FlammEx smoke sensor. FIXME.
- Added Eurodomest protocol. `Protocols -> 433.92Mhz -> Switches -> Eurodomest <https://manual.pilight.org/protocols/433.92/switch/eurodomest.html>`_
- Added TCM 218943 protocol `Protocols -> 433.92Mhz -> Weather -> TCM <https://manual.pilight.org/protocols/433.92/weather/protocols/433.92/weather/tcm.html>`_

- Allow event triggers based on received actions. See `Eventing -> Syntax <https://manual.pilight.org/eventing/syntax.html#devices>`_.
- Added the ISNOT operator. See `Eventing -> Operators <https://manual.pilight.org/eventing/operators.html>`_

- Added webGUI support for illuminance sensor.

- Allow filtering ``pilight-receive`` protocols. See `Programs -> pilight-receive <https://manual.pilight.org/programs/receive.html>`_
- Split pilight daemon debug and foreground functionality in two parameters. See `Programs -> pilight-daemon <https://manual.pilight.org/programs/daemon.html>`_

- Removed internal wiringX integration and changes to shared library linking.
- Validate duplicate 433gpio GPIO for both sender and receiver value.

- Default pilight paths have changes.
- The tzdata.json file has been deprecated and moved internally into pilight.

.. rubric:: Bugfixes

- XBMC and LIRC protocol. Properly try to reconnect when connection is lost.
- X10 Switch. The same unit was sent for units 8 and 9.
- Beamisch Switch. Resend the beamisch switch 10 times again.
- Arctech Switch. More precise pulse lengths.
- EV1527 Switch. Allow a bigger ID range.
- Conrad RSL switch. Add learn parameter for device learning.
- RSL366 Switch. Better protocol validation for less false positives.
- Arctech Dusk. Fixed swapped states.
- Clarus Switch. Prevent crashes on too long ID parameter.
- Alecto WX500 Weather Station. Fixed negative temperatures.
- Alecto WS1700 Weather Station. Better protocol implementation according to specsheet.
- Arctech Old Switch. Better protocol validation for less false positives.
- Quigg GT-1000 Switch. Added support more group codes.
- Elro 800 Switch. Support for more systemcodes.
- Teknihall Weather Station. Fixed negatives temperature values.
- Dim action. IN timeout could take too long.
- ``smtp-user`` setting. Any character is now allowed.
- ``smtp-email`` setting. Better email address validation.
- pilight startup. Start pilight after network at boottime.

.. rubric:: Internal core changes

- Frequency properties are made hardware module independent.
- Added file_get_contents function.
- Updated webGUI jQuery and moment library .
- Better internal pushbullet and pushbullet action argument parsing.
- Better binary to decimals and vice versa parsing.
- Support for AArch64 compilation.
- Send version after request values API call.
- Differentiate JSON types using bitmasks.
- Better mail library status checking.
- Added python3 client example.

- Various typo fixes.

- Fixed shared and static library linking.
- Fixed unused protocol repeat parameter.
- Fixed various uninitialized fields or incomplete buffer initializations.
- Fixed memory leaks in Dim, Label, and Switch action.
- Fixed inconsistent min and max dimlevel parsing in generic dimmer.
- Fixed webGUI dimmer display bugs.
- Fixed possible deadlock in datetime library.
- Fixed various buffer overflows in protocols.
- Fixed lm75, lm76, and bmp180 i2c-patch parsing.
- Fixed ntp time library bugs.
