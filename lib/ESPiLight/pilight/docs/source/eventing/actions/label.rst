.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Label
=====

.. rubric:: Description

Changes the text and color of a generic label device.

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| DEVICE   | |yes|            | |yes|               | Device(s) to change                               |
+----------+------------------+---------------------+---------------------------------------------------+
| TO       | |yes|            | |no|                | New text                                          |
+----------+------------------+---------------------+---------------------------------------------------+
| COLOR    | |no|             | |no|                | New color. If omitted, color is set to black      |
+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| FOR      | |no|             | |no|                | | Determine how long this new label lasts         |
|          |                  |                     | | before we change back to the previous label     |
+----------+------------------+---------------------+---------------------------------------------------+
| AFTER    | |no|             | |no|                | After how long do we want the new label to be set |
+----------+------------------+---------------------+---------------------------------------------------+

.. note:: Units for ``FOR`` and ``AFTER``

   - MILLISECOND
   - SECOND
   - MINUTE
   - HOUR
   - DAY

.. rubric:: Examples

.. code-block:: console

   IF 1 == 1 THEN label DEVICE tempLabel TO on
   IF 1 == 1 THEN label DEVICE tempLabel AND humiLabel TO No information
   IF 1 == 1 THEN label DEVICE tempLabel TO 23.5 FOR 10 SECOND
   IF 1 == 1 THEN label DEVICE tempLabel TO Bell rang AFTER 30 SECOND
   IF 1 == 1 THEN label DEVICE tempLabel TO None FOR 10 MINUTE AFTER 30 SECOND

.. versionchanged:: 8.1.0.

.. code-block:: console

   IF 1 == 1 THEN label DEVICE tempLabel TO on
   IF 1 == 1 THEN label DEVICE tempLabel AND humiLabel TO 'No information'
   IF 1 == 1 THEN label DEVICE tempLabel TO 23.5 FOR '10 SECOND'
   IF 1 == 1 THEN label DEVICE tempLabel TO 'Bell rang' AFTER '30 SECOND'
   IF 1 == 1 THEN label DEVICE tempLabel TO None FOR '10 MINUTE' AFTER '30 SECOND'