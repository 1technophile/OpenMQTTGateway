.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Ping
====

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
       "ping": {
         "protocol": [ "ping" ],
         "id": [{
           "ip": "x.x.x.x"
         }],
         "state": "connected"
       }
     }
   }

+------------------+--------------------------+
| **Option**       | **Value**                |
+------------------+--------------------------+
| ip               | *valid ip address*       |
+------------------+--------------------------+
| state            | connected / disconnected |
+------------------+--------------------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+--------------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                                  |
+--------------------+-------------+------------+--------------------------------------------------+
| poll-interval      | 1           | >= 1       | How often do we want to ping the ip (in seconds) |
+--------------------+-------------+------------+--------------------------------------------------+

.. rubric::: Comment


The ping protocol uses the ICMP ECHO request command and reports the state DISCONNECTED if it receives a response that it is not an ECHO REPLY.

The ping protocol doesn't have a GUI representation. You can of course use the ping protocol in your rules.

.. rubric:: Status Replies

| 3 - Destination Unreachable
| 4 - Source Quench
| 5 - Redirect (change route)
| 11 - Time Exceeded
| 12 - Parameter Problem
| 13 - Timestamp Request
| 14 - Timestamp Reply
| 15 - Information Request
| 16 - Information Reply
| 17 - Address Mask Request
| 18 - Address Mask Reply