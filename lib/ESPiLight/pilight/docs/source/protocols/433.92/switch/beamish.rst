.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Beamish
=======

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

+----------------------+------------------+
| **Brand**            | **Protocol**     |
+----------------------+------------------+
| Beamish 4-AE4        | beamish_switch   |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -t --on                    send a toggle state signal to the device
   -f --off                   send a toggle state signal to the device
   -i --id=id                 control the device with this system id (1 ... 65535)
   -u --unit=unit             control the device with this unit code (1 ... 4)
   -a --all                   send on/off command to all device units belonging to this system id

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "beamish_switch" ],
         "id": [{
           "id": 8,
           "unit": 2
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
| systemcode       | 1 - 65535       |
+------------------+-----------------+
| unit             | 1 - 4           |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| readonly             | 1           | 1 or 0     | Disable controlling this device from the GUIs             |
+----------------------+-------------+------------+-----------------------------------------------------------+
| confirm              | 1           | 1 or 0     | Ask for confirmation when switching device                |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Protocol

This protocol sends 50 pulses like this

.. code-block:: console

   323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 323 1292 1292 323 323 1292 323 1292 323 1292 323 1292 323 1292 1292 323 1292 323 323 1292 323 1292 323 1292 323 1292 323 10982

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   1292 323
   323 1292
   323 1292
   323 1292
   323 1292
   323 1292
   1292 323
   1292 323
   323 1292
   323 1292
   323 1292
   323 1292
   323 10982

If we now look at carefully at these groups you can distinguish three types of groups:

- ``323 1292``
- ``1292 323``

The 1st group is defined by a short-long sequence (logical 0) The 2nd group is defined by a long-short sequence (logical 1):

.. code-block:: console

   000000000000100000110000

We can group the sequence of bits into the following groups A and B:

.. code-block:: console

   AAAAAAAAAAAAAAAA BBBB BBBB
   0000000000001000 0011 0000

Each of the groups of bits (A and B) has a specific meaning:

+-----------+-----------+-----------------+------------+-----------------+
| **Group** | **Bit #** | **Config name** | **Range**  | **Description** |
+-----------+-----------+-----------------+------------+-----------------+
| A         | 1 to 16   | id              | 1 to 65535 | SystemCode id   |
+-----------+-----------+-----------------+------------+-----------------+
| B         | 17 to 24  | unit            | 1 to 4     | ButtonCode unit |
+-----------+-----------+-----------------+------------+-----------------+

The protocol driver creates the binary presentation as required by the device

+------+------+-------+-------------------------------+
| 1100 | 0000 |	-u 1  | Button A Toggle device status |
+------+------+-------+-------------------------------+
| 0011 | 0000 | -u 2  | Button B Toggle device status |
+------+------+-------+-------------------------------+
| 0000 | 1100 | -u 3  | Button C Toggle device status |
+------+------+-------+-------------------------------+
| 0000 | 0011 | -u 4  | Button D Toggle device status |
+------+------+-------+-------------------------------+
| 0000 | 1111 | -a -t | Button ALL ON                 |
+------+------+-------+-------------------------------+
| 1100 | 0011 | -a -f | Button ALL OFF                |
+------+------+-------+-------------------------------+

So this code represents:

.. code-block:: console

   {
      "code": {
        "id": 8,
        "unit": 2
      },
      "origin": "receiver",
      "protocol": "beamish_switch",
      "uuid": "0000-00-00-00-000000",
      "repeats": 2
   }

.. rubric:: Examples

CLI command:

.. code-block:: console

   pilight-send -p beamish_switch -i 8 -u 1

.. code-block:: console

   {
     "code": {
       "systemcode": 2,
       "id": 8,
       "unit": 1
     },
     "origin": "receiver",
     "protocol": "beamish_switch",
     "uuid": "0000-00-00-00-000000",
     "repeats": 1
   }

.. code-block:: console

   pilight-send -p beamish_switch -i 8 -a -t

.. code-block:: console

   {
     "origin": "sender",
     "protocol": "beamish_switch",
     "code": {
       "id": 8,
       "all": 1,
       "state": "on"
     },
     "repeat": 1,
     "uuid": "0000-00-00-00-000000"
   }