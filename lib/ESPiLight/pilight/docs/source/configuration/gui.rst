GUI
===

- `Introduction`_
- `Elements`_
- `Group`_

Introduction
------------

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.


pilight has an extensive API that allows developers to create custom graphical user interfaces (GUI). A pilight GUI is actually nothing more then a presentation of the devices a user wants to easily control. This means that a GUI does not have to show all devices added to the device set-up and that not all GUIs have to show the same devices. You can have a more compact GUI for a small screen such as a mobile phone or a more extensive GUI for you computer screen.

Elements
--------

Remember the device set-up we created earlier.

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

To present one of these two devices to our GUI, we add the to the GUI list like this:

.. code-block:: json
   :linenos:

   {
     "gui": {
       "television": {
         "name": "Television",
         "group": [ "Living" ],
         "media": [ "web" ],
         "readonly": 0
       },
       "bookShelfLight": {
         "name": "Book Shelve",
         "group": [ "Library" ],
         "media": [ "mobile" ],
         "readonly": 1
       }
     }
   }

Each device you want to add to the GUI has its own entry. You can only add devices that are also registered in the device set-up, but of course, you do not have to present all those devices to the GUIs. There are some fixed fields that are common for all GUI elements:

- name
   The display name inside the GUI.
- group
   See below.
- media
   Which GUIs should display this devices: mobile, web, desktop, all (default)
- readonly
   Should the GUI element be readonly? Default is 0.
- confirm
   Should the GUI ask for confirmation before switching the device? Default is 0.

To know what the additional options are for the specific protocols, check the protocol section of this manual.

Group
-----

The group attribute allows you to group GUI elements. pilight allows you to define multiple groups but it is dependent on the GUI how it will interpret these groups. Currently, the pilight webGUI and the Android App Illumina only parse the first group and will create different tabs for each one of them. However, possible implementation could be:

- Assign element to multiple groups
- Create a treeview of descending parents / children
- Create pages with grouped widgets