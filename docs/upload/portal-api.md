# Configuration portal HTTP API

This page documents the HTTP form fields accepted by the on-device WiFi configuration portal so that companion apps, provisioning scripts, and Home Assistant flows can integrate without reading firmware source.

The portal is implemented on top of [h2zero's WiFiManager fork](https://github.com/h2zero/WiFiManager) and the field declarations live in `main/main.cpp` (see `setup_wifimanager`).

## Endpoint

```
POST http://<portal-ip>/wifisave
Content-Type: application/x-www-form-urlencoded
```

When the portal is active (no saved credentials, or trigger pressed at boot) the device exposes a soft-AP — see [Portal](portal.md) for SSID/password rules.

## Form fields

| Field        | Type             | Source declaration              | Persisted? |
|--------------|------------------|---------------------------------|------------|
| `s`          | SSID             | WiFiManager built-in            | yes        |
| `p`          | WiFi PSK         | WiFiManager built-in            | yes        |
| `server`     | MQTT host (≤ 64) | `custom_mqtt_server`            | yes, if non-empty |
| `port`       | MQTT port        | `custom_mqtt_port`              | yes, if non-empty |
| `user`       | MQTT username    | `custom_mqtt_user`              | yes, if non-empty |
| `pass`       | MQTT password    | `custom_mqtt_pass`              | yes, if non-empty *and* value differs from the compile-time `MQTT_PASS` sentinel |
| `secure`     | `0` / `1`        | `custom_mqtt_secure`            | yes (always) |
| `validate`   | `0` / `1`        | `custom_validate_cert`          | yes (always) |
| `cert`       | PEM (≤ 4096 B)   | `custom_mqtt_cert`              | yes, if length > `MIN_CERT_LENGTH` |
| `ota_cert`   | PEM (≤ 4096 B)   | `custom_ota_server_cert`        | yes, if length > `MIN_CERT_LENGTH` |
| `client_cert`| PEM              | `custom_client_cert` (signed-client builds only) | yes, if length > `MIN_CERT_LENGTH` |
| `client_key` | PEM              | `custom_client_key`  (signed-client builds only) | yes, if length > `MIN_CERT_LENGTH` |
| `topic`      | MQTT base topic  | `custom_mqtt_topic`             | yes, if non-empty (trailing `/` added if missing) |
| `name`       | Gateway name     | `custom_gateway_name`           | yes, if non-empty |
| `ota`        | OTA / WebUI password | `custom_ota_pass`           | yes, if non-empty |

## Empty-field semantics

Empty `server` / `port` / `user` / `topic` / `name` / `ota` values are **ignored** rather than persisted. This means a partial POST (e.g. WiFi-only) preserves any pre-flashed defaults instead of clearing them. To intentionally clear a field, this portal API does not currently support it — re-flash with new build defaults or use the WebUI.

The `pass` field combines the empty-check with a sentinel guard: the firmware persists the form value only when it is non-empty *and* differs from the compile-time `MQTT_PASS` sentinel. The sentinel branch prevents an unmodified form (which echoes back the default) from overwriting a stored password; the non-empty branch prevents a partial POST from wiping it.

## Response and AP teardown

After processing the POST, WiFiManager returns a small success page and then tears down the soft-AP to attempt joining the configured station network. The teardown happens quickly enough that the TCP socket may be reset before the HTTP response is fully flushed to the client. Companion apps should:

- Treat a connection reset immediately after the POST as a likely-success signal, not a failure.
- Verify provisioning by checking for the device on the target network or for its first MQTT LWT message rather than relying on the HTTP response.

## Example

```bash
curl -X POST \
  --data-urlencode 's=my-ssid' \
  --data-urlencode 'p=my-psk' \
  --data-urlencode 'server=192.168.1.10' \
  --data-urlencode 'port=1883' \
  --data-urlencode 'name=kitchen-omg' \
  --data-urlencode 'ota=mySecret123' \
  --data-urlencode 'topic=home/' \
  http://192.168.4.1/wifisave
```
