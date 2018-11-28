API
===

- `Identification`_
- `Options`_
- `Actions`_
- `Webserver (REST Api)`_
- `Heartbeat`_
- `Buffer Sizes`_

Identification
--------------

All programs communicate with each other by using JSON objects. The normal identification process is as follows:

- Connect to pilight-daemon by using the pilight IP and port. The port pilight is running at is either a randomly chosen number or a fixed number as configured in the pilight settings.
- The pilight-daemon waits for the client to identify itself by sending the following JSON object:

   .. code-block:: json
      :linenos:

      {
        "action": "identify"
      }

- Additionally, a client can send certain options to tell the daemon what level of communication is the client want to set-up. A full identification JSON looks like this:

   .. code-block:: json
      :linenos:

      {
        "action": "identify",
        "options": {
          "core": 1,
          "receiver": 1,
          "config": 1,
          "forward": 0
        },
        "uuid": "0000-d0-63-00-000000",
        "media": "all"
      }

- After each action, the daemon will send either one of the following two status responses indicating a success or failure:

   .. code-block:: json
      :linenos:

      {
        "status": "success"
      }

   .. code-block:: json
      :linenos:

      {
        "status": "failure"
      }

Options
-------

The identification options define what communication is set-up between the daemon and the client.

- receiver
   When the receiver option is set, the daemon will forward the received or sent codes to the client. An example:

   .. code-block:: json
      :linenos:

      {
        "origin": "receiver",
        "protocol": "kaku_switch",
        "code": {
          "id": 1234,
          "unit": 0,
          "off": 1
        }
      }
      {
        "origin": "sender",
        "protocol": "kaku_switch",
        "code": {
          "id": 1234,
          "unit": 0,
          "off": 1
        }
      }

   In the received JSON messages, the origin will tell if an externally received message was received or a code sent by pilight self.

- config
   When the config option is set, the daemon will communicate all configuration updates. An example:

   .. code-block:: json
      :linenos:

      {
        "origin": "config",
        "devices": [ "mainlight" ],
        "values": {
          "state": "on"
        }
      }

   |

- core
   When the core option is set, the daemon will communicate pilight core information.

- stats
   When the stats option is set, the daemon will communicate the RAM and CPU statistics.

   .. code-block:: json
      :linenos:

      {
        "origin": "core",
        "values": {
          "cpu": 0.07822473698105643,
          "ram": 0.1535397936955158
        },
        "type": -1,
        "uuid": "0000-d0-63-00-000000"
      }

   |

- forward
   When the forward option is enabled, all incoming (valid) socket data will be forwarded to the client.

The uuid setting is meant for the client to send its unique UUID.

The media setting is used to tell the daemon what information is sent based on the specific media. As can be read in the GUI configuration, a user can create different GUIs based on different devices. The currently supported GUI types are all, web, mobile, and desktop. If you define your client as one of those GUI types, pilight will only send devices, GUI elements, config updates and rules that apply the specific GUI type, leaving the rest out. Therefore, you do not have to do any additional parsing on the client side.

These options can be updated on-the-fly while the client is running. The daemon will start or stop sending specific messages. To update these options, just send another identification request. An example identification object:


   .. code-block:: json
      :linenos:

      {
        "action": "identify",
        "options": {
          "stats": 1,
          "receiver": 1
        },
        "uuid": "0000-d0-63-00-000000"
      }

Actions
-------

As we have seen in the identification process, pilight can handle several actions. The first action we encounter is the identification action. The following list contains all possible actions the daemon can handle. Again, the daemon will respond to all actions with a success and failure so the client can check if the action succeeded.

- send
   In case the client wants to send specific protocol codes, the send action is used. A sent action is accompanied by specific additional arguments:

   .. code-block:: json
      :linenos:

      {
        "action": "send",
        "code": {
          "protocol": [ "kaku_switch" ],
          "id": 1234,
          "unit": 0,
          "off": 1
        }
      }

   These are basically the command like arguments. If an argument requires a value, then the value is added to the argument (as with id and unit). If the argument does not take a value, than it is defaulted to 1 (as with off). The pilight-daemon will check if the code was valid, and report back with a failure if it was not.

- control
   The control action is used to control registered devices by using their device IDs. The pilight-daemon will check if the values are valid and report back with a failure if they are not. An example control object:

   .. code-block:: json
      :linenos:

      {
        "action": "control",
        "code": {
          "device": "mainlight",
          "state": "on",
          "values": {
            "dimlevel": 10
          }
        }
      }

   |

- registry
   The pilight registry can be managed by the registry API. The registry is a multi-purpose storage solution within pilight. Developers can store any information they want inside it so it is retrievable later on. The registry allows three types of actions: set, get, remove. The syntax for each on of them is:

   .. code-block:: json
      :linenos:

      {
        "action": "registry",
        "type": "set",
        "key": "pilight.version",
        "value": "6.0"
      }

   .. code-block:: json
      :linenos:

      {
        "action": "registry",
        "type": "get",
        "key": "pilight.version"
      }

   .. code-block:: json
      :linenos:

      {
        "action": "registry",
        "type": "remove",
        "key": "pilight.version"
      }

   The response to a get command is as follows:

   .. code-block:: json
      :linenos:

      {
        "action": "registry",
        "value": "6.0",
        "key": "pilight.version"
      }

   Please note that the pilight registry can only hold JSON string and number types.

- request config
   The request config is used to request the full configuration object from the pilight-daemon:

   .. code-block:: json
      :linenos:

      {
        "action": "request config"
      }

   |

After this command, the pilight-daemon will send the raw JSON configuration as it is used internally.

   .. code-block:: json
      :linenos:

      {
        "devices": {
          "tv": {
            "uuid": "0000-00-00-07-646b93",
            "origin": "0000-d0-63-00-000000",
            "timestamp": 0,
            "protocol": [ "relay" ],
            "id": [{
              "gpio": 3
            }],
            "state": "off",
            "default": "off"
          }
        },
        "rules": {
          "tvswitch": {
            "rule": "IF tv.state IS on THEN switch DEVICE tv TO off",
            "active": 1
          }
        },
        "gui": {
          "television": {
            "type": 4,
            "order": 1,
            "name": "tv",
            "group": [ "Living" ],
            "media": [ "all" ],
            "readonly": 0
          }
        },
        "settings": {
          "log-level": 4,
          "pid-file": "/var/run/pilight.pid",
          "log-file": "/var/log/pilight.log",
          "webserver-enable": 1,
          "webserver-http-port": 5001,
          "webserver-cache": 0,
          "webserver-root": "/usr/local/share/pilight"
        },
        "hardware": {
          "433gpio": {
            "sender": 0,
            "receiver": 1
          }
        },
        "registry": {
          "pilight": {
            "version": {
              "current": "6.0"
            }
          }
        }
      }


- request values

   .. code-block:: json
      :linenos:

      {
        "action": "request values"
      }

   pilight tries to make app development as easy as possible. This means that GUIs should only have to parse the GUI configuration object. The downside however is that the GUI object does not contain the device values. To solve this issue, the GUIs can request the values for all devices at once. The returned object will have the same information as the daemon communicates with the config option enabled, but now in bulk for all devices. An example:

   .. code-block:: json
      :linenos:

      [{
        "type": 4,
        "devices": [ "tv" ],
        "values": {
          "timestamp": 0,
          "state": "off"
        }
      },
      {
        "type": 1,
        "devices": [ "bookShelfLight" ],
        "values": {
          "timestamp": 0,
          "state": "off"
        }
      }]

   Please be aware that right after the request values object, the pilight version object is sent. It is up to the GUIs to ignore or parse this information.

Webserver (REST API)
--------------------

The webserver has some special pages:

- The config page will present the latest configuration JSON object.
- The values page will present the latest values of all device.

   The values and config page will by default only show the configuration that applies to a web GUI type. To retrieve the configuration relevant for other GUI types, use the media argument:

   .. code-block:: console

      http://x.x.x.x:5001/config?media=all

.. versionadded:: 4.0 Send codes through webserver

- The send page can be used to control devices. To use this function, call the send page with a URL-encoded "send" or "registry" JSON object like this:

   .. code-block:: console

      send%3F%7B%0A%09%22action%22%3A%20%22control%22%2C%0A%09%22code%22%3A%20%7B%0A%09%09%22device%22%3A%20%22mainlight%22%2C%0A%09%09%22state%22%3A%20%22on%22%2C%0A%09%09%22values%22%3A%20%7B%0A%09%09%09%22dimlevel%22%3A%20%2210%22%0A%09%09%7D%0A%09%7D%0A%7D

   This will send the object as described earlier.

.. versionchanged:: 8.0 Implemented REST API instead of posting arbitrary JSON codes

- The send page has the same functionality as the ``pilight-send`` program and uses the same arguments:

   .. code-block:: guess

      http://x.x.x.x:5001/send?protocol=kaku_switch&on=1&id=1&unit=1

   As can be seen, the URL arguments are the same as for the ``kaku_switch`` protocol:

   .. code-block:: console

      pi@pilight:~# pilight-send -p kaku_switch -H
      ...
      [kaku_switch]
        -t --on                        send an on signal
        -f --off                       send an off signal
        -u --unit=unit                 control a device with this unit code
        -i --id=id                     control a device with this id
        -a --all                       send command to all devices with this id
        -l --learn                     send multiple streams so switch can learn

   All protocols can be controlled using their respective arguments using the webserver send page as described in this example.

- The control page has the same functionality as the ``pilight-control`` program and uses the same arguments:

   .. code-block:: guess

      http://x.x.x.x/control?device=nasStatus&values[label]=computer%20is%20disconnected&values[color]=red

   In this case a generic_label device is being controlled

Heartbeat
---------

One special function of the *pilight-daemon* is the heartbeat. The heartbeat is meant to check if a connection is still alive. The client has to send a "HEART\n" on which the *pilight-daemon* will respond with a "BEAT\n". This is the only exception in which not a JSON object is sent.

Buffer Sizes
------------

pilight will sends all streams in 1024 bits. If these streams are smaller than 1024 bits, everything was sent at once. If a stream is 1024 bits, then you will know it is a chunk of a bigger stream. In that case, read until you encounter the end of streams delimiter which is currently made up of two new lines "\n\n".

If multiple streams were buffered somewhere, you can also distinguish them by the double new line delimiters. An example:

   .. code-block:: console
      :linenos:

      {"message":"test"}\n\n
      {"message":"test"}\n\n
      {"message":"test"}\n\n

As you can see. pilight wanted to send three messages, but the TCP sockets concatenated them to one.

The pilight socket_read function takes care of these buffered messages to check if we encountered concatenated multiple streams. pilight will then convert these messages back so the output is just one big stream separated by single newlines:

   .. code-block:: console
      :linenos:

      {"message":"test"}\n
      {"message":"test"}\n
      {"message":"test"}\n
