// commonConfig.js
module.exports = {
  markdown: {
    lineNumbers: true
  },
  title: 'Theengs: BLE RF LoRa applications and MQTT gateways',
  description: 'Theengs: the open-source interoperability platform that bridges hundreds of sensors into one interface. Compatible with major IoT and home automation solutions like AWS, OpenHAB, Home Assistant, and Jeedom. It supports MQTT protocol, enabling efficient integration and automation.',
  head: [
    ['meta', { name: 'viewport', content: 'width=device-width, initial-scale=1' }],
    ['link', { rel: "apple-touch-icon", sizes: "180x180", href: ".apple-touch-icon.png" }],
    ['link', { rel: "icon", type: "image/png", sizes: "32x32", href: "/favicon-32x32.png" }],
    ['link', { rel: "icon", type: "image/png", sizes: "16x16", href: "/favicon-16x16.png" }],
    ['link', { rel: 'manifest', href: '/manifest.json' }],
    ['meta', { name: 'theme-color', content: '#3eaf7c' }],
    ['meta', { name: 'apple-mobile-web-app-capable', content: 'yes' }],
    ['meta', { name: 'apple-mobile-web-app-status-bar-style', content: 'black' }],
    ['link', { rel: 'mask-icon', href: '/icons/safari-pinned-tab.svg', color: '#3eaf7c' }],
    ['meta', { name: 'msapplication-TileImage', content: '/favicon-144x144.png' }],
    ['meta', { name: 'msapplication-TileColor', content: '#000000' }]
  ],
  themeConfig: {
    smoothScroll: true,
    search: false,
    docsDir: 'docs',
    docsBranch: 'development',
    lastUpdated: 'Last Updated',
    editLinks: true,
    nav: [
      { text: 'Home', link: 'https://www.theengs.io', target: '_self', rel: '' },
      {
        text: 'Use cases',
        items: [
          { text: 'Smart Home', link: 'https://www.theengs.io/usecases/smarthome.html', target: '_self', rel: '' },
          { text: 'IoT', link: 'https://www.theengs.io/usecases/iot.html', target: '_self', rel: '' },
          { text: 'Research', link: 'https://www.theengs.io/usecases/research.html', target: '_self', rel: ''  }
        ]
      },
      { 
        text: 'Solutions',
        items: [
          { 
            text: 'Hardware',
            items: [
              { text: 'Theengs Plug', link: 'https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption', target: '_self', rel: '' },
              { text: 'Theengs Bridge', link: 'https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna', target: '_self', rel: '' },
            ]
          },
          {
            text: 'Software',
            items: [
              { text: 'Mobile App', link: 'https://app.theengs.io', target: '_self', rel: '' },
              { text: 'OpenMQTTGateway', link: 'https://docs.openmqttgateway.com', target: '_self', rel: '' },
              { text: 'Gateway', link: 'https://gateway.theengs.io', target: '_self', rel: '' },
            ]
          },
          {
            text: 'Core',
            items: [
              { text: 'Decoder', link: 'https://decoder.theengs.io', target: '_self', rel: '' },
              { text: 'Web Parser', link: 'https://parser.theengs.io', target: '_self', rel: '' }
            ]
          }
        ]
      },
      { text: 'Compatible devices', link: 'https://decoder.theengs.io/devices/devices.html', target: '_self', rel: '' },
      { text: 'Community', link: 'https://community.openmqttgateway.com', target: '_self', rel: '' },
      { text: 'Shop', link: 'https://shop.theengs.io/', target: '_self', rel: '' },
      { text: 'Sponsor 🤍', link: 'https://github.com/sponsors/theengs', target: '_self', rel: '' }
    ],
  },
  plugins: [
    ['@vuepress/pwa', {
      serviceWorker: true,
      updatePopup: true
    }],
    ['@vuepress/medium-zoom', true],
    ['@vuepress/nprogress']
  ]
};
