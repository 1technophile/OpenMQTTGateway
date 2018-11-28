.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

TechLiCo
========

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
| TechLiCo             | techlico_switch  |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -t --on                    send a toggle state signal to the device
   -f --off                   send a toggle state signal to the device
   -i --id=id                 control the device with this system id (1 ... 65535)
   -u --unit=unit             control the device with this unit code (1 ... 4)

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "techlico_switch" ],
         "id": [{
           "id": 4935,
           "unit": 1
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
| id               | 1 - 65535       |
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

The techlico_switch protocol sends 50 pulses like this

.. code-block:: console

   208 624 208 832 208 624 624 208 208 624 208 624 624 208 624 208 208 832 624 208 208 832 208 624 208 832 624 208 624 208 624 208 208 832 208 624 208 832 208 832 208 624 208 624 624 208 624 208 208 7072

The last two pulses are the ``footer``. These are meant to identify the pulses as genuine. We don't use them for further processing. The next step is to transform this output into 24 groups of 2 pulses (and thereby dropping the ``footer`` pulse).

.. code-block:: console

   208 624
   208 832
   208 624
   624 208
   208 624
   208 624
   624 208
   624 208
   208 832
   624 208
   208 832
   208 624
   208 832
   624 208
   624 208
   624 208
   208 832
   208 624
   208 832
   208 832
   208 624
   208 624
   624 208
   624 208
   208 7072

If we now analyse these groups we can distinguish two types of groups:

- ``208 624``
- ``624 208``

The 1st group is defined by a short-long sequence (logical 0) The 2nd group is defined by a long-short sequence (logical 1):

Binary representation:

.. code-block:: console

   000100110100011100000011

We can group the sequence of bits into the following groups A and B:

.. code-block:: console

   AAAAAAAAAAAAAAAA BBBB BBBB
   0001001101000111 0000 0011

Each of the groups of bits (A and B) has a specific meaning:

+-----------+-----------+-----------------+------------+-----------------+
| **Group** | **Bit #**	| **Config name** | **Range**  | **Description** |
+-----------+-----------+-----------------+------------+-----------------+
| A         | 1 to 16   | id:             | 1 to 65535 | SystemCode id   |
+-----------+-----------+-----------------+------------+-----------------+
| B         | 17 to 24  | unit:           | 1 to 4     | ButtonCode unit |
+-----------+-----------+-----------------+------------+-----------------+

The protocol driver creates the binary presentation as required by the device

+-----------+------------------------------------+
| 0000 0011 | -u 1 Button A Toogle device status |
+-----------+------------------------------------+
| 1100 0000 | -u 2 Button B Toogle device status |
+-----------+------------------------------------+
| 0000 1111 | -u 3 Button C Toogle device status |
+-----------+------------------------------------+
| 0000 1100 | -u 4 Button D Toogle device status |
+-----------+------------------------------------+

So this code represents:

.. code-block:: console

   {
      "code": {
         "id": 4935,
         "unit": 1
      },
      "origin": "receiver",
      "protocol": "techlico_switch",
      "uuid": "0000-00-00-00-000000",
      "repeats": 2
   }

.. rubric:: Examples

CLI command:

.. code-block:: console

   pilight-send -p techlico_switch -i 4935 -u 1

.. rubric:: Known Limitations

As the device will not report its state, and the same Button on the original remote control toggles the state of the device, the GUI may show the wrong status of the device.