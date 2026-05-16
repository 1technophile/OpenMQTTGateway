// docs/.vitepress/config.mts
import { defineConfig } from 'vitepress'
import { resolve, dirname } from 'path'
import { fileURLToPath } from 'url'
import { commonNav, commonHead } from './commonConfig.mts'

const __dirname = dirname(fileURLToPath(import.meta.url))

// Load metadata with override support
import defaults from './defaults.json'
let meta = { ...defaults }
try {
  const overrides = await import('./meta.json')
  meta = { ...meta, ...overrides.default }
} catch {
  console.warn('meta.json not found or not valid. Using default configuration.')
}

export default defineConfig({
  title: `${meta.title} - ${meta.version}`,
  description: 'One gateway, many technologies: MQTT gateway for ESP8266 or ESP32 with bidirectional 433mhz/315mhz/868mhz, Infrared communications, BLE, LoRa, beacons detection, mi flora / mi jia / LYWSD02/ Mi Scale compatibility, SMS & LORA.',
  base: meta.url_prefix as '/' | `/${string}/`,
  outDir: resolve(__dirname, '../../', meta.dest),
  head: commonHead,
  ignoreDeadLinks: [
    /localhost/,
    /LoraTemperature/
  ],
  lastUpdated: true,
  markdown: {
    lineNumbers: true
  },
  themeConfig: {
    siteTitle: 'OpenMQTTGateway',
    // @ts-expect-error custom fields consumed by index.md to render the dev-version banner
    mode: meta.mode,
    // @ts-expect-error
    version: meta.version,
    // @ts-expect-error
    repo: meta.theme_config_repo,
    nav: commonNav,
    socialLinks: [
      { icon: 'github', link: `https://github.com/${meta.theme_config_repo}` }
    ],
    editLink: {
      pattern: `https://github.com/${meta.theme_config_repo}/edit/development/docs/:path`
    },
    sidebar: [
      { text: '0 - What is it for', link: '/' },
      {
        text: '1 - Prerequisites',
        collapsed: true,
        items: [
          { text: 'Devices', link: '/prerequisites/devices' },
          { text: 'Board', link: '/prerequisites/board' },
          { text: 'Parts', link: '/prerequisites/parts' },
          { text: 'Broker', link: '/prerequisites/broker' },
          { text: 'Controller', link: '/prerequisites/controller' }
        ]
      },
      {
        text: '2 - Set it up',
        collapsed: true,
        items: [
          { text: 'RF', link: '/setitup/rf' },
          { text: 'BLE', link: '/setitup/ble' },
          { text: 'IR', link: '/setitup/ir' },
          { text: 'LoRa', link: '/setitup/lora' },
          { text: 'GSM', link: '/setitup/gsm' },
          { text: 'Serial', link: '/setitup/serial' },
          { text: 'Sensors', link: '/setitup/sensors' },
          { text: 'Actuators', link: '/setitup/actuators' }
        ]
      },
      {
        text: '3 - Upload',
        collapsed: true,
        link: '/upload/',
        items: [
          { text: '(Option 1) Upload from the web', link: '/upload/web-install' },
          { text: 'Binaries', link: '/upload/binaries' },
          { text: 'Builds', link: '/upload/builds' },
          { text: 'Gitpod', link: '/upload/gitpod' },
          { text: 'Portal', link: '/upload/portal' },
          { text: 'Portal HTTP API', link: '/upload/portal-api' },
          { text: 'Advanced Configuration', link: '/upload/advanced-configuration' },
          { text: 'Troubleshoot', link: '/upload/troubleshoot' }
        ]
      },
      {
        text: '4 - Use',
        collapsed: true,
        items: [
          { text: 'RF', link: '/use/rf' },
          { text: 'BLE', link: '/use/ble' },
          { text: 'IR', link: '/use/ir' },
          { text: 'LoRa', link: '/use/lora' },
          { text: 'GSM', link: '/use/gsm' },
          { text: 'Serial', link: '/use/serial' },
          { text: 'RFM69', link: '/use/rfm69' },
          { text: 'Sensors', link: '/use/sensors' },
          { text: 'Actuators', link: '/use/actuators' },
          { text: 'Boards', link: '/use/boards' },
          { text: 'Displays', link: '/use/displays' },
          { text: 'Gateway', link: '/use/gateway' },
          { text: 'WebUI', link: '/use/webui' }
        ]
      },
      {
        text: '5 - Integrate',
        collapsed: true,
        items: [
          { text: 'OpenHAB 2', link: '/integrate/openhab2' },
          { text: 'OpenHAB 3', link: '/integrate/openhab3' },
          { text: 'Home Assistant', link: '/integrate/home_assistant' },
          { text: 'Node-RED', link: '/integrate/node_red' },
          { text: 'AWS IoT', link: '/integrate/aws_iot' },
          { text: 'Jeedom', link: '/integrate/jeedom' }
        ]
      },
      {
        text: '6 - Participate',
        collapsed: true,
        link: '/participate/',
        items: [
          { text: 'Quick Start', link: '/participate/quick_start' },
          { text: 'Support', link: '/participate/support' },
          { text: 'Development', link: '/participate/development' },
          { text: 'Adding Protocols', link: '/participate/adding-protocols' },
          { text: 'Community', link: '/participate/community' },
          { text: 'License', link: meta.url_license_file }
        ]
      }
    ],
    search: {
      provider: 'local'
    }
  },
  sitemap: {
    hostname: meta.hostname
  },
  vite: {
    publicDir: '.vitepress/public'
  }
})
