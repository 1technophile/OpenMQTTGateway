.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Lirc
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
     "ab": {
       "protocol": [ "lirc" ],
       "id": [{
         "remote": "logitech"
       }],
       "code": "000000000000e204",
       "repeat": 1,
       "button": "KEY_ARROWDOWN"
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| remote           | *any value*     |
+------------------+-----------------+
| code             | *any value*     |
+------------------+-----------------+
| repeat           | 1 - 999         |
+------------------+-----------------+
| button           | *any value*     |
+------------------+-----------------+

.. rubric:: Comments

The Lirc protocol tries to automatically connect to Lirc when pilight starts. When a connection can be made, it will send the same output as shown with ``irw`` in a pilight format.

.. code-block:: console

   root@pi:~# irw
   000000037ff07be0 00 KEY_ARROWDOWN logitech-harmony-300i

.. code-block:: console

   root@pi:~# pilight-receive
   {
     "code": {
       "id": "000000000000e204",
       "repeat": "01",
       "button": "KEY_ARROWDOWN",
       "remote": "logitech"
     },
     "origin": "receiver",
     "protocol": "lirc"
   }

Whenever, a connection to Lirc is lost, pilight will try to automatically reconnect.

All signal processing will be done by Lirc, so make sure you have a working Lirc configuration. Tutorials on this can be found all around the internet.

pilight tries to connect to the lirc socket found in ``/dev/lircd`` so make sure it exists or a symlink to the original socket is created to ``/dev/lircd``.

**Raspbian Kernels 3.18 and higher**

The most recent Raspbian release, with Pi 2 support, switches to a new kernel (3.18), and includes a configuration change to enable Device Tree support by default.

You do need to add ``dtoverlay=lirc-rpi`` to ``/boot/config.txt``.

Depending on your hardware setup, ``lirc-rpi`` module parameters are added to the end of the dtoverlay line: ``gpio_in_pin=16,gpio_in_pull=high,gpio_out_pin=17``

Example:

.. code-block:: console

   dtoverlay=lirc-rpi,gpio_in_pin=18,gpio_out_pin=17