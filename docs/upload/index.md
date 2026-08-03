# Upload

Getting OpenMQTTGateway onto your board takes three steps, whichever method you pick:

<div class="omg-journey">
  <div class="omg-step">
    <span class="omg-step-num">1</span>
    <strong>Flash the firmware</strong>
    <span class="omg-step-sub">pick a method below</span>
  </div>
  <span class="omg-step-arrow">→</span>
  <div class="omg-step">
    <span class="omg-step-num">2</span>
    <strong>Configure WiFi &amp; MQTT</strong>
    <span class="omg-step-sub">through the <a href="./portal.html">portal</a></span>
  </div>
  <span class="omg-step-arrow">→</span>
  <div class="omg-step">
    <span class="omg-step-num">3</span>
    <strong>Verify</strong>
    <span class="omg-step-sub">gateway appears on your <a href="../getting-started.html#step-4-check-that-it-works">broker &amp; controller</a></span>
  </div>
</div>

## Choose your installation method

<div class="omg-methods">
  <a class="omg-method" href="./web-install.html">
    <span class="omg-method-badge">Easiest</span>
    <h3>1. Web installer</h3>
    <p><strong>Choose this if</strong> you want the fastest path: flash directly from your browser, nothing to install.</p>
    <p class="omg-method-req">Needs Chrome, Edge or Opera and a data USB cable. Standard configurations only.</p>
  </a>
  <a class="omg-method" href="./binaries.html">
    <h3>2. Ready-to-go binaries</h3>
    <p><strong>Choose this if</strong> you prefer a desktop flashing tool (esptool.py, Espressif Flash Download Tool), or web flashing doesn't work for you.</p>
    <p class="omg-method-req">Same pre-built firmware as the web installer, flashed from your computer.</p>
  </a>
  <a class="omg-method" href="./builds.html">
    <h3>3. Build from source</h3>
    <p><strong>Choose this if</strong> you need custom pin assignments, module combinations, or credentials embedded at build time.</p>
    <p class="omg-method-req">Needs <a href="https://platformio.org/">PlatformIO</a>. Full control over the configuration.</p>
  </a>
</div>

|                          | Web installer | Binaries | From source |
|--------------------------|:---:|:---:|:---:|
| No software to install   | ✅ | ❌ | ❌ |
| Custom configuration     | ❌ | ❌ | ✅ |
| Difficulty               | Easiest | Easy | Advanced |

## After flashing: configure your gateway

However you flashed, a freshly installed gateway starts its own WiFi access point (named `OpenMQTTGateway` or starting with `OMG_`). Connect to it and the [configuration portal](portal.md) lets you set your WiFi network, MQTT broker, and optional security settings — no rebuild needed.

If you build from source you can alternatively [embed the network and MQTT settings at build time](builds.md), so the gateway connects automatically on first boot.

## Going further

* [Advanced configuration](advanced-configuration.md) — TLS-secured MQTT connections, certificates, OTA updates.
* [Home Assistant integration](../integrate/home_assistant.md) — auto-discovery is enabled by default; your gateway and sensors appear automatically.
* [Gateway configuration](../use/gateway.md) — MQTT topic structure and runtime commands per module.

Stuck? Check the [troubleshooting page](troubleshoot.md), or ask on the [community forum](https://community.openmqttgateway.com).

<style scoped>
.omg-journey {
  display: flex;
  align-items: stretch;
  gap: 8px;
  margin: 24px 0;
  flex-wrap: wrap;
}
.omg-step {
  flex: 1 1 150px;
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 4px;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 16px 12px;
}
.omg-step-num {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border-radius: 50%;
  background: var(--vp-c-brand-1);
  color: var(--vp-c-white, #fff);
  font-weight: 700;
  font-size: 14px;
}
.omg-step-sub {
  color: var(--vp-c-text-2);
  font-size: 13px;
}
.omg-step-arrow {
  align-self: center;
  color: var(--vp-c-brand-1);
  font-size: 20px;
  font-weight: 700;
}
.omg-methods {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: 16px;
  margin: 16px 0 24px;
}
.omg-method {
  position: relative;
  display: block;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 20px;
  text-decoration: none !important;
  color: var(--vp-c-text-1);
  transition: border-color 0.2s ease, transform 0.2s ease;
}
.omg-method:hover {
  border-color: var(--vp-c-brand-1);
  transform: translateY(-2px);
}
.omg-method h3 {
  margin: 0 0 8px;
  font-size: 16px;
  font-weight: 600;
  color: var(--vp-c-brand-1);
}
.omg-method p {
  margin: 0 0 8px;
  font-size: 14px;
  line-height: 1.5;
}
.omg-method-req {
  color: var(--vp-c-text-2);
  font-size: 13px !important;
}
.omg-method-badge {
  position: absolute;
  top: -10px;
  right: 12px;
  background: var(--vp-c-brand-1);
  color: var(--vp-c-white, #fff);
  border-radius: 10px;
  padding: 2px 10px;
  font-size: 12px;
  font-weight: 600;
}
@media (max-width: 640px) {
  .omg-journey { flex-direction: column; }
  .omg-step-arrow { transform: rotate(90deg); }
}
</style>
