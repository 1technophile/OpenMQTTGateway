.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Pushbullet
==========

.. rubric:: Description

Send a message to your mobile phone with the pushbullet API

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| TITLE    | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| BODY     | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| TOKEN    | |yes|            | |no|                | Unique pushbullet user account token              |
+----------+------------------+---------------------+---------------------------------------------------+
| TYPE     | |yes|            | |no|                | Message type                                      |
+----------+------------------+---------------------+---------------------------------------------------+

.. rubric:: Example

.. code-block:: console

   IF 1 == 1 THEN pushbullet TITLE Doorbell rang BODY Doorbell rang TOKEN abcd123abc TYPE note

.. versionchanged:: 8.1.0

.. code-block:: console

   IF 1 == 1 THEN pushbullet TITLE 'Doorbell rang' BODY 'Doorbell rang' TOKEN abcd123abc TYPE note