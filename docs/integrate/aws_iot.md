# Integrate AWS IOT
## Create a thing

* From AWS console, search for IOT core
* Create a Thing and name it, this name will be used as the OpenMQTTGateway name.

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Thing.png)

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Thing2.png)

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Thing3.png)

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Thing4.png)

## Create a policy

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Policy.png)

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Policy2.png)

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Policy3.png)

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

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Certificate.png)
![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Certificate2.png)

* Download certificates and key

![AWS tutorial step](../img/Integrate_AWS_IOT_Create_Certificate3.png)

## Attach Policy with certificate

![AWS tutorial step](../img/Integrate_AWS_IOT_Attach_Policy.png)
![AWS tutorial step](../img/Integrate_AWS_IOT_Attach_Policy2.png)

## Activate certificate and attach it to Thing

![AWS tutorial step](../img/Integrate_AWS_IOT_Attach_Thing.png)
![AWS tutorial step](../img/Integrate_AWS_IOT_Attach_Thing2.png)

## Find AWS EndPoint

![AWS tutorial step](../img/Integrate_AWS_IOT_Find_Endpoint.png)

## Gather the information for OpenMQTTGateway configuration

Now you should have the following information for the OpenMQTTGateway configuration:
* Root certificate
* Client certificate
* Client key
* End point url
* Gateway name

## Modify OpenMQTTGateway configuration

With PlatformIO you can directly leverage the environment `esp32dev-ble-aws` as an example and modify it following your needs.

## Build and upload

## Connect to a WiFi Access point [see portal](../upload/portal)
Enter your credentials and verify that Secure connection is marked.

## Verify that you receive data in AWS

* Go to things
* Click on the thing created
* Go to Activity
* Click on MQTT Test Client
* Subscribe to `+/#`

You should see messages coming.

![AWS tutorial step](../img/Integrate_AWS_IOT_result_MQTT.png)