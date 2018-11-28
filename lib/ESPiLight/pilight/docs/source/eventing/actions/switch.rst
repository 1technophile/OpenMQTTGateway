.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Switch
======

.. rubric:: Description

Changes the state of a switch, relay or dimmer.

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| DEVICE   | |yes|            | |yes|               | Device(s) to change                               |
+----------+------------------+---------------------+---------------------------------------------------+
| TO       | |yes|            | |no|                | New state                                         |
+----------+------------------+---------------------+---------------------------------------------------+

.. versionadded:: 7.0

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| FOR      | |no|             | |no|                | | Determine how long this new state lasts         |
|          |                  |                     | | before we change back to the previous state     |
+----------+------------------+---------------------+---------------------------------------------------+
| AFTER    | |no|             | |no|                | After how long do we want the new state to be set |
+----------+------------------+---------------------+---------------------------------------------------+

.. note:: Units for ``FOR`` and ``AFTER``

   - MILLISECOND
   - SECOND
   - MINUTE
   - HOUR
   - DAY

.. rubric:: Examples

.. code-block:: console

   IF 1 == 1 THEN switch DEVICE light TO on
   IF 1 == 1 THEN switch DEVICE mainlight AND bookshelve AND hall TO on

.. versionadded:: 7.0

.. code-block:: console

   IF 1 == 1 THEN switch DEVICE light TO on FOR 10 MINUTE
   IF 1 == 1 THEN switch DEVICE light TO on AFTER 30 MILLISECOND
   IF 1 == 1 THEN switch DEVICE light TO off FOR 10 SECOND AFTER 30 MINUTE

.. versionchanged:: 8.1.0

.. code-block:: console

   IF 1 == 1 THEN switch DEVICE light TO on FOR '10 MINUTE'
   IF 1 == 1 THEN switch DEVICE light TO on AFTER '30 MILLISECOND'
   IF 1 == 1 THEN switch DEVICE light TO off FOR '10 SECOND' AFTER '30 MINUTE'