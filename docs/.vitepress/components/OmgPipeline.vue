<template>
  <div class="omg-pipeline-wrap">
    <div class="omg-pipeline">
      <div class="omg-node">
        <span class="omg-node-icon">🌡️</span>
        <strong>Your devices</strong>
        <span class="omg-node-sub">BLE · RF · IR · LoRa</span>
      </div>
      <span class="omg-arrow">⇄</span>
      <div class="omg-node omg-node-brand">
        <span class="omg-node-icon">📟</span>
        <strong>OpenMQTTGateway</strong>
        <span class="omg-node-sub">ESP32 / ESP8266</span>
      </div>
      <span class="omg-arrow">⇄</span>
      <div class="omg-node">
        <span class="omg-node-icon">🔀</span>
        <strong>MQTT broker</strong>
        <span class="omg-node-sub">Mosquitto...</span>
      </div>
      <span class="omg-arrow">⇄</span>
      <div class="omg-node">
        <span class="omg-node-icon">🏠</span>
        <strong>Your controller</strong>
        <span class="omg-node-sub">Home Assistant, OpenHAB...</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.omg-pipeline-wrap {
  container-type: inline-size;
  margin: 24px 0;
}

.omg-pipeline {
  display: flex;
  align-items: stretch;
  gap: 8px;
}

.omg-node {
  flex: 1 1 0;
  min-width: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  gap: 4px;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 20px 10px;
}

.omg-node-brand {
  border-color: var(--vp-c-brand-1);
  box-shadow: 0 0 0 1px var(--vp-c-brand-1);
}

.omg-node-icon {
  font-size: 28px;
}

.omg-node strong {
  font-size: 15px;
  line-height: 1.3;
}

.omg-node-sub {
  color: var(--vp-c-text-2);
  font-size: 13px;
}

.omg-arrow {
  align-self: center;
  flex-shrink: 0;
  color: var(--vp-c-brand-1);
  font-size: 20px;
  font-weight: 700;
}

/* Narrow containers (doc column, tablets): tighten the nodes, keep one row */
@container (max-width: 760px) {
  .omg-pipeline {
    gap: 6px;
  }
  .omg-node {
    padding: 12px 6px;
  }
  .omg-node strong {
    font-size: 13px;
  }
  .omg-node-sub {
    font-size: 11px;
  }
  .omg-node-icon {
    font-size: 22px;
  }
  .omg-arrow {
    font-size: 16px;
  }
}

/* Narrow (phones, small tablets): stack vertically. Below ~680px the four nodes
   no longer leave room for the longest label ("OpenMQTTGateway"), which spilled
   out of its box and over the arrows. */
@container (max-width: 680px) {
  .omg-pipeline {
    flex-direction: column;
  }
  .omg-arrow {
    transform: rotate(90deg);
  }
  .omg-node strong {
    font-size: 15px;
  }
  .omg-node-sub {
    font-size: 13px;
  }
}
</style>
