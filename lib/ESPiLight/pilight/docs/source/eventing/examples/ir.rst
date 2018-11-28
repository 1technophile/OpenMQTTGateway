.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

IR Remote
=========

.. rubric:: Description

Switch devices based on your IR remote buttons

.. rubric:: Devices

.. code-block:: json
   :linenos:

   {
     "devices": {
       "television": {
       "protocol": [ "kaku_switch" ],
       "id": [{
         "id": 123456,
         "unit": 0
       }],
       "state": "off"
     },
     "remote": {
       "protocol": [ "lirc" ],
       "id": [{
         "remote": "logitech"
       }],
       "code": "000000000000e290",
       "repeat": 2,
       "button": "KEY_P"
     }
   }

.. rubric:: Rule

.. code-block:: json
   :linenos:

   {
     "remote-television-switch": {
       "rule": "IF remote.button IS KEY_P AND remote.repeat == 0 THEN toggle DEVICE television BETWEEN on AND off",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

.. code-block:: json
   :linenos:

   {
     "remote-television-switch": {
       "rule": "IF remote.button == KEY_P AND remote.repeat == 0 THEN toggle DEVICE television BETWEEN on AND off",
       "active": 1
     }
   }