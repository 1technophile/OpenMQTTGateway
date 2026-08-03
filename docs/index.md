---
layout: home

hero:
  name: OpenMQTTGateway
  text: One gateway, many technologies
  tagline: Bridge RF, Infrared, Bluetooth LE, LoRa and more to MQTT, and to your home automation platform.
  image:
    src: /img/OpenMQTTGateway.png
    alt: Overview of the protocols and compatible controllers
  actions:
    - theme: brand
      text: Get started
      link: /getting-started
    - theme: alt
      text: Flash from your browser
      link: /upload/web-install
    - theme: alt
      text: Supported BLE devices
      link: https://decoder.theengs.io/devices/devices.html

features:
  - icon: 📶
    title: Bluetooth Low Energy
    details: Decodes more than 100 BLE sensors out of the box thanks to Theengs Decoder — Mi Flora, Xiaomi, Govee, Inkbird, SwitchBot and more.
  - icon: 📡
    title: RF 433/315/868/915MHz
    details: Receive and transmit RF signals with RTL_433, RCSwitch, KaKu or Pilight — weather stations, door sensors, remotes, PIR sensors.
  - icon: 🔴
    title: Infrared & LoRa
    details: Make your old TV or AC smart through IR, or reach far-away sensors with LoRa.
  - icon: 🏠
    title: Home automation ready
    details: Home Assistant MQTT auto-discovery enabled by default. Works with OpenHAB, Domoticz, Jeedom, Node-RED, FHEM and any MQTT-capable software.
  - icon: ⚡
    title: Easy to install
    details: Flash an ESP32 or ESP8266 directly from your browser, then configure WiFi and MQTT through a web portal. No development tools required.
  - icon: 🔧
    title: Extensible
    details: Add sensors (BME280, DHT, INA226...) and actuators (relays, PWM, LED), secure connections with TLS, and update over the air.
---

<script setup>
import { useData } from 'vitepress'
import { computed } from 'vue'
const { theme } = useData()
const isDev = theme.value.mode === 'dev'
const version = theme.value.version
const commitUrl = computed(() =>
  theme.value.repo && version
    ? `https://github.com/${theme.value.repo}/commit/${version}`
    : null
)

const useCases = [
  { icon: '🌱', text: 'Water the garden when a Mi Flora sensor says the soil is dry' },
  { icon: '🚪', text: 'Detect opened doors and windows through 433MHz or BLE' },
  { icon: '🧊', text: 'Get alerted when the fridge or freezer gets too warm' },
  { icon: '📺', text: 'Make your old TV or AC smart through infrared control' },
  { icon: '⌚', text: 'Run a welcome scenario when your smartwatch comes home' },
  { icon: '🥩', text: 'Follow your meat temperature while cooking with an Inkbird IBBQ' },
  { icon: '💧', text: 'Detect water leakage or smoke remotely' },
  { icon: '📬', text: 'Know when a distant mailbox is opened, over LoRa' },
  { icon: '🌡️', text: 'Trigger a fan from a Mi Jia temperature and humidity sensor' },
  { icon: '🚨', text: 'Sound a siren if something is going wrong' },
  { icon: '⚖️', text: 'Log your weight automatically from a BLE scale' },
  { icon: '🛞', text: 'Monitor your vehicle tire pressure' }
]

const functions = [
  'Home Assistant auto-discovery', 'Message deduplication', 'Simple MQTT API',
  'Signal forward & repeat', 'WiFi onboarding portal', 'Web UI configuration',
  'Whitelist & blacklist', 'TLS secure connections', 'Over-the-air updates',
  'Local first, cloud optional'
]

const videos = [
  { id: '_gdXR1uklaY', title: '433MHz and BLE gateway', author: 'Andreas Spiess' },
  { id: 'noUROhtf0E0', title: 'BLE gateway', author: 'Andreas Spiess' },
  { id: 'H-JXWbWjJYE', title: 'RTL_433 on ESP32', author: 'Tech Mind' },
  { id: '6DftaHxDawM', title: 'LoRa gateway', author: 'PricelessToolkit' }
]

const press = [
  { name: 'ESPHome vs OMG', url: 'https://medium.com/@zediot/esphome-vs-openmqttgateway-choosing-the-right-esp32-bridge-model-7126379e36be' },
  { name: 'ZedIoT', url: 'https://zediot.com/blog/what-is-openmqttgateway/' },
  { name: 'HomeTechHacker', url: 'https://hometechhacker.com/theengs-bridge-ble-mqtt-gateway-review/' },
  { name: 'Squix', url: 'https://blog.squix.org/2023/04/esp32-cheap-sensor-network-with-openmqttgateway.html' },
  { name: 'Hackaday', url: 'https://hackaday.com/2023/01/13/arduino-library-brings-rtl_433-to-the-esp32' },
  { name: 'CNX Software', url: 'https://www.cnx-software.com/2023/01/14/esp32-board-with-lora-433-mhz-sensors/' },
  { name: 'RTL-SDR', url: 'https://www.rtl-sdr.com/rtl_433-ported-to-esp32-microcontrollers-with-cc1101-or-sx127x-transceiver-chips/' },
  { name: 'LWN.net', url: 'https://lwn.net/Articles/921497/' }
]
</script>

<div v-if="isDev" class="warning custom-block omg-section">
<p class="custom-block-title">Development Version</p>
<p>This is the edge version of the documentation, built from commit <a v-if="commitUrl" :href="commitUrl"><code>{{ version }}</code></a><code v-else>{{ version }}</code>. It is under active development and may contain bugs, incomplete features, or breaking changes. Use it at your own risk.</p>
</div>

<div class="omg-section">
  <h2>How it works</h2>
  <p class="omg-lead">OpenMQTTGateway is a firmware for ESP32 and ESP8266 boards that translates your devices' signals into <a href="http://mqtt.org/">MQTT</a> messages — and back. One board replaces a drawer full of proprietary bridges.</p>
  <OmgPipeline />
  <p class="omg-caption">Decodes <a href="https://decoder.theengs.io/devices/devices.html">more than 100 BLE devices</a> and many RF protocols (433/315/868/915MHz) through <a href="./use/rf.html#supported-decoders">RTL_433 decoders</a> — and the BLE decoding also runs on Raspberry Pi, Windows or Linux with <a href="https://theengs.github.io/gateway/">Theengs Gateway</a>.</p>
</div>

<div class="omg-section">
  <h2>What will you automate?</h2>
  <div class="omg-grid">
    <div v-for="uc in useCases" :key="uc.text" class="omg-card">
      <span class="omg-card-icon">{{ uc.icon }}</span>
      <span>{{ uc.text }}</span>
    </div>
  </div>
  <p class="omg-caption">The limit is your imagination 😀</p>
</div>

<div class="omg-section">
  <h2>See it in action</h2>
  <div class="omg-videos">
    <a v-for="v in videos" :key="v.id" class="omg-video" :href="`https://www.youtube.com/watch?v=${v.id}`" target="_blank" rel="noopener noreferrer">
      <img class="no-zoom" :src="`https://img.youtube.com/vi/${v.id}/mqdefault.jpg`" :alt="`${v.title} video by ${v.author}`" loading="lazy" />
      <span class="omg-video-title">{{ v.title }}</span>
      <span class="omg-video-author">by {{ v.author }}</span>
    </a>
  </div>
  <p class="omg-caption">In the press:
    <template v-for="(p, i) in press" :key="p.url"><a :href="p.url" target="_blank" rel="noopener noreferrer">{{ p.name }}</a><span v-if="i < press.length - 1"> · </span></template>
  </p>
</div>

<div class="omg-section">
  <h2>Under the hood</h2>
  <div class="omg-pills">
    <span v-for="f in functions" :key="f" class="omg-pill">{{ f }}</span>
  </div>
</div>

<div class="omg-section">
  <h2>Support the project</h2>
  <div class="omg-product">
    <a href="https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna" target="_blank" rel="noopener noreferrer">
      <img src="./img/Theengs-Bridge-ble-gateway.png" alt="Theengs Bridge, BLE to MQTT gateway with Ethernet and external antenna" />
    </a>
    <div>
      <h3>Theengs Bridge — ready-to-use BLE gateway</h3>
      <p>Pre-flashed with OpenMQTTGateway, with an Ethernet port and an external antenna for enhanced BLE range. Every purchase directly funds the project.</p>
      <a class="omg-cta" href="https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna" target="_blank" rel="noopener noreferrer">Get the Theengs Bridge</a>
    </div>
  </div>
  <p style="margin-top: 24px;">You can also sponsor the development to help keep the project healthy and evolving.</p>
  <div style="text-align: center;">
    <iframe src="https://github.com/sponsors/1technophile/button" title="Sponsor 1technophile" height="32" width="228" style="border: 0; border-radius: 6px;"></iframe>
  </div>
</div>

<div class="omg-section">
  <p class="omg-disclaimer">The material and information contained in this documentation is for general information purposes only. You should not rely upon it as a basis for making any business, legal or any other decisions. There is no warranty given on this documentation content; if you decide to follow the information and materials given it is at your own risk.</p>
</div>

<style scoped>
.omg-section {
  max-width: 1152px;
  margin: 0 auto;
  padding: 24px 24px 0;
}
.omg-section h2 {
  border-top: 1px solid var(--vp-c-divider);
  padding-top: 24px;
  margin-bottom: 16px;
  font-size: 24px;
  font-weight: 600;
  letter-spacing: -0.02em;
}
.omg-lead {
  color: var(--vp-c-text-1);
  max-width: 720px;
  margin: 0 0 8px;
}
.omg-caption {
  color: var(--vp-c-text-2);
  font-size: 14px;
  margin-top: 16px;
}
.omg-caption a { color: var(--vp-c-brand-1); }
.omg-lead a { color: var(--vp-c-brand-1); }

/* Use cases grid */
.omg-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 12px;
}
.omg-card {
  display: flex;
  align-items: center;
  gap: 12px;
  background: var(--vp-c-bg-soft);
  border-radius: 12px;
  padding: 14px 16px;
  font-size: 14px;
  line-height: 1.5;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}
.omg-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.12);
}
.omg-card-icon { font-size: 24px; flex-shrink: 0; }

/* Videos */
.omg-videos {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 20px;
}
.omg-video {
  display: flex;
  flex-direction: column;
  gap: 2px;
  text-decoration: none;
  color: var(--vp-c-text-1);
}
.omg-video img {
  width: 100%;
  border-radius: 12px;
  aspect-ratio: 16 / 9;
  object-fit: cover;
  transition: transform 0.2s ease;
}
.omg-video:hover img { transform: scale(1.03); }
.omg-video-title { font-size: 14px; font-weight: 600; margin-top: 8px; }
.omg-video-author { font-size: 13px; color: var(--vp-c-text-2); }

/* Function pills */
.omg-pills {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
.omg-pill {
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 999px;
  padding: 6px 14px;
  font-size: 13px;
  color: var(--vp-c-text-1);
}

/* Product card */
.omg-product {
  display: flex;
  align-items: center;
  gap: 32px;
  background: var(--vp-c-bg-soft);
  border-radius: 12px;
  padding: 24px;
  flex-wrap: wrap;
}
.omg-product > a { flex: 0 1 280px; min-width: 200px; }
.omg-product img { width: 100%; border-radius: 8px; }
.omg-product > div { flex: 1 1 300px; }
.omg-product h3 { margin: 0 0 8px; font-size: 18px; font-weight: 600; }
.omg-product p { color: var(--vp-c-text-2); margin: 0 0 16px; }
.omg-cta {
  display: inline-block;
  background: var(--vp-c-brand-1);
  color: var(--vp-c-white, #fff);
  border-radius: 20px;
  padding: 8px 20px;
  font-size: 14px;
  font-weight: 600;
  text-decoration: none;
  transition: background 0.2s ease;
}
.omg-cta:hover { background: var(--vp-c-brand-2); }

.omg-disclaimer {
  border-top: 1px solid var(--vp-c-divider);
  padding: 24px 0 32px;
  color: var(--vp-c-text-3);
  font-size: 12px;
  line-height: 1.6;
}

@media (max-width: 640px) {
  .omg-pipeline { flex-direction: column; }
  .omg-arrow { transform: rotate(90deg); }
}
</style>
