Devices
=======

* `Introduction`_
* `Structure`_
* `Device Types`_
* `Multiple IDs`_
* `Multiple Protocols Per Device`_
* `Overriding Protocol Settings`_

Introduction
------------

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.

.. note::

	This page will only explain the device set-up basics with some protocols as an example. To see the full
	list of supported protocols and their respective syntax read the protocol pages of this manual.

pilight can be ran with or without a fixed devices set-up. The advantage of a fixed device set-up is that pilight can use them in events and for external clients such as the built-in webGUI. Status updates will automatically be updated just like information from weather information.

The protocol names outputted by *pilight-receive* are not (always) the same as the protocols used in the device setup. The reason for this, is that various brands use the same underlying protocol. The arctech_switch protocol is used by at least five different brands. So there is the same underlying protocol for each of these brands.

#. Check the brand of the device you want to define in the device set-up.
#. Check if your device is recognize in *pilight-receive*.
#. Check the protocol pages to see if your device is listed and what the device syntax is.

If your specific brand / device is not listed, but *pilight-receive* does show output, then contact us so we can add the device to the specific protocol supported brand list.

Structure
---------

.. code-block:: json
   :linenos:

   {
     "devices": {
       "television": {
         "protocol": [ "relay" ],
         "id": [{
           "gpio": 3
         }],
         "state": "off"
       },
        "bookShelfLight": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456,
            "unit": 1
         }],
         "state": "off"
       }
     }
   }

Each device you want to set-up in pilight has its own entry. In this example we have set-up two devices. One GPIO connected Relay that turns the television on and off and KlikAanKlikUit controlled bookShelfLight. Whenever an action has been performed or received by pilight the registered device will update itself when necessary. So, would you in this example, turn the bookShelfLight to on with a KlikAanKlikUit remote, pilight will also record this, and change the status of the bookShelfLight to on. Of course, a receiver needs to be connected for this to work. Each device has its own specific syntax but its basic properties remain the same. That is:

#. name
#. protocol
#. id

To know the syntax of the specific protocol you want to use you can refer to the protocols description found in the protocol section of this manual.

.. note:: changed in version 8

   Protocol names cannot be used as device names.

Device Types
------------

pilight can work with various device types. Each device type has different functionality and requirements, however, they sometimes use the same code characteristics. One example are the KlikAanKlikUit switches, dimmers, doorbells, and screens. In pilight a KlikAanKlikUit code can show up like this:

.. code-block:: console

	{
		"origin": "sender",
		"protocol": "arctech_switches",
		"code": {
			"id": 100,
			"unit": 15,
			"state": off
		},
		"repeat": 1,
	}
	{
		"origin": "sender",
		"protocol": "arctech_dimmers",
		"code": {
			"id": 100,
			"unit": 15,
			"state": off
		},
		"repeat": 1,
	}
	{
		"origin": "sender",
		"protocol": "arctech_screens",
		"code": {
			"id": 100,
			"unit": 15,
			"state": up
		},
		"repeat": 1,
	}

As you can see, a single KlikAanKlikUit command was interpreted as three different devices. You need to choose carefully which device actually sent the code to make sure how to define it in the device set-up. The difference is that a dimmer will have a slider and an on/off button, a switch will just show an on/off button, and a screen will have momentary up/down buttons. Defining a screen as a dimmer is possible but does not give you the ability to control the dimmer as a dimmer from the different GUIs.

Multiple IDs
------------

Each protocol needs to have at least one id defined so pilight knows what device has been controlled. However, it is possible that you have multiple KlikAanKlikUit remotes that control the same KlikAanKlikUit switch. In that case, you can define multiple id's to your devices. In case of a KlikAanKlikUit switch:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "bookShelfLight": {
         "protocol": ["kaku_switch"],
         "id": [{
           "id": 1234,
           "unit": 0
         },
         {
           "id": 2345,
           "unit": 1
         }],
         "state": "off"
       }
     }
   }

Whenever one of these id's have been received, pilight will update the device accordingly.

Multiple Protocols Per Device
-----------------------------

pilight supports multiple protocols per device. The new KlikAanKlikUit switches are backwards compatible with the old KlikAanKlikUit remotes. This means that pilight needs to check both protocols to know whether a device state was changed. In case of a KlikAanKlikUit dimmer, pilight needs to check three protocols. To add multiple protocols per device, the device must contain at least one ID for each protocol and all protocol values should be present. An example:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "bookShelfLight": {
         "protocol": [ "kaku_dimmer", "kaku_switch", "kaku_old" ],
         "id": [{
           "id": 123456,
           "unit": 1
         },
         {
           "id": 10,
           "unit": 5
         }],
         "state": "off",
         "dimlevel": 10
       }
     }
   }

There are a few important steps when you use multiple protocols in a single device setup.

1. The kaku_dimmer and kaku_switch protocols both share the same id specifications, but the kaku_old protocol can only have an id < 16 and a unit < 33. The id set for the kaku_switch and kaku_dimmer is thereby not supported by the kaku_old protocol. Therefore an additional id must be added to match the requirements by kaku_old.

2. Because we have a dimmer and switch protocol combined we must have a dimlevel and state value present in the device.

3. The kaku_dimmer is the first protocol defined. This is important, because pilight will now interpret this device as a dimmer instead of a switch. Would the kaku_dimmer protocol be defined as second or third protocol, then the device would be interpreted as a switch.

Overriding Protocol Settings
----------------------------

Each protocol has some specific settings you can override in your device set-up. What these settings are, can be found in the protocols section of this manual. These settings can change the internal functioning of a protocol or the values a protocol can take. These settings are device specific.

For example, we do not want to have our dimmer to go to a full dimlevel, because then it is to bright. But we also do not want it to go to its minimum dimlevel, because then it is to dim. In that case, you can override the minimum and maximum values of the dimmer:

.. code-block:: json
   :linenos:

   {
     "devices": {
       "bookShelfLight": {
         "protocol": [ "kaku_dimmer" ],
         "id": [{
           "id": 1234,
           "unit": 1
         }],
         "state": "on",
         "dimlevel": 3,
         "dimlevel-minimum": 3,
         "dimlevel-maximum": 10
       }
     }
   }

Of course, the maximum dimlevels can still be overridden by the KlikAanKlikUit remote, but pilight will make sure it cannot control the dimmers below or above these dimlevels within pilight.