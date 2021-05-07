module.exports = {
    markdown: {
      lineNumbers: true
    },
    title: 'OpenMQTTGateway version_tag',
    description: 'One gateway, many technologies: MQTT gateway for ESP8266, ESP32, Sonoff RF Bridge or Arduino with bidirectional 433mhz/315mhz/868mhz, Infrared communications, BLE, beacons detection, mi flora / mi jia / LYWSD02/ Mi Scale compatibility, SMS & LORA.',
    head: [
      ['link', { rel: "icon", type: "image/png", sizes: "32x32", href: "/img/favicon-32x32.png"}],
      ['link', { rel: "icon", type: "image/png", sizes: "16x16", href: "/img/favicon-16x16.png"}],
      ['link', { rel: 'icon', href: '/img/Openmqttgateway_logo_mini.png' }],
      ['link', { rel: 'manifest', href: '/manifest.json' }],
      ['meta', { name: 'theme-color', content: '#3eaf7c' }],
      ['meta', { name: 'apple-mobile-web-app-capable', content: 'yes' }],
      ['meta', { name: 'apple-mobile-web-app-status-bar-style', content: 'black' }],
      ['link', { rel: "apple-touch-icon", sizes: "180x180", href: "/img/apple-touch-icon.png"}],
      ['link', { rel: 'mask-icon', href: '/icons/safari-pinned-tab.svg', color: '#3eaf7c' }],
      ['meta', { name: 'msapplication-TileImage', content: '/icons/msapplication-icon-144x144.png' }],
      ['meta', { name: 'msapplication-TileColor', content: '#000000' }]
    ],
    themeConfig: {
      smoothScroll: true,
      repo: '1technophile/OpenMQTTGateway',
      docsDir: 'docs',
      docsBranch: 'development',
      lastUpdated: 'Last Updated',
      editLinks: true,
      nav: [
        { text: 'Blog', link: 'https://1technophile.blogspot.com'},
        { text: 'Docs', link: '/'},
        { text: 'Community', link: 'https://community.openmqttgateway.com', target:'_self', rel:''},
        { text: 'Devices', link: 'https://compatible.openmqttgateway.com/index.php/devices'},
        { text: 'Boards', link: 'https://compatible.openmqttgateway.com/index.php/boards'}
      ],
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
            'upload/binaries',
            'upload/builds',
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
            'use/gateway'
          ]
        },
        {
          title: '5 - Integrate 🎉',   // required
          sidebarDepth: 1,    // optional, defaults to 1
          children: [
            'integrate/openhab2',
            'integrate/home_assistant',
            'integrate/node_red'
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
    plugins: [
      ['@vuepress/pwa', {
        serviceWorker: true,
        updatePopup: true
      }],
      ['@vuepress/medium-zoom', true],
      ['@vuepress/nprogress']
    ]
  }
