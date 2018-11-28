.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

XBMC / Kodi
===========

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |no|        |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

*None*

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "xbmc": {
         "protocol": [ "xbmc" ],
         "id": [{
           "server": "127.0.0.1",
           "port": 9090
         }],
         "action": "shutdown",
         "media": "none"
       }
     },
     "gui": {
       "xbmc": {
         "name": "XBMC",
         "group": [ "Programs" ],
         "media": [ "all" ]
       }
     }
   }


+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| server           | *valid ip*      |
+------------------+-----------------+
| port             | 1 - 65535       |
+------------------+-----------------+
| action           | | shutdown      |
|                  | | home          |
|                  | | play          |
|                  | | pause         |
|                  | | active        |
|                  | | inactive      |
+------------------+-----------------+
| media            | | none          |
|                  | | episode       |
|                  | | movie         |
|                  | | movies        |
|                  | | song          |
|                  | | screensaver   |
+------------------+-----------------+

.. note::

   - The ``shutdown`` and ``home`` actions are only allowed for the ``none`` media.
   - The ``play``, ``home``, and  ``pause`` actions are only allowed for the ``episode``, ``movie``, ``movies``, or ``song`` media.
   - The ``active`` and ``inactive``  actions are only allowed for the ``screensaver`` media.

.. rubric:: Optional Settings

:underline:`GUI Settings`

+------------------+-------------+------------+-----------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                               |
+------------------+-------------+------------+-----------------------------------------------+
| show-media       | 1           | 1 or 0     | Don't display the media icon                  |
+------------------+-------------+------------+-----------------------------------------------+
| show-action      | 1           | 1 or 0     | Don't display the action icon                 |
+------------------+-------------+------------+-----------------------------------------------+

.. rubric:: Comment

The XBMC protocol will try to connect to a XBMC instance. However, make sure the JSON-RPC API is `enabled <http://kodi.wiki/view/JSON-RPC_API#Enabling_JSON-RPC>`_ in XBMC. You don't need to do anything more. Whenever a connection to XBMC is lost, the protocol will automatically try to reconnect. The XBMC protocol sends several messages depending on the actions in XBMC.

+-----------+----------------+
| **Media** | **Action**     |
+-----------+----------------+
| movie	    | play, pause    |
+-----------+----------------+
| song      | play, pause    |
+-----------+----------------+
| episode   | play, pause    |
+-----------+----------------+
| none	    | shutdown, home |
+-----------+----------------+

The stop event isn't recording, but pilight will interpret as the home action of the none media. So when XBMC is started or when nothing plays, the action will be home. When pilight can't connect to XBMC or when the connection is lost the shutdown action of the none media will be sent.

Important

pilight uses JSON-RPC via TCP-Port 9090. Don't mix it up with JSON-RPC via HTTP which is available over port 8080 by default (http://Kodi:8080/jsonrpc). This setting is hidden in Kodi/XBMC.