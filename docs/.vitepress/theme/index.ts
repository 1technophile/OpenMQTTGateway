// docs/.vitepress/theme/index.ts
import DefaultTheme from 'vitepress/theme'
import mediumZoom from 'medium-zoom'
import { onMounted, watch, nextTick } from 'vue'
import { useRoute } from 'vitepress'
import './custom.css'
import BoardEnvironmentTable from '../components/BoardEnvironmentTable.vue'
import FlashEnvironmentSelector from '../components/FlashEnvironmentSelector.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('BoardEnvironmentTable', BoardEnvironmentTable)
    app.component('FlashEnvironmentSelector', FlashEnvironmentSelector)
  },
  setup() {
    const route = useRoute()

    const initZoom = () => {
      mediumZoom('.vp-doc img:not(.no-zoom)', {
        background: 'var(--vp-c-bg)'
      })
    }

    onMounted(() => {
      initZoom()
    })

    watch(
      () => route.path,
      () => nextTick(() => initZoom())
    )
  }
}
