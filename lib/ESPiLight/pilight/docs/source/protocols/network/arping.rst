.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Arping
======

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
       "arping": {
         "protocol": [ "arping" ],
         "id": [{
           "mac": "xx:xx:xx:xx:xx:xx"
         }],
         "ip": "0.0.0.0",
         "state": "connected"
       }
     }
   }

+------------------+--------------------------+
| **Option**       | **Value**                |
+------------------+--------------------------+
| mac              | *valid mac address*      |
+------------------+--------------------------+
| ip               | *valid ip address*       |
+------------------+--------------------------+
| state            | connected / disconnected |
+------------------+--------------------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+-------------------------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                                             |
+--------------------+-------------+------------+-------------------------------------------------------------+
| poll-interval      | 1           | >= 1       | How often do we want to arping the mac address (in seconds) |
+--------------------+-------------+------------+-------------------------------------------------------------+

.. rubric::: Comment


The arping protocol doesn't have a GUI representation. You can of course use the arping protocol in your rules.

The ping protocol doesn't have a GUI representation. You can of course use the ping protocol in your rules.

.. rubric:: Note

The MAC address must be written in lowercase.