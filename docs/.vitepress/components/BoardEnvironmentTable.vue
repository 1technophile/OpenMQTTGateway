<template>
  <div class="board-environment-list">
    <div v-if="loading" class="custom-block tip">
      <p class="custom-block-title">
        <span class="loading-spinner" aria-label="Loading"></span>
        Loading supported environments...
      </p>
    </div>

    <div v-if="error" class="custom-block danger">
      <p class="custom-block-title">Error</p>
      <p>{{ error }}</p>
    </div>

    <div v-if="!loading && !error">
      <!-- Guided picker -->
      <div class="wizard">
        <span class="wizard-title">Help me choose</span>
        <div class="wizard-row">
          <span class="filter-group__label">I want to bridge:</span>
          <div class="filter-chips">
            <button
              v-for="b in bridgeOptions"
              :key="b.key"
              :class="['filter-chip', { 'filter-chip--active': wizardBridge === b.key }]"
              @click="wizardBridge = wizardBridge === b.key ? null : b.key">
              {{ b.label }}
            </button>
          </div>
        </div>
        <div class="wizard-row">
          <span class="filter-group__label">My hardware:</span>
          <div class="filter-chips">
            <button
              v-for="h in hardwareOptions"
              :key="h.key"
              :class="['filter-chip', { 'filter-chip--active': wizardHardware === h.key }]"
              @click="wizardHardware = wizardHardware === h.key ? null : h.key">
              {{ h.label }}
            </button>
          </div>
        </div>
        <div v-if="wizardBridge && recommendedBoard" class="wizard-result">
          <div class="wizard-result__text">
            <span>Recommended: <code>{{ recommendedBoard.environment }}</code></span>
            <span v-if="recommendedBoard.description" class="wizard-result__desc" v-html="recommendedBoard.description"></span>
            <span v-if="wizardHardware === 'none' && shoppingHint" class="wizard-result__desc">🛒 {{ shoppingHint }}</span>
          </div>
          <button class="wizard-result__cta" @click="openSelector(recommendedBoard.environment)">Flash it →</button>
        </div>
        <div v-else-if="wizardBridge && !recommendedBoard" class="wizard-result wizard-result--none">
          No pre-built environment matches this combination<template v-if="wizardBridge === 'ble' && wizardHardware === 'esp8266'"> — Bluetooth needs an ESP32</template>. Try another hardware choice, or <a :href="buildsUrl">build from source</a>.
        </div>
      </div>

      <!-- Search and Filter Bar -->
      <div class="filter-bar">
        <div class="search-box">
          <svg class="search-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
          <input
            v-model="searchQuery"
            type="text"
            placeholder="Search by name, description, module..."
            class="search-input"
            aria-label="Search boards">
          <button
            v-if="searchQuery"
            class="search-clear"
            @click="searchQuery = ''"
            aria-label="Clear search">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
          </button>
        </div>

        <div v-if="hasActiveFilters" class="filter-status">
          <span>Showing {{ filteredBoards.length }} of {{ boards.length }} boards</span>
          <button class="filter-clear" @click="clearFilters">Clear all filters</button>
        </div>
      </div>

      <!-- No Results -->
      <div v-if="filteredBoards.length === 0" class="no-results">
        <p>No boards match your filters.</p>
        <button class="filter-clear" @click="clearFilters">Clear all filters</button>
      </div>

      <div class="boards-grid">
      <article
        v-for="board in filteredBoards"
        :key="board.environment"
        class="board-card"
        @click="openSelector(board.environment)">

        <div class="board-card__image">
          <span v-if="popularEnvironments.includes(board.environment)" class="popular-badge">Popular</span>
          <img
            :src="getBoardImageUrl(board)"
            :alt="board.environment"
            loading="lazy">
        </div>

        <div class="board-card__content">
          <div class="board-card__header">
            <h3 class="board-card__title">
              <code>{{ board.environment }}</code>
            </h3>
            <span v-if="board.microcontroller" class="board-card__chip">
              {{ board.microcontroller }}
            </span>
          </div>

          <p v-if="board.description" class="board-card__description" v-html="board.description">
          </p>


          <!-- Modules Section with expand/collapse -->
          <div v-if="Array.isArray(board.modules) && board.modules.length" class="board-card__libraries">
            <div class="modules-label-row">
              <span class="libraries-label">Modules:</span>
              <span
                v-if="board.modules.length > 2"
                class="expand-icon-btn"
                @click.stop="toggleModules(board.environment)"
                :title="expandedModules[board.environment] ? 'Hide' : 'Show all'">
                <svg :class="{rotated: expandedModules[board.environment]}" width="19" height="19" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 6 15 12 9 18"/></svg>
              </span>
            </div>
            <Transition name="expand-fade">
              <div class="libraries-badges"
                :style="expandedModules[board.environment] ? '' : 'max-height: 2.2em; overflow: hidden;'">
                <span
                  v-for="(mod, index) in board.modules"
                  :key="index"
                  class="lib-badge">
                  {{ mod }}
                </span>
              </div>
            </Transition>

          </div>

          <!-- Libraries Section with expand/collapse -->
          <div v-if="Array.isArray(board.libraries) && board.libraries.length" class="board-card__libraries">
            <div class="modules-label-row">
              <span class="libraries-label">Libraries:</span>
              <span
                v-if="board.libraries.length > 2"
                class="expand-icon-btn"
                @click.stop="toggleLibraries(board.environment)"
                :title="expandedLibraries[board.environment] ? 'Hide' : 'Show all'">
                <svg :class="{rotated: expandedLibraries[board.environment]}" width="19" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 6 15 12 9 18"/></svg>
              </span>

            </div>
            <Transition name="expand-fade">
              <div class="libraries-badges"
                :style="expandedLibraries[board.environment] ? '' : 'max-height: 2.2em; overflow: hidden;'">
                <span
                  v-for="(lib, index) in board.libraries"
                  :key="index"
                  class="lib-badge">
                  {{ lib }}
                </span>
              </div>
            </Transition>

          </div>

          <div class="board-card__action">
            <span class="action-text">More info →</span>
          </div>
        </div>
      </article>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, watch } from 'vue'

interface Board {
  environment: string
  description?: string
  microcontroller?: string
  modules?: string[]
  libraries?: string[]
  customImg?: string
  CustomImg?: string
}

const props = withDefaults(defineProps<{
  boardsUrl?: string
  selectorPath?: string
}>(), {
  boardsUrl: '/boards-info.json',
  selectorPath: '/upload/board-selector.html'
})

const boards = ref<Board[]>([])
const loading = ref(false)
const error = ref<string | null>(null)
const expandedModules = reactive<Record<string, boolean>>({})
const expandedLibraries = reactive<Record<string, boolean>>({})
const searchQuery = ref('')
const wizardBridge = ref<string | null>(null)
const wizardHardware = ref<string | null>(null)

const bridgeOptions = [
  { key: 'ble', label: 'Bluetooth devices' },
  { key: 'rf433', label: 'RF devices (433/868/915MHz)' },
  { key: 'ir', label: 'Infrared' },
  { key: 'lora', label: 'LoRa' }
]

const hardwareOptions = [
  { key: 'esp32dev', label: 'Generic ESP32' },
  { key: 'esp32c3', label: 'ESP32-C3' },
  { key: 'esp32s3', label: 'ESP32-S3' },
  { key: 'lilygo', label: 'LilyGo / Heltec / TTGO' },
  { key: 'm5', label: 'M5Stack' },
  { key: 'esp8266', label: 'ESP8266 / NodeMCU' },
  { key: 'none', label: "I don't have hardware yet" }
]

const popularEnvironments = [
  'esp32dev-ble',
  'theengs-bridge-v11',
  'lilygo-rtl_433',
  'lilygo-rtl_433-fsk',
  'esp32dev-ir'
]

function buildUrl(path: string): string {
  const base = import.meta.env.BASE_URL || '/'
  const cleanBase = base.endsWith('/') ? base.slice(0, -1) : base
  const cleanPath = path.startsWith('/') ? path : `/${path}`
  return `${cleanBase}${cleanPath}`
}

const resolvedBoardsUrl = computed(() => buildUrl(props.boardsUrl))
const resolvedSelectorUrl = computed(() => buildUrl(props.selectorPath))

function getMcuFamily(mcu: string | undefined): string | null {
  if (!mcu) return null
  const m = mcu.toLowerCase()
  if (m.includes('esp32-s3') || m.includes('esp32s3') || m.includes('atoms3') || m.includes('lilygo-t3-s3')) return 'ESP32-S3'
  if (m.includes('esp32-c3') || m.includes('esp32c3') || m.includes('lolin_c3') || m.includes('airm2m')) return 'ESP32-C3'
  if (m.includes('esp32') || m.includes('m5st') || m.includes('heltec') || m.includes('ttgo') || m.includes('lolin32') || m.includes('pico32') || m.includes('tinypico') || m.includes('feather')) return 'ESP32'
  if (m.includes('esp8') || m.includes('nodemcu')) return 'ESP8266'
  return 'Other'
}

function matchesBridge(board: Board, bridge: string): boolean {
  const mods = (board.modules || []).map(m => m.toLowerCase())
  switch (bridge) {
    case 'ble': return mods.some(m => m.includes('bt'))
    case 'rf433': return mods.some(m => m.includes('rf') || m.includes('rtl_433') || m.includes('pilight'))
    case 'ir': return mods.some(m => m.includes('ir'))
    case 'lora': return mods.some(m => m.includes('lora'))
    default: return true
  }
}

function matchesHardware(board: Board, hardware: string): boolean {
  const haystack = `${board.environment} ${board.microcontroller || ''}`.toLowerCase()
  switch (hardware) {
    case 'esp32dev': return board.environment.toLowerCase().includes('esp32dev')
    case 'esp32c3': return getMcuFamily(board.microcontroller) === 'ESP32-C3'
    case 'esp32s3': return getMcuFamily(board.microcontroller) === 'ESP32-S3'
    case 'lilygo': return /lilygo|heltec|ttgo/.test(haystack)
    case 'm5': return haystack.includes('m5')
    case 'esp8266': return getMcuFamily(board.microcontroller) === 'ESP8266'
    default: return true // 'none': no hardware constraint
  }
}

// Preferred defaults per bridge choice, consulted before the generic
// popular-first order. All entries must be web-flashable environments.
const preferredByBridge: Record<string, string[]> = {
  ble: ['esp32dev-ble', 'esp32c3-dev-c2-ble', 'esp32s3-dev-c1-ble', 'esp32-m5atom-lite', 'heltec-ble', 'lilygo-ble'],
  rf433: ['lilygo-rtl_433', 'esp32dev-rtl_433', 'heltec-rtl_433', 'nodemcuv2-rf'],
  ir: ['esp32dev-ir', 'nodemcuv2-ir', 'esp32-m5atom-lite'],
  lora: ['ttgo-lora32-v21', 'heltec-wifi-lora-32', 'ttgo-lora32-v1', 'ttgo-t-beam']
}

const recommendedBoard = ref<Board | null>(null)
const manifestCache = new Map<string, boolean>()

// Not every environment in boards-info.json has a pre-built firmware
// manifest; only recommend ones that can actually be web-flashed.
async function hasManifest(env: string): Promise<boolean> {
  const cached = manifestCache.get(env)
  if (cached !== undefined) return cached
  let ok = false
  try {
    const response = await fetch(buildUrl(`/firmware_build/${env}.manifest.json`), { method: 'HEAD' })
    ok = response.ok
  } catch {
    ok = false
  }
  manifestCache.set(env, ok)
  return ok
}

let recommendationToken = 0
watch([wizardBridge, wizardHardware, boards], async () => {
  const token = ++recommendationToken
  const bridge = wizardBridge.value
  if (!bridge) {
    recommendedBoard.value = null
    return
  }
  const candidates = boards.value.filter(b =>
    matchesBridge(b, bridge) &&
    (!wizardHardware.value || matchesHardware(b, wizardHardware.value))
  )
  const preferred = preferredByBridge[bridge] || []
  const ordered = [...candidates].sort((a, b) => {
    const ai = preferred.indexOf(a.environment)
    const bi = preferred.indexOf(b.environment)
    return (ai === -1 ? preferred.length : ai) - (bi === -1 ? preferred.length : bi)
  })
  for (const candidate of ordered.slice(0, 8)) {
    if (await hasManifest(candidate.environment)) {
      if (token === recommendationToken) recommendedBoard.value = candidate
      return
    }
  }
  if (token === recommendationToken) recommendedBoard.value = null
})

const shoppingHint = computed<string>(() => {
  switch (wizardBridge.value) {
    case 'ble':
    case 'ir': return 'Any ESP32 development board will do.'
    case 'rf433': return 'A LILYGO LoRa32 or Heltec LoRa V2 board matching your devices\' frequency (433/868/915MHz) needs no soldering.'
    case 'lora': return 'Get a LILYGO LoRa32 or Heltec LoRa board matching the frequency used in your region.'
    default: return ''
  }
})

const buildsUrl = computed(() => buildUrl('/upload/builds.html'))

const hasActiveFilters = computed(() =>
  searchQuery.value || wizardBridge.value || wizardHardware.value
)

const filteredBoards = computed<Board[]>(() =>
  boards.value.filter(board => {
    if (wizardBridge.value && !matchesBridge(board, wizardBridge.value)) return false
    if (wizardHardware.value && wizardHardware.value !== 'none' && !matchesHardware(board, wizardHardware.value)) return false
    if (searchQuery.value) {
      const q = searchQuery.value.toLowerCase()
      const haystack = [
        board.environment,
        board.description || '',
        board.microcontroller || '',
        (board.modules || []).join(' ')
      ].join(' ').toLowerCase()
      if (!haystack.includes(q)) return false
    }
    return true
  })
)

function clearFilters() {
  searchQuery.value = ''
  wizardBridge.value = null
  wizardHardware.value = null
}

async function loadBoards() {
  loading.value = true
  error.value = null
  try {
    const response = await fetch(resolvedBoardsUrl.value)
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`)
    }
    const data = await response.json()
    if (!Array.isArray(data)) {
      throw new Error('boards-info.json must be an array')
    }
    boards.value = data
      .filter((board: Board) => board && typeof board.environment === 'string')
      .sort((a: Board, b: Board) => {
        const aPopular = popularEnvironments.includes(a.environment)
        const bPopular = popularEnvironments.includes(b.environment)
        if (aPopular && !bPopular) return -1
        if (!aPopular && bPopular) return 1
        return a.environment.localeCompare(b.environment)
      })
  } catch (err: unknown) {
    console.error('Failed to load boards-info:', err)
    error.value = err instanceof Error ? err.message : 'Unable to load board information'
  } finally {
    loading.value = false
  }
}

function toggleModules(env: string) {
  expandedModules[env] = !expandedModules[env]
}

function toggleLibraries(env: string) {
  expandedLibraries[env] = !expandedLibraries[env]
}

function openSelector(environment: string) {
  if (!environment) return
  const url = `${resolvedSelectorUrl.value}?env=${encodeURIComponent(environment)}`
  window.location.href = url
}

function getBoardImageUrl(board: Board): string {
  const customImg = board.customImg || board.CustomImg
  if (customImg) {
    if (customImg.startsWith('http')) {
      return customImg
    }
    return buildUrl(customImg)
  }
  return buildUrl('/img/microcontroller.gif')
}

onMounted(() => {
  // Preset the guided picker from query parameters, e.g. ?bridge=ble&hardware=esp32dev
  const params = new URLSearchParams(window.location.search)
  const bridgeParam = params.get('bridge')
  if (bridgeParam && bridgeOptions.some(b => b.key === bridgeParam)) {
    wizardBridge.value = bridgeParam
  }
  const hardwareParam = params.get('hardware')
  if (hardwareParam && hardwareOptions.some(h => h.key === hardwareParam)) {
    wizardHardware.value = hardwareParam
  }
  loadBoards()
})
</script>

<style scoped>
.board-environment-list {
  margin: 2rem 0;
}

/* Guided picker */
.wizard {
  display: flex;
  flex-direction: column;
  gap: 10px;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 1.25rem;
}

.wizard-title {
  font-weight: 600;
  font-size: 0.95rem;
}

.wizard-row {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.wizard-result {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  border-top: 1px dashed var(--vp-c-divider);
  padding-top: 12px;
  font-size: 0.9rem;
}

.wizard-result__text {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.wizard-result__desc {
  color: var(--vp-c-text-2);
  font-size: 0.85rem;
}

.wizard-result__cta {
  background: var(--vp-c-brand-1);
  color: var(--vp-c-white, #fff);
  border: none;
  border-radius: 20px;
  padding: 8px 18px;
  font-weight: 600;
  font-size: 0.9rem;
  cursor: pointer;
  transition: background 0.2s ease;
}

.wizard-result__cta:hover {
  background: var(--vp-c-brand-2);
}

.wizard-result--none {
  display: block;
  color: var(--vp-c-text-2);
}

.wizard-result--none a {
  color: var(--vp-c-brand-1);
}

/* Search and Filter Bar */
.filter-bar {
  margin-bottom: 1.5rem;
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.search-box {
  position: relative;
  display: flex;
  align-items: center;
}

.search-icon {
  position: absolute;
  left: 12px;
  color: var(--vp-c-text-2);
  pointer-events: none;
}

.search-input {
  width: 100%;
  padding: 0.6rem 2.2rem 0.6rem 2.4rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  font-size: 0.9rem;
  background: var(--vp-c-bg);
  color: var(--vp-c-text-1);
  transition: border-color 0.2s;
}

.search-input:focus {
  outline: none;
  border-color: var(--vp-c-brand-1);
  box-shadow: 0 0 0 3px rgba(62, 175, 124, 0.15);
}

.search-input::placeholder {
  color: var(--vp-c-text-3);
}

.search-clear {
  position: absolute;
  right: 8px;
  background: none;
  border: none;
  cursor: pointer;
  padding: 4px;
  color: var(--vp-c-text-2);
  border-radius: 50%;
  display: flex;
  align-items: center;
}

.search-clear:hover {
  background: var(--vp-c-bg-soft);
}

.filter-groups {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.filter-group__label {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--vp-c-text-2);
  white-space: nowrap;
}

.filter-chips {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
}

.filter-chip {
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  border: 1px solid var(--vp-c-divider);
  padding: 0.25rem 0.7rem;
  border-radius: 16px;
  font-size: 0.78rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.filter-chip:hover {
  border-color: var(--vp-c-brand-1);
  color: var(--vp-c-brand-1);
}

.filter-chip--active {
  background: var(--vp-c-brand-1);
  color: #ffffff;
  border-color: var(--vp-c-brand-1);
}

.filter-chip--active:hover {
  color: #ffffff;
  opacity: 0.9;
}

.filter-status {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  font-size: 0.85rem;
  color: var(--vp-c-text-2);
}

.filter-clear {
  background: none;
  border: none;
  color: var(--vp-c-brand-1);
  cursor: pointer;
  font-size: 0.85rem;
  font-weight: 500;
  padding: 0;
}

.filter-clear:hover {
  text-decoration: underline;
}

.no-results {
  text-align: center;
  padding: 3rem 1rem;
  color: var(--vp-c-text-2);
}

.no-results p {
  margin-bottom: 1rem;
}

/* Grid Layout */
.boards-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 1rem;
  margin-top: 1.5rem;
}

/* Card Design */
.board-card {
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  overflow: hidden;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  flex-direction: column;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
}

.board-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
  border-color: var(--vp-c-brand-1);
}

/* Image Section */
.board-card__image {
  background: var(--vp-c-bg-soft);
  padding: 1.5rem;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 120px;
  border-bottom: 1px solid var(--vp-c-divider);
  position: relative;
}

.popular-badge {
  position: absolute;
  top: 8px;
  right: 8px;
  background: #ff9800;
  color: #ffffff;
  font-size: 0.65rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
}

.board-card__image img {
  max-width: 100px;
  max-height: 100px;
  object-fit: contain;
}

/* Content Section */
.board-card__content {
  padding: 1.25rem;
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

/* Header */
.board-card__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 0.75rem;
  margin-bottom: 0.25rem;
}

.board-card__title {
  margin: 0;
  font-size: 1rem;
  font-weight: 600;
  line-height: 1.4;
  flex: 1;
}

.board-card__title code {
  background: transparent;
  color: var(--vp-c-brand-1);
  font-size: 0.9em;
  padding: 0;
  font-weight: 600;
}

.board-card__chip {
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-2);
  padding: 0.3rem 0.8rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-weight: 600;
  white-space: nowrap;
  flex-shrink: 0;
  border: 1px solid var(--vp-c-divider);
}

/* Description */
.board-card__description {
  margin: 0;
  font-size: 0.9rem;
  line-height: 1.5;
  color: var(--vp-c-text-2);

}

/* Libraries Section */
.board-card__libraries {
  margin-top: 0.5rem;
  padding-top: 0.25rem;
  border-top: 1px solid var(--vp-c-divider);
}

.modules-label-row {
  display: flex;
  align-items: center;
  gap: 0.2em;
  margin-bottom: 0.5rem;
}
.libraries-label {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--vp-c-text-2);
  display: inline-block;
}

.libraries-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
}

.lib-badge {
  display: inline-block;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  padding: 0.2rem 0.6rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-weight: 500;
  border: 1px solid var(--vp-c-divider);
}

/* Action Footer */
.board-card__action {
  padding-top: 0.75rem;
  margin-top: 0.5rem;
  border-top: 1px solid var(--vp-c-divider);
}

.action-text {
  color: var(--vp-c-brand-1);
  font-size: 0.85rem;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 0.25rem;
}

.board-card:hover .action-text {
  text-decoration: underline;
}

/* Loading Spinner */
.loading-spinner {
  display: inline-block;
  width: 16px;
  height: 16px;
  min-width: 16px;
  border: 2px solid #cfd8dc;
  border-top-color: var(--vp-c-brand-1);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  vertical-align: middle;
  margin-right: 0.5rem;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

/* Responsive */
@media (max-width: 768px) {
  .filter-groups {
    flex-direction: column;
    gap: 0.5rem;
  }

  .filter-group {
    flex-wrap: wrap;
  }

  .boards-grid {
    grid-template-columns: 1fr;
  }

  .board-card__image {
    min-height: 100px;
    padding: 1rem;
  }

  .board-card__image img {
    max-width: 80px;
    max-height: 80px;
  }
}

@media (max-width: 480px) {
  .board-card__header {
    flex-direction: column;
    align-items: flex-start;
  }

  .board-card__chip {
    align-self: flex-start;
  }
}
/* Espansione animata per badge */
.expand-fade-enter-active, .expand-fade-leave-active {
  transition: max-height 0.3s cubic-bezier(0.4, 0, 0.2, 1), opacity 0.3s;
}
.expand-fade-enter-from, .expand-fade-leave-to {
  opacity: 0;
  max-height: 0;
}
.expand-fade-enter-to, .expand-fade-leave-from {
  opacity: 1;
  max-height: 500px;
}

.expand-icon-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: var(--vp-c-brand-1);
  cursor: pointer;
  margin-top: 0.3em;
  margin-left: 0.2em;
  padding: 0.1em 0.2em;
  border-radius: 50%;
  transition: background 0.2s;
}
.expand-icon-btn:hover {
  background: rgba(62, 175, 124, 0.08);
}
.expand-icon-btn svg {
  transition: transform 0.25s cubic-bezier(0.4,0,0.2,1);
  vertical-align: middle;
}
.expand-icon-btn svg.rotated {
  transform: rotate(90deg);
}
</style>
