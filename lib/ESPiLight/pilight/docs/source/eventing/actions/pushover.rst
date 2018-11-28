.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Pushover
========

.. rubric:: Description

Send a message to your mobile phone with the pushover API

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| TITLE    | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| MESSAGE  | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| TOKEN    | |yes|            | |no|                | Unique pushover user account token                |
+----------+------------------+---------------------+---------------------------------------------------+
| USER     | |yes|            | |no|                | Unique pushover username                          |
+----------+------------------+---------------------+---------------------------------------------------+

.. rubric:: Example

.. code-block:: console

   IF 1 == 1 THEN pushover TITLE Doorbell rang MESSAGE Doorbell rang TOKEN abcd123abc USER pilight

.. versionchanged:: 8.1.0

.. code-block:: console

   IF 1 == 1 THEN pushover TITLE 'Doorbell rang' MESSAGE 'Doorbell rang' TOKEN abcd123abc USER pilight
