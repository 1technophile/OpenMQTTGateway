<template>
  <div class="flash-selector">
    <!-- Loading State -->
    <div v-if="loading" class="custom-block tip">
      <p class="custom-block-title">
        <span class="loading-spinner"></span>
        Loading environments...
      </p>
    </div>

    <!-- Error State -->
    <div v-if="error" class="custom-block danger">
      <p class="custom-block-title">Error</p>
      <p>{{ error }}</p>
    </div>

    <!-- Board Details (when selected) -->
    <Transition name="fade">
      <div v-if="currentEnvironment && !loading" class="board-details">

        <!-- Hero Section - Clean and Focused -->
        <div class="board-hero">
          <div class="board-hero__content">
            <!-- Inline Board Switcher -->
            <h2>
              <div class="board-switcher">
                <label for="envSelect" class="switcher-label">Environment:</label>
                <div class="switcher-select-wrapper">
                  <select
                    id="envSelect"
                    v-model="selectedEnvironment"
                    class="switcher-select">
                    <option
                      v-for="env in environments"
                      :key="env.environment"
                      :value="env.environment">
                      {{ env.environment }}
                    </option>
                  </select>
                  <span class="switcher-icon">⚙️</span>
                </div>
              </div>
            </h2>

            <div v-if="currentEnvironment.microcontroller" class="board-hero__meta">
              <span class="meta-label">MCU:</span>
              <span class="meta-value">{{ currentEnvironment.microcontroller }}</span>
            </div>

            <p v-if="currentEnvironment.description" class="board-hero__description">
             <span class="meta-label">Description:</span> <span v-html="currentEnvironment.description"></span>
            </p>

            <!-- Primary Action -->
            <div class="hero-action">
              <esp-web-install-button
                v-if="manifestUrl"
                :manifest="manifestUrl"
                aria-label="Flash firmware to device">
              </esp-web-install-button>
              <button v-else class="connect-button connect-button--disabled" disabled>
                CONNECT
              </button>
            </div>
          </div>

          <!-- Image Optional -->
          <div v-if="boardImageUrl && !imageError" class="board-hero__image">
            <img
              :src="boardImageUrl"
              :alt="currentEnvironment.environment"
              loading="lazy"
              @error="handleImageError">
          </div>
        </div>

        <!-- Technical Details - Collapsible -->
        <details class="tech-details" open>
          <summary class="tech-details__summary">
            <span class="tech-details__icon">📋</span>
            <span class="tech-details__title">Technical Information</span>
            <span class="tech-details__toggle">▼</span>
          </summary>

          <div class="tech-details__content">
            <div class="tech-grid">

              <!-- Libraries -->
              <div v-if="currentEnvironment.libraries && currentEnvironment.libraries.length" class="tech-section">
                <h4 class="tech-section__title">Libraries</h4>
                <div class="tech-section__content">
                  <div class="chip-group">
                    <span v-for="(lib, index) in currentEnvironment.libraries" :key="index" class="chip">
                      {{ lib }}
                    </span>
                  </div>
                </div>
              </div>

              <!-- Gateway Modules -->
              <div v-if="currentEnvironment.modules && currentEnvironment.modules.length" class="tech-section">
                <h4 class="tech-section__title">Gateway Modules</h4>
                <div class="tech-section__content">
                  <div class="chip-group">
                    <span v-for="(mod, index) in currentEnvironment.modules" :key="index" class="chip">
                      {{ mod }}
                    </span>
                  </div>
                </div>
              </div>

              <!-- Hardware -->
              <div v-if="currentEnvironment.hardware" class="tech-section">
                <h4 class="tech-section__title">Hardware</h4>
                <div class="tech-section__content" >
                  <span class="chip" v-html="currentEnvironment.hardware"></span>
                </div>
              </div>

              <!-- Partitions -->
              <div v-if="currentEnvironment.partitions" class="tech-section">
                <h4 class="tech-section__title">Partitions</h4>
                <div class="tech-section__content">
                  <code class="code-inline">{{ currentEnvironment.partitions }}</code>
                </div>
              </div>

              <!-- Build Options -->
              <div v-if="currentEnvironment.options" class="tech-section tech-section--full">
                <h4 class="tech-section__title">Build Options</h4>
                <div class="tech-section__content">
                  <pre class="code-block">{{ formatOptions(currentEnvironment.options) }}</pre>
                </div>
              </div>
            </div>
          </div>
        </details>
      </div>

    </Transition>
    <!-- Board Detail not selected message -->
    <Transition name="fade">
      <div v-if="!currentEnvironment && !loading && !error" class="custom-block info">
        <p class="custom-block-title">Need to select one environment...</p>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'

interface Environment {
  environment: string
  description?: string
  microcontroller?: string
  modules?: string[]
  libraries?: string[]
  hardware?: string
  partitions?: string
  options?: string | string[]
  customImg?: string
  CustomImg?: string
}

const props = withDefaults(defineProps<{
  boardsUrl?: string
  firmwareBaseUrl?: string
}>(), {
  boardsUrl: '/boards-info.json',
  firmwareBaseUrl: '/firmware_build/'
})

const environments = ref<Environment[]>([])
const selectedEnvironment = ref<string>('')
const loading = ref<boolean>(true)
const error = ref<string | null>(null)
const imageError = ref<boolean>(false)

const currentEnvironment = computed<Environment | undefined>(() => {
  return environments.value.find(env => env.environment === selectedEnvironment.value)
})

const resolvedBoardsUrl = computed<string>(() => buildUrl(props.boardsUrl))

const resolvedFirmwareBaseUrl = computed<string>(() => buildUrl(props.firmwareBaseUrl))

const manifestUrl = computed<string | null>(() => {
  if (!selectedEnvironment.value) return null
  return `${resolvedFirmwareBaseUrl.value}${selectedEnvironment.value}.manifest.json`
})

const boardImageUrl = computed<string | null>(() => {
  if (!currentEnvironment.value) return null
  const customImg = currentEnvironment.value.customImg || currentEnvironment.value.CustomImg
  if (customImg && !imageError.value) {
    if (customImg.startsWith('http')) {
      return customImg
    }
    return buildUrl(customImg)
  }
  return buildUrl('/img/microcontroller.gif')
})

watch(selectedEnvironment, () => {
  imageError.value = false
  error.value = null
})

onMounted(() => {
  const script = document.createElement('script')
  script.type = 'module'
  script.src = 'https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module'
  document.head.appendChild(script)
  loadEnvironments()
})

function buildUrl(path: string): string {
  const base = import.meta.env.BASE_URL || '/'
  const cleanBase = base.endsWith('/') ? base.slice(0, -1) : base
  const cleanPath = path.startsWith('/') ? path : `/${path}`
  return `${cleanBase}${cleanPath}`
}

function getQueryParam(param: string): string | null {
  const params = new URLSearchParams(window.location.search)
  return params.get(param)
}

async function loadEnvironments(): Promise<void> {
  loading.value = true
  error.value = null

  try {
    const response = await fetch(resolvedBoardsUrl.value)

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`)
    }

    const data = await response.json()

    if (!Array.isArray(data)) {
      throw new Error('Invalid data format: expected an array')
    }

    const validEnvironments = data.filter((env: unknown) =>
      env &&
      typeof env === 'object' &&
      (env as Environment).environment &&
      typeof (env as Environment).environment === 'string'
    ) as Environment[]

    if (validEnvironments.length === 0) {
      throw new Error('No valid environments found in data')
    }

    environments.value = validEnvironments

    const envParam = getQueryParam('env')
    if (envParam) {
      const matchingEnv = validEnvironments.find(env => env.environment === envParam)
      if (matchingEnv) {
        selectedEnvironment.value = envParam
      } else {
        console.warn(`Environment '${envParam}' not found in available environments`)
      }
    }
  } catch (err) {
    console.error('Failed to load environments:', err)
    error.value = (err instanceof Error ? err.message : null) || 'Failed to load environments'
  } finally {
    loading.value = false
  }
}

function formatOptions(optionsText: string | string[]): string {
  if (!optionsText) return ''
  let optionsTextMap: string[] = []
  if (typeof optionsText === 'string') {
    optionsTextMap = optionsText.split('\n')
  } else if (Array.isArray(optionsText)) {
    optionsTextMap = optionsText
  }

  return optionsTextMap.map(line => line.trim())
    .filter(line => line.length > 0)
    .join('\n')
}

function handleImageError(): void {
  imageError.value = true
}
</script>

<style scoped>
/* Main Container */
.flash-selector {
  max-width: 1200px;
  margin: 2rem auto;
  padding: 0 1rem;
}

/* Board Details Container */
.board-details {
  margin-top: 2rem;
}

/* Hero Section - Material Design 3 */
.board-hero {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 24px;
  background: var(--vp-c-bg);
  border-radius: 12px;
  padding: 32px;
  margin-bottom: 16px;
  box-shadow: 0 1px 2px 0 var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
}

.board-hero__content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.board-hero__title {
  margin: 0 0 8px 0;
  font-size: 1.75rem;
  font-weight: 400;
  color: var(--vp-c-brand-1);
  font-family: 'Monaco', 'Courier New', monospace;
  line-height: 1.2;
  letter-spacing: -0.01em;
}

/* Board Switcher - Material Chip Style */
.board-switcher {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 0;
}

.switcher-label {
  font-size: 0.75rem;
  text-transform: uppercase;
  font-weight: 500;
  letter-spacing: 0.1em;
  color: var(--vp-c-text-2);
}

.switcher-select-wrapper {
  position: relative;
  display: inline-flex;
  align-items: center;
}

.switcher-select {
  appearance: none;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  border: none;
  padding: 6px 32px 6px 12px;
  border-radius: 16px;
  font-size: 0.875rem;
  font-weight: 500;
  font-family: 'Monaco', 'Courier New', monospace;
  cursor: pointer;
  transition: background 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  min-height: 32px;
  min-width: 280px;
  width: auto;
}

.switcher-select:hover {
  background: var(--vp-c-bg-soft);
}

.switcher-select:focus {
  outline: 2px solid var(--vp-c-brand-1);
  outline-offset: 2px;
}

.switcher-icon {
  position: absolute;
  right: 8px;
  pointer-events: none;
  font-size: 1rem;
  opacity: 0.6;
}

.board-hero__meta {
  display: flex;
  align-items: baseline;
  gap: 8px;
  margin: 0;
}

.meta-label {
  font-size: 0.75rem;
  text-transform: uppercase;
  font-weight: 500;
  letter-spacing: 0.1em;
  color: var(--vp-c-text-2);
}

.meta-value {
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--vp-c-text-1);
  font-family: 'Monaco', 'Courier New', monospace;
}

.board-hero__description {
  margin: 0;
  font-size: 1rem;
  line-height: 1.5;
  color: var(--vp-c-text-1);
  max-width: 600px;
}

.board-hero__image {
  display: flex;
  align-items: flex-start;
  justify-content: center;
  min-width: 180px;
  max-width: 200px;
}

.board-hero__image img {
  max-width: 100%;
  max-height: 180px;
  object-fit: contain;
}

/* Primary Action - Material Filled Button */
.hero-action {
  margin-top: 8px;
}

.connect-button {
  background: #1976d2;
  color: #ffffff;
  border: none;
  padding: 10px 24px;
  border-radius: 20px;
  font-size: 0.875rem;
  font-weight: 500;
  cursor: pointer;
  transition: box-shadow 0.28s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.3), 0 1px 3px 1px rgba(0, 0, 0, 0.15);
  text-transform: uppercase;
  letter-spacing: 0.125em;
  height: 40px;
  min-width: 120px;
}

.connect-button:hover:not(:disabled) {
  box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.3), 0 2px 6px 2px rgba(0, 0, 0, 0.15);
}

.connect-button:active:not(:disabled) {
  box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.3), 0 1px 3px 1px rgba(0, 0, 0, 0.15);
}

.connect-button--disabled {
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-3);
  cursor: not-allowed;
  box-shadow: none;
}

/* Technical Details - Material Surface */
.tech-details {
  background: var(--vp-c-bg);
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 1px 2px 0 var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
}

.tech-details__summary {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px 24px;
  cursor: pointer;
  user-select: none;
  background: var(--vp-c-bg-alt);
  transition: background 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  list-style: none;
}

.tech-details__summary::-webkit-details-marker {
  display: none;
}

.tech-details__summary:hover {
  background: var(--vp-c-bg-soft);
}

.tech-details__icon {
  font-size: 1.25rem;
  opacity: 0.6;
}

.tech-details__title {
  font-size: 1rem;
  font-weight: 500;
  color: var(--vp-c-text-1);
  flex: 1;
  letter-spacing: 0.01em;
}

.tech-details__toggle {
  font-size: 0.875rem;
  transition: transform 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  opacity: 0.6;
}

.tech-details[open] .tech-details__toggle {
  transform: rotate(180deg);
}

.tech-details__content {
  padding: 24px;
}

/* Tech Grid */
.tech-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 24px;
}

.tech-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.tech-section--full {
  grid-column: 1 / -1;
}

.tech-section__title {
  margin: 0;
  font-size: 0.75rem;
  text-transform: uppercase;
  font-weight: 500;
  letter-spacing: 0.1em;
  color: var(--vp-c-text-2);
}

.tech-section__content {
  font-size: 0.875rem;
  line-height: 1.5;
  color: var(--vp-c-text-1);
}

/* Chips - Material Design */
.chip-group {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.chip {
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  padding: 6px 12px;
  border-radius: 8px;
  font-size: 0.875rem;
  font-weight: 400;
  border: none;
  height: 32px;
  display: inline-flex;
  align-items: center;
}

/* Code Styles - Material Surface */
.code-inline {
  background: var(--vp-c-bg-soft);
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 0.875rem;
  font-family: 'Monaco', 'Courier New', monospace;
  color: var(--vp-c-text-1);
}

.code-block {
  background: var(--vp-c-bg-soft);
  padding: 16px;
  border-radius: 8px;
  font-size: 0.8125rem;
  line-height: 1.5;
  overflow-x: auto;
  margin: 0;
  border: 1px solid var(--vp-c-divider);
  white-space: pre-wrap;
  word-break: break-word;
  font-family: 'Monaco', 'Courier New', monospace;
  color: var(--vp-c-text-1);
}

/* Loading Spinner */
.loading-spinner {
  display: inline-block;
  width: 18px;
  height: 18px;
  border: 2px solid rgba(62, 175, 124, 0.3);
  border-top-color: var(--vp-c-brand-1);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  vertical-align: middle;
  margin-right: 0.5rem;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

/* Fade Transition */
.fade-enter-active, .fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from, .fade-leave-to {
  opacity: 0;
}

/* Accessibility */
.visually-hidden {
  position: absolute;
  width: 1px;
  height: 1px;
  margin: -1px;
  padding: 0;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  border: 0;
}

/* Responsive Design */
@media (max-width: 968px) {
  .board-hero {
    grid-template-columns: 1fr;
  }

  .board-hero__image {
    order: -1;
    max-width: 150px;
    margin: 0 auto;
  }

  .tech-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .selector-card {
    padding: 1.5rem;
  }

  .board-hero {
    padding: 1.5rem;
  }

  .board-hero__title {
    font-size: 1.5rem;
  }

  .connect-button {
    width: 100%;
    padding: 0.9rem 2rem;
  }
}
</style>
