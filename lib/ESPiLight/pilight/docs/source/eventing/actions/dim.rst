.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Dim
===

.. rubric:: Description

Dims a device to a dimlevel or starts a sequence from one dimlevel to another.

.. rubric:: Options

+----------+------------------+---------------------+-------------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                       |
+----------+------------------+---------------------+-------------------------------------------------------+
| DEVICE   | |yes|            | |yes|               | Device(s) to change                                   |
+----------+------------------+---------------------+-------------------------------------------------------+
| TO       | |yes|            | |no|                | New dimlevel                                          |
+----------+------------------+---------------------+-------------------------------------------------------+

.. versionadded:: 7.0

+----------+------------------+---------------------+-------------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                       |
+----------+------------------+---------------------+-------------------------------------------------------+
| FOR      | |no|             | |no|                | | Determine how long this new state lasts before      |
|          |                  |                     | | we change back to the previous dimlevel and state   |
+----------+------------------+---------------------+-------------------------------------------------------+
| AFTER    | |no|             | |no|                | After how long do we want the new state to be set     |
+----------+------------------+---------------------+-------------------------------------------------------+
| IN       | |no|             | |no|                | | To be combined with FROM. In what time do we want   |
|          |                  |                     | | to change the dimlevel FROM x TO y                  |
+----------+------------------+---------------------+-------------------------------------------------------+
| FROM     | |no|             | |no|                | | To be combined with IN. From what dimlevel do we    |
|          |                  |                     | | want to start a sequence to the target dimlevel     |
+----------+------------------+---------------------+-------------------------------------------------------+

.. note:: Units for ``FOR``, ``AFTER``, and ``IN``

   - MILLISECOND
   - SECOND
   - MINUTE
   - HOUR
   - DAY

.. rubric:: Examples

.. code-block:: console

   IF 1 == 1 THEN dim DEVICE mainlight TO 15
   IF 1 == 1 THEN dim DEVICE mainlight AND bookshelve AND hall TO 10

.. versionadded:: 7.0

.. code-block:: console

   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FOR 10 MINUTE
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 AFTER 30 SECOND
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 AFTER 30 SECOND FOR 10 MINUTE
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FROM 0 IN 45 MINUTE
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FROM 0 IN 45 MINUTE FOR 15 MINUTE AFTER 5 MINUTE
   IF 1 == 1 THEN switch DEVICE light TO off FOR 10 SECOND AFTER 30 MINUTE

.. versionchanged:: 8.1.0

.. code-block:: console

   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FOR '10 MINUTE'
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 AFTER '30 SECOND'
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 AFTER '30 SECOND' FOR '10 MINUTE'
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FROM 0 IN '45 MINUTE'
   IF 1 == 1 THEN dim DEVICE mainlight TO 15 FROM 0 IN '45 MINUTE' FOR '15 MINUTE' AFTER '5 MINUTE'
   IF 1 == 1 THEN switch DEVICE light TO off FOR '10 SECOND' AFTER '30 MINUTE'