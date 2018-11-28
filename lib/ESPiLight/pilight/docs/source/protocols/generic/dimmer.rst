.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Dimmer
======

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |no|        |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

*None*

.. rubric:: Sender Arguments

.. code-block:: console

   -t --on                  send an on signal
   -f --off                 send an off signal
   -i --id=id               control a device with this id
   -d --dimlevel=dimlevel   send a specific dimlevel

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "generic_dimmer" ],
         "id": [{
           "id": 100
         }],
         "state": "off",
         "dimlevel": 0
       }
     },
     "gui": {
       "dimmer": {
         "name": "Dimmer",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 99999       |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+
| dimlevel         | 0 - 99999       |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`GUI Settings`

+------------------+-------------+------------+-----------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                               |
+------------------+-------------+------------+-----------------------------------------------+
| readonly         | 0           | 1 or 0     | Disable controlling this device from the GUIs |
+------------------+-------------+------------+-----------------------------------------------+
| confirm          | 0           | 1 or 0     | Ask for confirmation when switching device    |
+------------------+-------------+------------+-----------------------------------------------+

.. versionadded:: 8.0.6

.. note::

   The minimum and maximum is dependent on the ``Device Settings`` below.
   The actual dimlevel is not validated against these ranges.

:underline:`GUI Settings`

+------------------+-------------+------------+------------------+
| **Setting**      | **Default** | **Format** | **Description**  |
+------------------+-------------+------------+------------------+
| dimlevel-minimum | 0           | number     | Minimum dimlevel |
+------------------+-------------+------------+------------------+
| dimlevel-maximum | 15          | number     | Maximum dimlevel |
+------------------+-------------+------------+------------------+

.. deprecated:: 8.0.5

.. note::

   The minimum and maximum is dependent on the ``Device Settings`` below.

:underline:`Device Settings`

+------------------+-------------+------------+------------------+
| **Setting**      | **Default** | **Format** | **Description**  |
+------------------+-------------+------------+------------------+
| dimlevel-minimum | 0           | number     | Minimum dimlevel |
+------------------+-------------+------------+------------------+
| dimlevel-maximum | 15          | number     | Maximum dimlevel |
+------------------+-------------+------------+------------------+
