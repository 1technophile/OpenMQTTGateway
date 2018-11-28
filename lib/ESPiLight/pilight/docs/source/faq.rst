Frequently Asked Questions
==========================

- `General`_
   - `Retrieving pilight version`_
   - `pilight doesn't start or immediately exits`_
   - `Fixing SSDP connection issues`_
   - `Disabling SSDP completely`_
   - `Using other devices besides 433.92 MHz`_
   - `Fixing broken Lirc and 1-wire protocols`_
   - `Fixing the high CPU usage after a Kernel upgrade`_
- `Sending & Receiving`_
   - `The GPIO connected receiver does not work`_
   - `I only see sent codes, not received ones`_
   - `Let devices learn new codes sent from pilight`_
- `Configuration`_
   - `How to use raw codes in the configuration`_
   - `How to fix an empty webGUI`_

.. deprecated:: 8.0

- `General`_
   - `Using PHP inside the pilight webserver`_
- `Sending & Receiving`_
   - `The send-repeat setting does not work anymore`_

General
-------

.. _Retrieving pilight version:
.. rubric:: Retrieving pilight version

You can use the following command to retrieve the pilight version:

.. code-block:: console

   pi@pilight:~# pilight-daemon -V

If you run a stable version, only the version will be shown:

.. code-block:: console

   pilight-daemon version v7.0

If you run a developmental version, a commit message will also be shown. It will point to the last commit included in your pilight compilation.

.. code-block:: console

   pilight-daemon version v7.0-24-gb64a135

In this case, the 24th commit after version 7.0 with the SHA id gb64a135 is running. To see which commit this is, check the `git development commit list <https://github.com/pilight/pilight/commits/development>`_.

.. _pilight doesn't start or immediately exits:
.. rubric:: pilight doesn't start or immediately exits

Most probably you made an error in the pilight configuration. Start pilight in debug mode and check the output:

.. code-block:: console

   pi@pilight:~# pilight-daemon -D

.. _Fixing SSDP connection issues:
.. rubric:: Fixing SSDP connection issues

Make sure your iptables are set correctly:

.. code-block:: console

   pi@pilight:~# sudo iptables -A INPUT -s 127.0.0.1 -j ACCEPT

Also make sure your network interfaces are configured correctly in **/etc/network/interfaces**:

.. code-block:: guess
   :linenos:

   auto lo
   iface lo inet loopback
   allow-hotplug eth0
   auto eth0
   iface eth0 inet static
   address x.x.x.x
   netmask 255.255.255.0
   gateway x.x.x.x

Especially the **iface lo inet loopback** part is essential for SSDP to work. A reboot could be necessary after making these changes.

.. _Disabling SSDP completely:
.. rubric:: Disabling SSDP completely

Add the standalone setting in the config.json and set it to 1.

.. code-block:: json
   :linenos:

   {
      "settings": {
         "standalone": 1
      }
   }

However, because all pilight clients use SSDP to find the main pilight daemon, you need to pass the server and port arguments when you want to control this standalone running daemon. Check the documentation for the specific pilight client for additonal information.

.. _Using PHP inside the pilight webserver:
.. rubric:: Using PHP inside the pilight webserver

.. deprecated:: 8.0

You probably encounter this message when running pilight in debug mode:

.. code-block:: guess
   :linenos:

   pilight-daemon: ERROR: php support disabled due to missing php-cgi executable

This means that you miss some packages to run PHP. The required packages pilight needs for PHP support are:

- php-cgi
- base64
- cat

.. _Using other devices besides 433.92 MHz:
.. rubric:: Using other devices besides 433.92 MHz

pilight was built with 433.92 MHz devices as a reference, but the code is not limited to this frequency. We actually always wrote the code with other frequencies in mind. The only reason no other frequencies like 868 MHz are supported is lack of time and the lack of other developers to do it for us. The hardware part of pilight is completely modular so adding support for other frequencies should be as easy as writing new protocols.

.. _Fixing broken Lirc and 1-wire protocols:
.. rubric:: Fixing broken Lirc and 1-wire protocols

You probably installed the latest Raspberry Pi kernel. The new kernel works with device trees so the kernel knows what devices you want to use. Check the Raspberry Pi documentation how to use this new device tree.

.. _Fixing the high CPU usage after a Kernel upgrade:
.. rubric:: Fixing the high CPU usage after a Kernel upgrade

The wiringPi GPIO library used in pilight version 5.0 and lower contained a bug. This is fixed in pilight version 6 and up.

Version 8 and up use the wiringX library also written by the pilight developers which also fixes the (old) wiringPi bugs.

Sending & Receiving
-------------------

.. _The GPIO connected receiver does not work:
.. rubric:: The GPIO connected receiver does not work

The most encountered reason for this problem is the quality of the receiver. A lot of users buy unsupported (cheap) receivers from either eBay, DealExtreme or similar sites often referred to as FS1000A and XY-MK-5V. However, these receivers have a terrible range. To make sure it is the receiver and not a fault in connecting the device to your Raspberry Pi, make sure to keep your remote next to the receiver. If it still fails, check then check your connections.

If you do want to use pilight for controlling devices across your house, consider buying a good quality receiver. Refer to the pilight shop for supported peripherals.

.. _I only see sent codes, not received ones:
.. rubric:: I only see sent codes, not received ones

The pilight receive output always contains an origin value. This means you can see from were the outputted code came from. Only if this field says receiver you know that the code was picked up by the receiver. When it says sender the codes has been created and processed internally. pilight processes these codes as if it was a received code so it can update the GUIs and config. Only if you see receiver you know it was not generated by pilight.

.. _Let devices learn new codes sent from pilight:
.. rubric:: Let devices learn new codes sent from pilight

Some protocols support learning devices. This learn feature temporarily sends an increased amount of codes to the device. Check the protocol send arguments to see if your protocol supports it. For example, the KlikAanKlikUit protocol does this as follows:

.. code-block:: console

   pi@pilight:~# pilight-send -p kaku_switch -i 1 -u 1 -t -l

.. _The send-repeat setting does not work anymore:
.. rubric:: The send-repeat setting does not work anymore

.. deprecated:: 6.0

pilight version 6 was the last version supporting the global send-repeat setting. This setting told pilight how often a pulsetrain was repeated. This setting got removed because it interfered with a lot of protocols.

Most remote control devices repeat a pulsetrain two to six times on a single button press. On some devices all pulsetrains are identical, so repeating them does not introduce any issues. However, on some devices the 1st pulsetrain differs from the subsequent pulsetrains. In addition, some devices sent a wakeup pulse sequence before the very 1st pulsetrain to trigger internal wakeup logic. Most devices use footer pulses, while devices transmit header pulses, and some devices transmit both.
If you keep the button pressed on some remote controls, a series of pulsetrains is sent until the button is released, while others stop sending repetitive pulsetrains after a certain time period, and some set a toggle bit for repetitive pulsetrains each time a button is pressed.

pilight was not differentiating between those various operating scenarios, because the send-repeat parameter specified only how often a single pulsetrain was re-transmitted for all devices. So increasing the global send-repeats actually broke a lot of these protocols. We therefore removed the old global send-repeat parameter and replaced it with a protocol specific repeat parameter, currently not configurable from userspace.
We also discovered that in almost all cases, the solution was not increasing the send-repeats parameter, but instead using a good antenna.

Configuration
-------------

.. _How to use raw codes in the configuration:
.. rubric:: How to use raw codes in the configuration

This is not possible, because pilight cannot know what these codes mean and how to interpret them.

.. _How to fix an empty webGUI:
.. rubric:: How to fix an empty webGUI

You need to add devices to the "GUI" section of config.json as well, not just the "devices" section.
