let meta = require('./defaults.json');
try {
  meta_overload = require('./meta.json');
  meta = { ...meta, ...meta_overload };
} catch (e) {
  console.warn('meta.json not found or not valid. Using default configuration.');
}



const fs = require('fs');
const path = require('path');
const commonConfigPath = path.resolve(__dirname, 'public/commonConfig.js');
if (!fs.existsSync(commonConfigPath)) {
  throw new Error(`commonConfig.js not found in ${commonConfigPath}.\nPlease download from https://www.theengs.io/commonConfig.js or create this file before you build the documentation.`);
}
const commonConfig = require('./public/commonConfig');

module.exports = {
  ...commonConfig,
  title: `${meta.title} - ${meta.version}`,
  base: meta.url_prefix,
  dest: meta.dest, // default is generated/site
  description: 'One gateway, many technologies: MQTT gateway for ESP8266 or ESP32 with bidirectional 433mhz/315mhz/868mhz, Infrared communications, BLE, LoRa, beacons detection, mi flora / mi jia / LYWSD02/ Mi Scale compatibility, SMS & LORA.',
  head: [...commonConfig.head],
  themeConfig: {
    ...commonConfig.themeConfig,
    repo: meta.theme_config_repo,
    docsDir: 'docs',
    mode: meta.mode,
    sidebar: [
      ['/', '0 - What is it for 🏠'],
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
          'setitup/serial',
          'setitup/sensors',
          'setitup/actuators'
        ]
      },
      {
        title: '3 - Upload ➡️',   // required
        path: '/upload/',
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          ['upload/web-install', "(Option 1) Upload from the web"],
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
          'use/serial',
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
        path: '/participate/',
        sidebarDepth: 1,    // optional, defaults to 1
        children: [
          'participate/quick_start',
          'participate/support',
          'participate/development',
          'participate/adding-protocols',
          'participate/community',
          [meta.url_license_file, 'License']
        ]
      }
    ]
  },
  plugins: {
    'sitemap': {
      hostname: meta.hostname,
      urls: [
        'https://decoder.theengs.io/devices/devices.html',
        meta.url_community_forum,
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
