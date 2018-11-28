.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

GPIO Switch
===============

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

+------------------+--------------+
| **Brand**        | **Protocol** |
+------------------+--------------+
| Raspberry Pi     | cpu_temp     |
+------------------+--------------+
| Hummingboard     | cpu_temp     |
+------------------+--------------+

.. rubric:: Sender Arguments

.. code-block:: console

   -g --gpio=gpio    the gpio the switch is connected to

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "switch": {
         "protocol": [ "gpio_switch" ],
         "id": [{
           "gpio": 1
         }],
         "state": "off"
       }
     },
     "gui": {
       "switch": {
         "name": "Switch",
         "group": [ "Misc" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| gpio             | 0 - 255         |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. note::

   The ``gpio`` value should be a valid GPIO for the platform used

.. rubric:: Optional Settings

:underline:`Device Settings`

+------------------+-------------+------------+-----------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                               |
+------------------+-------------+------------+-----------------------------------------------+
| readonly         | 0           | 1 or 0     | Disable controlling this device from the GUIs |
+------------------+-------------+------------+-----------------------------------------------+
| confirm          | 0           | 1 or 0     | Ask for confirmation when switching device    |
+------------------+-------------+------------+-----------------------------------------------+

.. rubric:: Comments

The GPIO switch protocol follows the wiringX numbering as can be found here: http://wiringx.org