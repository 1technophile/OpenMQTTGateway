module.exports = {
    markdown: {
      lineNumbers: true
    },
    title: 'OpenMQTTGateway',
    description: 'One gateway, many technologies',
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
        { text: 'Devices', link: 'https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=2126158079'}
      ],
      sidebar: [
        ['/','0 - What is it for 🏠'],
        {
          title: '1 - Prerequisites🧭',   // required
          path: '/prerequisites/devices/',       // optional, which should be a absolute path.
          //collapsable: true, // optional, defaults to true
          sidebarDepth: 1,    // optional, defaults to 1
          children: [
            'prerequisites/devices',
            'prerequisites/board',
            'prerequisites/broker',
            'prerequisites/controller']
        },
        {
          title: '2 - Set it up 🔨',   // required
          path: '/setitup/rf',       // optional, which should be a absolute path.
          sidebarDepth: 1,    // optional, defaults to 1
          children: [
            'setitup/rf',
            'setitup/ble',
            'setitup/ir',
            'setitup/lora',
            'setitup/gsm',
            'setitup/sensors',
            'setitup/actuators'
          ]
        },
        {
          title: '3 - Upload ➡️',   // required
          path: '/upload/binaries',       // optional, which should be a absolute path.
          sidebarDepth: 1,    // optional, defaults to 1
          children: [
            'upload/binaries',
            'upload/pio'
          ]
        },
        {
          title: '4 - Use ✈️',   // required
          path: '/use/rf',       // optional, which should be a absolute path.
          sidebarDepth: 1,    // optional, defaults to 1
          children: [
            'use/rf',
            'use/ble',
            'use/ir',
            'use/lora',
            'use/gsm',
            'use/rfm69',
            'use/sensors',
            'use/actuators'
          ]
        },
        {
          title: '5 - Integrate 🎉',   // required
          path: '/integrate/openhab2/',       // optional, which should be a absolute path.
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
            'CONTRIBUTING',
            ['https://github.com/1technophile/OpenMQTTGateway/blob/development/LICENSE.txt','License']
          ]
        }
    ]
    }
  }
