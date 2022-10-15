# Integrate AWS IOT
## Create a thing

* From AWS console, search for IOT core
* Create a Thing and name it, this name will be used as the OpenMQTTGateway name.

![](../img/Integrate_AWS_IOT_Create_Thing.png)

![](../img/Integrate_AWS_IOT_Create_Thing2.png)

![](../img/Integrate_AWS_IOT_Create_Thing3.png)

![](../img/Integrate_AWS_IOT_Create_Thing4.png)

## Create a policy

![](../img/Integrate_AWS_IOT_Create_Policy.png)

![](../img/Integrate_AWS_IOT_Create_Policy2.png)

![](../img/Integrate_AWS_IOT_Create_Policy3.png)

* Add this json code to the policy
```json
{
  "Version": "2021-11-01",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect",
        "iot:Publish",
        "iot:Subscribe",
        "iot:Receive",
        "greengrass:Discover"
      ],
      "Resource": [
        "*"
      ]
    }
  ]
}
```

## Create a certificate

* Create a new certificate

![](../img/Integrate_AWS_IOT_Create_Certificate.png)
![](../img/Integrate_AWS_IOT_Create_Certificate2.png)

* Download certificates and key

![](../img/Integrate_AWS_IOT_Create_Certificate3.png)

## Attach Policy with certificate

![](../img/Integrate_AWS_IOT_Attach_Policy.png)
![](../img/Integrate_AWS_IOT_Attach_Policy2.png)

## Activate certificate and attach it to Thing

![](../img/Integrate_AWS_IOT_Attach_Thing.png)
![](../img/Integrate_AWS_IOT_Attach_Thing2.png)

## Find AWS EndPoint

![](../img/Integrate_AWS_IOT_Find_Endpoint.png)

## Gather the information for OpenMQTTGateway configuration

Now you should have the following information for the OpenMQTTGateway configuration:
* Root certificate
* Client certificate
* Client key
* End point url
* Gateway name

## Modify OpenMQTTGateway configuration

With Arduino IDE, you can update the following field into User_config.h with the information gathered:
* Gateway_Name
* `MQTT_SERVER "xxxxxx.iot-eu-amazonaws.com"`
* `MQTT_PORT "8883"`
* `AWS_IOT true`
* ss_server_cert with the root certificate
* ss_client_cert with the client certificate
* ss_client_key with the Client key
* `MQTT_SECURE_SELF_SIGNED 1`
* `MQTT_SECURE_DEFAULT true`

With PlatformIO you can directly leverage the environment `esp32dev-ble-aws`

## Build and upload

## Connect to a wifi Access point [see portal](../upload/portal)
Enter your credentials and verify that Secure connection is marked.

## Verify that you receive data in AWS

* Go to things
* Click on the thing created
* Go to Activity
* Click on MQTT Test Client
* Subscribe to `+/#`

You should see messages coming.

![](../img/Integrate_AWS_IOT_result_MQTT.png)