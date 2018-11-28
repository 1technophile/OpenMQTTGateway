Hardware
========

- `Disabled`_
- `433.92Mhz`_
   - `433lirc`_
   - `433gpio`_
   - `433nano`_

Introduction
------------

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.

Since pilight 3.0 the feature to send and receive from multiple hardware modules has been introduced. This means you can control various different frequencies at the same time. However, only one module per frequency is supported. Each hardware module has its own syntax as listed below.

Disabled
--------

.. code-block:: json
   :linenos:

   {
     "hardware": {
       "none": { }
     }
   }

433.92Mhz
---------

.. _433lirc:
.. rubric:: Lirc Kernel Module

.. code-block:: json
   :linenos:

   {
     "hardware": {
       "433lirc": {
         "socket": "/dev/lirc0"
       }
     }
   }

.. _433gpio:
.. rubric:: Direct GPIO Access

.. code-block:: json
   :linenos:

   {
     "hardware": {
       "433gpio": {
         "sender": 0,
         "receiver": 1
       }
     }
   }

The default configuration to be used with the pilight PCB. When using custom wiring, refer to http://www.wiringx.org for the pin numbering of the various supported devices. If you want to disable the sender or receiver pin, set it to
-1.

.. versionchanged:: 8.0

You must now specify which GPIO platform pilight is running on. Refer to the settings page for more information.

.. _433nano:
.. rubric:: pilight USB Nano

.. code-block:: json
   :linenos:

   {
     "hardware": {
       "433nano": {
         "comport": "/dev/ttyUSB0"
       }
     }
   }

The comport value needs to correspond to a valid COM device on your platform. On Windows this value is generally formatted as COM1, on Linux as /dev/ttyUSB0, and on FreeBSD /dev/cuau0.

