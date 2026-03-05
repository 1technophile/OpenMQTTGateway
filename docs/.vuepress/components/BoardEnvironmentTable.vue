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

    <div v-if="!loading && !error" class="boards-grid">
      <article
        v-for="board in boards"
        :key="board.environment"
        class="board-card"
        @click="openSelector(board.environment)">
        
        <div class="board-card__image">
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
      expandedLibraries: {}
    };
  },
  computed: {
    resolvedBoardsUrl() {
      return this.buildUrl(this.boardsUrl);
    },
    resolvedSelectorUrl() {
      return this.buildUrl(this.selectorPath);
    }
  },
  methods: {
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
        this.boards = data
          .filter(board => board && typeof board.environment === 'string')
          .sort((a, b) => a.environment.localeCompare(b.environment));
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

/* Grid Layout */
.boards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
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
