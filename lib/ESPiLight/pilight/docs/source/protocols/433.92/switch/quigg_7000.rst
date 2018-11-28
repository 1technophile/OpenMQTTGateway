.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Globaltronics Quigg GT-7000
===========================

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

+-------------------------------+---------------+
| **Brand**                     | **Protocol**  |
+-------------------------------+---------------+
| Aldi Quigg GT-7000            | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-7000         | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-8000         | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-FSI-02       | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-FSI-04a      | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-FSI-05       | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-FSI-05d      | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics 7008AS          | quigg_gt7000  |
+-------------------------------+---------------+
| Conrad RC-710                 | quigg_gt7000  |
+-------------------------------+---------------+
| Globaltronics GT-FSI-05       | quigg_gt7000  |
+-------------------------------+---------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -t --on           send an on signal to a device
   -f --off          send an off signal to a device
   -i --id=id        control the device with this system id (1 ... 4095)
   -u --unit=unit    control the device with this unit code (0 ... 3)
   -a --all          send command to all device units belonging to this system id
   -l --learn        send multiple streams so switch can learn

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "quigg_gt7000" ],
         "id": [{
           "id": 15,
           "unit": 0
         }],
         "state": "off"
       }
     },
     "gui": {
       "Lamp": {
         "name": "TV Backlit",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 1 - 4095        |
+------------------+-----------------+
| unit             | 1 - 3           |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. versionchanged:: 8.0 Allow ID 0

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 4095        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+-------------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                                 |
+--------------------+-------------+------------+-------------------------------------------------+
| all                | 0           | 1 or 0     | | If specified this will trigger the "group"    |
|                    |             |            | | function of the advanced remotes and trigger  |
|                    |             |            | | all registered devices for the given unitcode |
+--------------------+-------------+------------+-------------------------------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| readonly             | 1           | 1 or 0     | Disable controlling this device from the GUIs             |
+----------------------+-------------+------------+-----------------------------------------------------------+
| confirm              | 1           | 1 or 0     | Ask for confirmation when switching device                |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Protocol

The quigg_switch protocol sends 42 pulses like this

.. code-block:: console

   700 1400 700 700 1400 1400 700 1400 700 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 700 1400 1400 700 700 1400 700 1400 700 1400 1400 700 81000

The first pulse is the ``header`` and the last pulse is the ``footer``. These are meant to identify the pulses as genuine. We don't use them for further processing. The next step is to transform this output into 20 groups of 2 pulses (and thereby dropping the ``header`` and ``footer`` pulse).

.. code-block:: console

   1400 700
   700 1400
   1400 700
   1400 700
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   700 1400
   1400 700
   700 1400
   700 1400
   700 1400
   1400 700

If we now analyse these groups we can distinguish two types of groups:

#. ``700 1400``
#. ``1400 700``

So the first group is defined by a short 1st and 2nd long and the second group by a long 1st and 2nd short pulse. So we take either of these two pulses to define a logical 0 or 1. In our case a long 1st pulse means a 1 and a short 1st pulse means a 0. We then get the following output:

.. code-block:: console

	 10110000000000010001

We can group the sequence of bits into the following groups A to H:

.. code-block:: console

   AAAAAAAAAAAA BB C D E F G H
   101100000000 00 0 1 0 0 0 1

Each of the groups of bits (A to H) has a specific meaning:

+-----------+-----------+-----------------+------------+-----------------------------+
| **Group** | **Bit #** | **Config name** | **Range**  | **Description**             |
+-----------+-----------+-----------------+------------+-----------------------------+
| A         | 1 to 12   | id              | 1 to 4095  | SystemCode                  |
+-----------+-----------+-----------------+------------+-----------------------------+
| B         | 13, 14    | unit            | 1 to 3     | UnitCode                    |
+-----------+-----------+-----------------+------------+-----------------------------+
| C         | 15        | id              | 1          | Command to all devices      |
+-----------+-----------+-----------------+------------+-----------------------------+
| D         | 16        | all             | 0,1        | Switch ON or OFF            |
+           +           +                 +            + Dimmer DOWN or UP           +
|           |           |                 |            |                             |
+-----------+-----------+-----------------+------------+-----------------------------+
| E         | 17        | dimm            | 0,1        | switch, dimmer mode         |
+-----------+-----------+-----------------+------------+-----------------------------+
| F         | 18        |                 | 0          | always zero                 |
+-----------+-----------+-----------------+------------+-----------------------------+
| G         | 19        |                 | 0,1        | internal, handled by driver |
+-----------+-----------+-----------------+------------+-----------------------------+
| H         | 20        |                 | 0,1        | even parity bit             |
+-----------+-----------+-----------------+------------+-----------------------------+

So this code represents:

.. code-block:: console

  "id": 2816,
  "unit": 1
  "state": Off

.. rubric:: Examples

CLI command:

.. code-block:: console

   pilight-send -p quigg_gt7000 -i 2816 -u 1 -f

.. rubric:: Comment

Extracting the system code id from an existing Globaltronics GT-7000 remote control device either requires a special version of the BPF, or you need to bypass the BPF.

After insertion of batteries the GT-7000 defaults to system code id #2816. Pressing the button "Neuer Code" located in the battery compartment, will trigger the generation of a new system code id. These are generated in sequential order, for the current quigg_switch protocol driver the id's are:

.. code-block:: console

   2816, 1792, 3840, 128, 2176, 1152, 3200, 640, 2688, 1664, 3712, 384, 2432, 1408, 3456, 896, 2944, 1920, 3968, ....

To let the device learn a new value, press the learning mode button on the switch and send the appropriate CLI command with pilight-send (configure a switch to be unit #2 and system code id #29):

.. code-block:: console

   pilight-send -p quigg_gt7000 -i 29 -u 2 -l -t

The device learns that it has now system code id #29 and that it is unit #2 and enters ON mode (e.q. the switch is turned on). If the switch is not connected to power for an extended period of time, it will loose its configuration and reset to the default id #2816 unit #0. QUIGG_GT7000 compatible switches with integrated dimmer require that you configure the quigg_screen protocol in addition to the quigg_gt7000 protocol. On the webgui you will get a separate button to dimm the device up and down.

Subsequently the switch unit #1 with system code id #2816 is turned off.