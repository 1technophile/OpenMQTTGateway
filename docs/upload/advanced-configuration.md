# Advanced configuration of the gateway

## Secure connection to the broker
By default the connection between the gateway and the MQTT broker is not encrypted and the identity of the broker is not verified.
For connections in a private local area network this might not be a big issue, but if you connect to a broker on the internet or the gateway is connected to a public network there is a high security risk.

To secure the connection, Transport Layer Security (TLS) can be used which is supported by most MQTT brokers.
Setting up and configuring TLS in the MQTT broker is a complex process and perhaps include creating a self-signed certificate.
The configuration of the broker is not covered here, you should look into the documentation of your broker.

### Prerequisites
The MQTT broker is configured for TLS and you have access to the CA certificate which was used to sign the MQTT broker certificate.
You are using ESP8266 or ESP32, for other boards TLS is not supported.

### Configure secure connection in the gateway
To enable the secure connection and use TLS uncomment `//#define SECURE_CONNECTION` in `User_config.h`.
Set `MQTT_SERVER` to the Common Name (CN) of the certificate of the broker.
This can be the hostname or the ip of the broker.

The CA certificate should be in PEM ascii format.
If your CA certificate is not in the correct format or you don't know the format, use `openssl` to convert the certificate to the correct format.
In `User_config.h` replace the `...` with the content of your certificate which is between the `-----BEGIN CERTIFICATE-----` and `-----END CERTIFICATE-----` lines:
```cpp
const char* certificate CERT_ATTRIBUTE = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";
```

If you have no ntp server in your local network (included in the router) or not using dhcp, you should uncomment `//#    define NTP_SERVER "pool.ntp.org"` to use a ntp server for time synchronization.
This is related to the `W: failed, ssl error code=54` error message, which indicate that the time of the esp is not correct.

You can know compile and upload to your board and the gateway should connect with TLS to your broker.
