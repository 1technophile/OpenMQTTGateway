.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Sendmail
========

.. rubric:: Description

Send a message to any e-mail address

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| TO       | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| SUBJECT  | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| MESSAGE  | |yes|            | |no|                |                                                   |
+----------+------------------+---------------------+---------------------------------------------------+

.. note::

   The ``sendmail`` action requires some additional settings in the pilight configuration:

   .. code-block:: json

      {
        "settings": {
          "smtp-sender": "sender@domain.com",
          "smtp-host": "smtp.smtphost.com",
          "smtp-port": 465,
          "smtp-user": "smtpuser@domain.com",
          "smtp-password": "password"
        }
      }

.. rubric:: Examples

.. code-block:: console

   IF 1 == 1 THEN sendmail SUBJECT Doorbell rang MESSAGE Doorbell rang TO someone@somewhere.com

.. versionchanged:: 8.1.0

.. code-block:: console

   IF 1 == 1 THEN sendmail SUBJECT 'Doorbell rang' MESSAGE 'Doorbell rang' TO someone@somewhere.com