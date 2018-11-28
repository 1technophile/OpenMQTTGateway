Adhoc Network
=============

- `Introduction`_
- `Network of Senders and Receivers`_
- `Network of (Conflicting) Sensors and Relays`_
- `Stable Mail Daemon`_

Introduction
------------

The probably least known but advanced feature of pilight is its adhoc network functionality. This basically means that you can create a big network of pilight instances (such as several Raspberry Pis and Windows or Linux computers) working together. A bit like the Borg from Star Trek.

Consider the following, quite common, user case scenario. We have one Raspberry Pi running pilight with a sender and receiver connected. This single pilight instance controls our remote controlled lights and can retrieve weather information from our weather station and/or from e.g. openweathermap.org. This system has already been set up and works.

Network of Senders and Receivers
--------------------------------

.. only:: html

   .. |fig1h| image:: images/adhoc-network-nodeA+B.svg
   .. |fig2h| image:: images/adhoc-network-nodeA+B.svg

   .. rst-class:: alt

   +-----------------+-----------------+
   | |fig1h|         | |fig2h|         |
   +-----------------+-----------------+
   | Image 1: Node A | Image 1: Node B |
   +-----------------+-----------------+

.. only:: latex

   .. |fig1l| image:: images/adhoc-network-nodeA+B.pdf
   .. |fig2l| image:: images/adhoc-network-nodeA+B.pdf

   .. rst-class:: alt

   +-----------------+-----------------+
   | |fig1l|         | |fig2l|         |
   +-----------------+-----------------+
   | Image 1: Node A | Image 1: Node B |
   +-----------------+-----------------+

The biggest limitation of a home domotica set-up is its range. Of course we can tinker with antennas and such, but that does not always solve the problem. How then, can we reach those corners in our house that the single Raspberry Pi with pilight cannot reach?

The solution is to create a network of senders and receivers all working together. That is exactly what pilight offers with the AdHoc network functionality. To be able to use this feature, we need (at least) two pilight capable computers. In this example we use two Raspberry Pis to control our devices; a lamp in our bookshelf ("bookshelf") on one side of the house and a lamp in the garden ("gardenlights") on the other side:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "bookshelf": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 1
         }],
         "state": "off"
       },
       "gardenlights": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 2
         }],
         "state": "off"
       }
     }
   }

Our settings for Node A will look like this:

.. code-block:: json
   :linenos:

   {
     "settings": {
     "log-level": 6,
     "log-file": "/var/log/pilight-daemon.log",
     "webserver-enable": 1,
     "webserver-cache": 1,
     "webserver-http-port": 5001,
     "webserver-root": "/usr/local/share/pilight/",
     "whitelist": ""
   }

Now, when we start pilight in debug mode in Node A, we can see this:

.. code-block:: console

   [Jan 24 15:34:18:191682] pilight-daemon: DEBUG: ssdp sent search
   [Jan 24 15:34:18:292486] pilight-daemon: NOTICE: no pilight daemon found, daemonizing

pilight knows it is (currently) the only instance running inside the home network and will therefore start as a main daemon. However, we notice that we cannot control both our bookshelf lamp and the light in the garden with a single Raspberry Pi with pilight, because of the limited range of the sender and receiver. The solution is to add another Raspberry Pi with pilight on the other side of the house and to connect them to the home network. The settings of Node B will look like this:

.. code-block:: json
   :linenos:

   {
     "settings": {
       "log-level": 6,
       "log-file": "/var/log/pilight-daemon.log",
       "whitelist": ""
     }
   }

As you can see here, we have removed the **webserver settings**, because a pilight node **will not have its webserver enabled**. Once we start Node B in debug mode, we can see the following:

.. code-block:: console

   [Jan 24 15:40:44:334028] pilight-daemon: DEBUG: ssdp sent search
   [Jan 24 15:40:44:434492] pilight-daemon: NOTICE: a pilight daemon was found @192.168.1.100, clientizing

pilight has found the first daemon Node A and will connect to it. It will identify as a node and connect to it
(clientizing):

.. code-block:: console

   [Jan 24 15:40:44:544657] pilight-daemon: DEBUG: socket write succeeded: {"action":"identify","options":{"receiver":1,"forward":1,"config":1},"uuid":"0363-00-00-63-000300"}
   [Jan 24 15:40:44:547148] pilight-daemon: DEBUG: socket recv: {"status":"success"}
   [Jan 24 15:40:44:547455] pilight-daemon: DEBUG: socket write succeeded: {"action":"request config"}

The last important part is that Node B will request the configuration from Node A, so that all nodes work with the same configuration definition as the main daemon (Node A). Once a code is received on Node B, it will be synchronised with Node A and Node A will again synchronize it with Node C etc. Whenever you send a code (using a 433MHz remote or the webGUI) it will automatically arrive at Node A, because all clients will always connect to the main daemon. Node A will transfer send request (such as to control a socket) to all connected nodes. So not only Node A will send but also **all other nodes**.

If the main daemon Node A crashes, the network is down. However, the other nodes will keep running until the main daemon is back up again. They will of course first clear the local configuration and resynchronize it with the daemon (because pilight cannot know what the new configuration will be):

.. code-block:: console

   [Jan 24 15:45:37:250271] pilight-daemon: DEBUG: garbage collected config library
   [Jan 24 15:45:37:250397] pilight-daemon: NOTICE: connection to main pilight daemon lost
   [Jan 24 15:45:37:250691] pilight-daemon: NOTICE: trying to reconnect...
   [Jan 24 15:45:40:250971] pilight-daemon: DEBUG: ssdp sent search
   [Jan 24 15:45:40:350471] pilight-daemon: ERROR: no pilight ssdp connections found
   [Jan 24 15:45:40:350758] pilight-daemon: DEBUG: garbage collected config library
   [Jan 24 15:45:40:350835] pilight-daemon: NOTICE: connection to main pilight daemon lost
   [Jan 24 15:45:40:350913] pilight-daemon: NOTICE: trying to reconnect...

Once the main daemon is back online (which does not necessarily have to be Node A), all nodes will reconnect:

.. code-block:: console

   [Jan 24 15:47:50:565899] pilight-daemon: NOTICE: trying to reconnect...
   [Jan 24 15:47:53:566159] pilight-daemon: DEBUG: ssdp sent search
   [Jan 24 15:40:44:544657] pilight-daemon: DEBUG: socket write succeeded: {"action":"identify","options":{"receiver":1,"forward":1,"config":1},"uuid":"0363-00-00-63-000300"}
   [Jan 24 15:40:44:547148] pilight-daemon: DEBUG: socket recv: {"status":"success"}
   [Jan 24 15:40:44:547455] pilight-daemon: DEBUG: socket write succeeded: {"action":"request config"}

This means that when you want to update the configuration with new devices, you only have to restart the main daemon and all nodes will reconnect and resynchronize the new configuration file. This way we can control both the bookshelf lamps in front of our house and the backyard light at the back.

pilight can also control various other devices like relays and read from sensors for temperature and/or humidity in the same network. We are going to continue on the previous example but add two additional pilight instances.

Network of (Conflicting) Sensors and Relays
-------------------------------------------

pilight can also control various other devices like relays and read from sensors for temperature and/or humidity in the *same network*. We are going to continue on the previous example but add two additional pilight instances.

.. only:: html

   .. |fig3h| image:: images/adhoc-network-nodeC.svg
   .. |fig4h| image:: images/adhoc-network-nodeD.svg

   .. rst-class:: alt

   +-----------------+-----------------+
   | |fig3h|         | |fig4h|         |
   +-----------------+-----------------+
   | Image 1: Node C | Image 1: Node D |
   +-----------------+-----------------+

.. only:: latex

   .. |fig3l| image:: images/adhoc-network-nodeC.pdf
   .. |fig4l| image:: images/adhoc-network-nodeD.pdf

   .. rst-class:: alt

   +-----------------+-----------------+
   | |fig3l|         | |fig4l|         |
   +-----------------+-----------------+
   | Image 1: Node C | Image 1: Node D |
   +-----------------+-----------------+

Node C is meant to add additional coverage in the house to make sure we receive all signals, but it also has a relay connected. We are going to use this relay to turn our television set on and off. The new configuration file will look like this:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "bookshelf": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 1
         }],
         "state": "off"
       },
       "television": {
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 4
         }],
         "state": "off"
       },
       "gardenlights": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 2
         }],
         "state": "off"
       }
     }
   }

Until now, we did not encounter any issues with our pilight AdHoc network. But what happens when we add  another relay (also connected to GPIO 3, but on Node D) to control a heater? Our new configuration file will look like this:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "heater": {
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 4
         }],
         "state": "off"
       },
       "bookshelf": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 1
         }],
         "state": "off"
       },
       "television": {
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 4
         }],
         "state": "off"
       },
       "gardenlights": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 2
         }],
       "state": "off"
       }
     }
   }

As you might see, we have a problem here. As soon as I turn my television set on, I also turn on the heater and when I turn the heater off, the television will go off as well. In this case, pilight has no way of knowing which relay it needs to control so it just switches all of them. So if I set GPIO 3 of Node D to HIGH, GPIO 3 of Node A, B, C will also be set HIGH. pilight offers a solution to this issue by giving all pilight instances a specific UUID. By adding the UUID of a pilight instance in the configuration file, pilight will know exactly what relay it should control. To know what the UUID of our pilight instance is, we can run the program called pilight-uuid. We are going to run this program first to know what the UUID of Node C is by running it on that Raspberry Pi:

.. code-block:: console

   pi@pilight:~# pilight-uuid
   0338-00-00-38-000300

And then on the Raspberry Pi of Node D to see what his UUID is:

.. code-block:: console


   pi@pilight:~# pilight-uuid
   0363-00-00-63-000300

Now we edit our configuration file incorporating the new UUID values:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "heater": {
         "uuid": "0338-00-00-38-000300",
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 4
         }],
         "state": "off"
       },
       "bookshelf": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 1
         }],
         "state": "off"
       },
       "television": {
         "uuid": "0338-00-00-68-000300",
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 4
         }],
         "state": "off"
       },
       "gardenlights": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456
           "unit": 2
         }],
         "state": "off"
       }
     }
   }

If you now want to turn the television set on, pilight knows that it should only control the relay connected to Node C with the UUID 0338-00-00-38-000300. The same would count for sensors connected to your Raspberry Pi. Just add proper UUID values to them, and pilight will know which sensor is connected to which pilight node.

Stable Mail Daemon
------------------

As you might have noticed, the main daemon is very important in the pilight AdHoc network. Once the main daemon crashes, the whole network will be down. You can easily restore the network by just restarting the main daemon. However, you might also have noticed that a Raspberry Pi is a bit less stable than normal everyday computers and less stable then your regular NAS system. What about running the main daemon on there? You can!

pilight has been tested on various platforms other than just the Raspberry Pi. It successfully ran on \*BSD and Debian based systems. The only problem is that these consumer mainboards generally does not have GPIO capability. That is not a problem because pilight can just run on these devices when you remove all hardware definitions.
