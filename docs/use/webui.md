# WebUI

For ESP32 based environments a WebUI is available to provide basic configuration and monitoring of your OpenMQTTGateway Device.  Functions included are:

* Configuration
* Information
* Firmware Upgrade
* Console
* Resart

# Login Authentication

By default access to the WebUI uses basic authentication to control access to your OpenMQTTGateway Device.  The login is `admin` and the password is your ota_password.

# Configuration Options

## Wifi

Abiltity to change the SSID and password for your Wifi, if the change is unsuccessful it will revert back to the previous wifi settings.

## MQTT

Abiltity to change the mqtt settings, if the change is unsuccessful it will revert back to the previous mqtt settings.

## WebUI

Ability to change the display of sensor to Metric or Imperial, and disable the WebUI Authentication

## Logging

Ability to temporarily change the logging level.

# Information

Details of OpenMQTTGateway Device status

# Firmware Upgrade

Ability to upgrade firmware by URL or to latest version.

# Console

Ability to view messages from the OpenMQTTGateway console.  The scope of messages visible in the UI is limited to just the OpenMQTTGatewy codebase, messages from the ESP hardware or other libraries are not visible,

Ability to inject commands to OpenMQTTGateway for processing.  The commands accepted are of the form mqtt topic then json message.  And as you are already on the target device, you do not need to include the device name ie

`commands/MQTTtoSYS/config {"cmd":"restart"}`

This works for all modules in your environment.