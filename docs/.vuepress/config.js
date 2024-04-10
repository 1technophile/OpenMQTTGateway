const commonConfig = require('./public/commonConfig');

module.exports = {
  ...commonConfig,
  title: 'Theengs OpenMQTTGateway version_tag',
  base: '/',
  description: 'One gateway, many technologies: MQTT gateway for ESP8266 or ESP32 with bidirectional 433mhz/315mhz/868mhz, Infrared communications, BLE, LoRa, beacons detection, mi flora / mi jia / LYWSD02/ Mi Scale compatibility, SMS & LORA.',
  head: [
    ...commonConfig.head,
    ['script', {type: 'module', src: 'https://unpkg.com/esp-web-tools@9.4.3/dist/web/install-button.js?module'}]
  ],
  themeConfig: {
    repo: '1technophile/OpenMQTTGateway',
    docsDir: 'docs',
    ...commonConfig.themeConfig,
    sidebar: [
      ['/','0 - What is it for 🏠'],
      {
        title: '1 - Prerequisites🧭',   // required
        //collapsable: true, // optional, defaults to true
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'prerequisites/devices',
          'prerequisites/board',
          'prerequisites/parts',
          'prerequisites/broker',
          'prerequisites/controller']
      },
      {
        title: '2 - Set it up 🔨',   // required
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'setitup/rf',
          'setitup/ble',
          'setitup/ir',
          'setitup/lora',
          'setitup/gsm',
          'setitup/rs232',
          'setitup/sensors',
          'setitup/actuators'
        ]
      },
      {
        title: '3 - Upload ➡️',   // required
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'upload/web-install',
          'upload/binaries',
          'upload/builds',
          'upload/gitpod',
          'upload/portal',
          'upload/advanced-configuration',
          'upload/troubleshoot'
        ]
      },
      {
        title: '4 - Use ✈️',   // required
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'use/rf',
          'use/ble',
          'use/ir',
          'use/lora',
          'use/gsm',
          'use/rs232',
          'use/rfm69',
          'use/sensors',
          'use/actuators',
          'use/boards',
          'use/displays',
          'use/gateway',
          'use/webui'
        ]
      },
      {
        title: '5 - Integrate 🎉',   // required
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'integrate/openhab2',
          'integrate/openhab3',
          'integrate/home_assistant',
          'integrate/node_red',
          'integrate/aws_iot',
          'integrate/jeedom'
        ]
      },
      {
        title: '6 - Participate 💻',   // required
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'participate/support',
          'participate/development',
          'participate/adding-protocols',
          'participate/community',
          ['https://github.com/1technophile/OpenMQTTGateway/blob/development/LICENSE.txt','License']
        ]
      }
  ]
  },
  plugins: {
    'sitemap': {
      hostname: 'https://docs.openmqttgateway.com',
      urls: [
        'https://decoder.theengs.io/devices/devices.html',
        'https://community.openmqttgateway.com/',
        'https://shop.theengs.io/',
        'https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption',
        'https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna',
        'https://app.theengs.io/',
        'https://gateway.theengs.io/',
        'https://decoder.theengs.io/',
        'https://parser.theengs.io/',
        'https://www.theengs.io/'
      ],
    },
  }
}
