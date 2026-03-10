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

        <div class="filter-groups">
          <div class="filter-group">
            <span class="filter-group__label">MCU:</span>
            <div class="filter-chips">
              <button
                v-for="mcu in mcuFilters"
                :key="mcu"
                :class="['filter-chip', { 'filter-chip--active': activeMcuFilter === mcu }]"
                @click="toggleMcuFilter(mcu)">
                {{ mcu }}
              </button>
            </div>
          </div>
          <div class="filter-group">
            <span class="filter-group__label">Gateway:</span>
            <div class="filter-chips">
              <button
                v-for="gw in gatewayFilters"
                :key="gw.key"
                :class="['filter-chip', { 'filter-chip--active': activeGatewayFilter === gw.key }]"
                @click="toggleGatewayFilter(gw.key)">
                {{ gw.label }}
              </button>
            </div>
          </div>
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
          <span v-if="popularEnvironments.indexOf(board.environment) !== -1" class="popular-badge">Popular</span>
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
            <transition name="expand-fade">
              <div class="libraries-badges"
                :style="expandedModules[board.environment] ? '' : 'max-height: 2.2em; overflow: hidden;'">
                <span 
                  v-for="(mod, index) in board.modules" 
                  :key="index" 
                  class="lib-badge">
                  {{ mod }}
                </span>
              </div>
            </transition>
            
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
            <transition name="expand-fade">
              <div class="libraries-badges"
                :style="expandedLibraries[board.environment] ? '' : 'max-height: 2.2em; overflow: hidden;'">
                <span 
                  v-for="(lib, index) in board.libraries" 
                  :key="index" 
                  class="lib-badge">
                  {{ lib }}
                </span>
              </div>
            </transition>
            
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

<script>
export default {
  name: 'BoardEnvironmentTable',
  props: {
    boardsUrl: {
      type: String,
      default: '/boards-info.json'
    },
    selectorPath: {
      type: String,
      default: '/upload/board-selector.html'
    }
  },
  data() {
    return {
      boards: [],
      loading: false,
      error: null,
      expandedModules: {},
      expandedLibraries: {},
      searchQuery: '',
      activeMcuFilter: null,
      activeGatewayFilter: null,
      popularEnvironments: [
        'esp32dev-ble',
        'theengs-bridge-v11',
        'lilygo-rtl_433',
        'lilygo-rtl_433-fsk',
        'esp32dev-ir'
      ]
    };
  },
  computed: {
    resolvedBoardsUrl() {
      return this.buildUrl(this.boardsUrl);
    },
    resolvedSelectorUrl() {
      return this.buildUrl(this.selectorPath);
    },
    mcuFilters() {
      var families = {};
      this.boards.forEach(function(b) {
        var family = this.getMcuFamily(b.microcontroller);
        if (family) families[family] = true;
      }.bind(this));
      return Object.keys(families).sort();
    },
    gatewayFilters() {
      var gateways = {
        BT: 'BLE',
        RF: 'RF',
        IR: 'IR',
        LORA: 'LoRa',
        RTL_433: 'RTL_433',
        Pilight: 'Pilight'
      };
      var result = [];
      var self = this;
      Object.keys(gateways).forEach(function(key) {
        var hasBoards = self.boards.some(function(b) {
          return Array.isArray(b.modules) && b.modules.some(function(m) {
            return m.toLowerCase().indexOf(key.toLowerCase()) !== -1;
          });
        });
        if (hasBoards) {
          result.push({ key: key, label: gateways[key] });
        }
      });
      return result;
    },
    hasActiveFilters() {
      return this.searchQuery || this.activeMcuFilter || this.activeGatewayFilter;
    },
    filteredBoards() {
      var self = this;
      return this.boards.filter(function(board) {
        if (self.searchQuery) {
          var q = self.searchQuery.toLowerCase();
          var haystack = [
            board.environment,
            board.description || '',
            board.microcontroller || '',
            (board.modules || []).join(' ')
          ].join(' ').toLowerCase();
          if (haystack.indexOf(q) === -1) return false;
        }
        if (self.activeMcuFilter) {
          var family = self.getMcuFamily(board.microcontroller);
          if (family !== self.activeMcuFilter) return false;
        }
        if (self.activeGatewayFilter) {
          var key = self.activeGatewayFilter.toLowerCase();
          if (!Array.isArray(board.modules) || !board.modules.some(function(m) {
            return m.toLowerCase().indexOf(key) !== -1;
          })) return false;
        }
        return true;
      });
    }
  },
  methods: {
    getMcuFamily(mcu) {
      if (!mcu) return null;
      var m = mcu.toLowerCase();
      if (m.indexOf('esp32-s3') !== -1 || m.indexOf('esp32s3') !== -1 || m.indexOf('atoms3') !== -1 || m.indexOf('lilygo-t3-s3') !== -1) return 'ESP32-S3';
      if (m.indexOf('esp32-c3') !== -1 || m.indexOf('esp32c3') !== -1 || m.indexOf('lolin_c3') !== -1 || m.indexOf('airm2m') !== -1) return 'ESP32-C3';
      if (m.indexOf('esp32') !== -1 || m.indexOf('m5st') !== -1 || m.indexOf('heltec') !== -1 || m.indexOf('ttgo') !== -1 || m.indexOf('lolin32') !== -1 || m.indexOf('pico32') !== -1 || m.indexOf('tinypico') !== -1 || m.indexOf('feather') !== -1) return 'ESP32';
      if (m.indexOf('esp8') !== -1 || m.indexOf('nodemcu') !== -1) return 'ESP8266';
      return 'Other';
    },
    toggleMcuFilter(mcu) {
      this.activeMcuFilter = this.activeMcuFilter === mcu ? null : mcu;
    },
    toggleGatewayFilter(gw) {
      this.activeGatewayFilter = this.activeGatewayFilter === gw ? null : gw;
    },
    clearFilters() {
      this.searchQuery = '';
      this.activeMcuFilter = null;
      this.activeGatewayFilter = null;
    },
    buildUrl(path) {
      const base = this.$site?.base || '/';
      const cleanBase = base.endsWith('/') ? base.slice(0, -1) : base;
      const cleanPath = path.startsWith('/') ? path : `/${path}`;
      return `${cleanBase}${cleanPath}`;
    },
    async loadBoards() {
      this.loading = true;
      this.error = null;
      try {
        const response = await fetch(this.resolvedBoardsUrl);
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        const data = await response.json();
        if (!Array.isArray(data)) {
          throw new Error('boards-info.json must be an array');
        }
        var self = this;
        this.boards = data
          .filter(board => board && typeof board.environment === 'string')
          .sort(function(a, b) {
            var aPopular = self.popularEnvironments.indexOf(a.environment) !== -1;
            var bPopular = self.popularEnvironments.indexOf(b.environment) !== -1;
            if (aPopular && !bPopular) return -1;
            if (!aPopular && bPopular) return 1;
            return a.environment.localeCompare(b.environment);
          });
      } catch (err) {
        console.error('Failed to load boards-info:', err);
        this.error = err.message || 'Unable to load board information';
      } finally {
        this.loading = false;
      }
    },
    toggleModules(env) {
      this.$set(this.expandedModules, env, !this.expandedModules[env]);
    },
    toggleLibraries(env) {
      this.$set(this.expandedLibraries, env, !this.expandedLibraries[env]);
    },
    openSelector(environment) {
      if (!environment) return;
      const url = `${this.resolvedSelectorUrl}?env=${encodeURIComponent(environment)}`;
      window.location.href = url;
    },
    getBoardImageUrl(board) {
      const customImg = board.customImg || board.CustomImg;
      if (customImg) {
        // If it's an absolute URL, use it as-is
        if (customImg.startsWith('http')) {
          return customImg;
        }
        // Otherwise, build the URL with site base
        return this.buildUrl(customImg);
      }
      return this.buildUrl('/img/microcontroller.gif');
    }
  },
  mounted() {
    this.loadBoards();
  }
};
</script>

<style scoped>
.board-environment-list {
  margin: 2rem 0;
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
  color: var(--text-color-secondary, #666);
  pointer-events: none;
}

.search-input {
  width: 100%;
  padding: 0.6rem 2.2rem 0.6rem 2.4rem;
  border: 1px solid var(--border-color, #eaecef);
  border-radius: 8px;
  font-size: 0.9rem;
  background: #ffffff;
  color: var(--text-color, #2c3e50);
  transition: border-color 0.2s;
}

.search-input:focus {
  outline: none;
  border-color: var(--accent-color, #3eaf7c);
  box-shadow: 0 0 0 3px rgba(62, 175, 124, 0.15);
}

.search-input::placeholder {
  color: var(--text-color-secondary, #999);
}

.search-clear {
  position: absolute;
  right: 8px;
  background: none;
  border: none;
  cursor: pointer;
  padding: 4px;
  color: var(--text-color-secondary, #666);
  border-radius: 50%;
  display: flex;
  align-items: center;
}

.search-clear:hover {
  background: rgba(0, 0, 0, 0.05);
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
  color: var(--text-color-secondary, #666);
  white-space: nowrap;
}

.filter-chips {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
}

.filter-chip {
  background: var(--code-bg-color, #f6f8fa);
  color: var(--text-color, #2c3e50);
  border: 1px solid var(--border-color, #eaecef);
  padding: 0.25rem 0.7rem;
  border-radius: 16px;
  font-size: 0.78rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.filter-chip:hover {
  border-color: var(--accent-color, #3eaf7c);
  color: var(--accent-color, #3eaf7c);
}

.filter-chip--active {
  background: var(--accent-color, #3eaf7c);
  color: #ffffff;
  border-color: var(--accent-color, #3eaf7c);
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
  color: var(--text-color-secondary, #666);
}

.filter-clear {
  background: none;
  border: none;
  color: var(--accent-color, #3eaf7c);
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
  color: var(--text-color-secondary, #666);
}

.no-results p {
  margin-bottom: 1rem;
}

/* Grid Layout */
.boards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
  gap: 1.5rem;
  margin-top: 1.5rem;
}

/* Card Design */
.board-card {
  background: #ffffff;
  border: 1px solid var(--border-color, #eaecef);
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
  border-color: var(--accent-color, #3eaf7c);
}

/* Image Section */
.board-card__image {
  background: var(--code-bg-color, #f6f8fa);
  padding: 1.5rem;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 120px;
  border-bottom: 1px solid var(--border-color, #eaecef);
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
  color: var(--accent-color, #3eaf7c);
  font-size: 0.9em;
  padding: 0;
  font-weight: 600;
}

.board-card__chip {
  background: var(--code-bg-color, #f6f8fa);
  color: var(--text-color-secondary, #666);
  padding: 0.3rem 0.8rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-weight: 600;
  white-space: nowrap;
  flex-shrink: 0;
  border: 1px solid var(--border-color, #eaecef);
}

/* Description */
.board-card__description {
  margin: 0;
  font-size: 0.9rem;
  line-height: 1.5;
  color: var(--text-color-secondary, #666);

}

/* Libraries Section */
.board-card__libraries {
  margin-top: 0.5rem;
  padding-top: 0.25rem;
  border-top: 1px solid var(--border-color, #eaecef);
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
  color: var(--text-color-secondary, #666);
  display: inline-block;
}

.libraries-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
}

.lib-badge {
  display: inline-block;
  background: var(--code-bg-color, #f6f8fa);
  color: var(--text-color, #2c3e50);
  padding: 0.2rem 0.6rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-weight: 500;
  border: 1px solid var(--border-color, #eaecef);
}

/* Action Footer */
.board-card__action {
  padding-top: 0.75rem;
  margin-top: 0.5rem;
  border-top: 1px solid var(--border-color, #eaecef);
}

.action-text {
  color: var(--accent-color, #3eaf7c);
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
  border-top-color: var(--accent-color, #3eaf7c);
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
    gap: 1rem;
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
.expand-fade-enter, .expand-fade-leave-to {
  opacity: 0;
  max-height: 0;
}
.expand-fade-enter-to, .expand-fade-leave {
  opacity: 1;
  max-height: 500px;
}

.expand-icon-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: var(--accent-color, #3eaf7c);
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
