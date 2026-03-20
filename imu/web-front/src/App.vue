<template>
  <section class="section has-background-light p-3" style="min-height: 100vh;">
    
    <!-- SIDEBAR MENU OVERLAY -->
    <div :class="['sidebar-menu', { 'is-active': showMenu }]">
      <div class="p-5">
        <div class="is-flex is-justify-content-space-between mb-5">
          <p class="title is-5 has-text-white mb-0">MENU</p>
          <button class="delete is-medium" @click="showMenu = false"></button>
        </div>
        <aside class="menu">
          <ul class="menu-list">
            <li><a class="has-text-white" @click="setView('dashboard')">Dashboard</a></li>
            <li><a class="has-text-white" @click="setView('calibration')">IMU Calibration</a></li>
          </ul>
        </aside>
      </div>
    </div>

    <!-- SIDEBAR MASK -->
    <div v-if="showMenu" class="sidebar-mask" @click="showMenu = false"></div>

    <div class="container">
      <!-- HEADER ROW: FIXED 3-COLUMN BOX -->
      <div class="is-flex is-align-items-center mb-5" style="min-height: 3.5rem;">
        
        <!-- LEFT: Fixed width for icon (Ensures clickability) -->
        <div style="width: 48px; flex-shrink: 0; z-index: 10;">
          <a @click.stop="showMenu = true" class="has-text-black is-flex is-align-items-center" style="cursor: pointer;">
            <Icon icon="mdi:menu" width="32" height="32" />
          </a>
        </div>

        <!-- CENTER: Title restricted to middle space -->
        <div style="flex-grow: 1; text-align: center; overflow: hidden;">
          <h1 class="title is-size-4-mobile is-size-3-tablet has-text-black has-text-weight-black m-0" style="white-space: nowrap;">
            {{ currentView === 'dashboard' ? 'IMU Status' : 'Calibration' }}
          </h1>
        </div>

        <!-- RIGHT: Invisible spacer to balance the icon -->
        <div style="width: 48px; flex-shrink: 0;"></div>
      </div>

      <!-- VIEW SWITCHER -->
      <DashboardView v-show="currentView === 'dashboard'" ref="dashboardRef" />
      <CalibrationView v-show="currentView === 'calibration'" ref="calibrationRef"/>
      
    </div>
  </section>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { Icon } from '@iconify/vue'
import { useIMUStore } from './store/imuStore'
import DashboardView from './components/DashboardView.vue'
import CalibrationView from './components/CalibrationView.vue'

const { updateIMU, updateSystemStats, setConnected } = useIMUStore()
const SIM_MODE = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'

const showMenu = ref(false)
const currentView = ref('dashboard')

const dashboardRef = ref(null)
const calibrationRef = ref(null)

let socket = null
let simTimer = null

const setView = (v) => { 
  currentView.value = v
  showMenu.value = false 
}

const handleIncomingData = (r, p, y, g, a, m) => {
  // Update Global Store (Source of Truth)
  updateIMU(r, p, y, g, a, m)

  // Update UI Chart/3D (If Dashboard is active)
  if (dashboardRef.value) {
    dashboardRef.value.updateChart(r, p, y)
  }

  if (calibrationRef.value) {
    calibrationRef.value.updateCalibrationCharts()
  }
}

const connectWebSocket = () => {
  socket = new WebSocket(`ws://${window.location.hostname}/ws_imu`)
  socket.binaryType = "arraybuffer"
  socket.onopen = () => setConnected(true)
  
  socket.onmessage = (event) => {
    const v = new DataView(event.data)
    
    // 0-11: Fused Attitude (Floats)
    const r = v.getFloat32(0, true), p = v.getFloat32(4, true), y = v.getFloat32(8, true)
    
    // 12-47: Sensor Vectors (3x3 Floats = 36 bytes)
    const g = { x: v.getFloat32(12, true), y: v.getFloat32(16, true), z: v.getFloat32(20, true) }
    const a = { x: v.getFloat32(24, true), y: v.getFloat32(28, true), z: v.getFloat32(32, true) }
    const m = { x: v.getFloat32(36, true), y: v.getFloat32(40, true), z: v.getFloat32(44, true) }

    // 48-71: System Performance & Hardware Health (Shifted)
    updateSystemStats(
      v.getInt32(48, true), v.getInt32(52, true), 
      v.getBigUint64(56, true), v.getBigUint64(64, true)
    )
    
    // Push through the single pipe
    handleIncomingData(r, p, y, g, a, m)
  }

  socket.onclose = () => {
    setConnected(false); socket = null
    if (!SIM_MODE) setTimeout(connectWebSocket, 2000)
  }
}

// 5. Lifecycle Hooks
onMounted(() => {
  if (SIM_MODE) {
    setConnected(true)
    simTimer = setInterval(() => {
      const r = Math.sin(Date.now()/1000)*45
      const p = Math.cos(Date.now()/1000)*45
      const y = (Date.now()/100)%360

      const g = {
        x: Math.sin(Date.now()/1000)*45,
        y: Math.sin(Date.now()/1000)*90,
        z: Math.sin(Date.now()/1000)*135
      }
      const a = {
        x: Math.sin(Date.now()/1000)*90,
        y: Math.sin(Date.now()/1000)*45,
        z: Math.sin(Date.now()/1000)*135
      }
      const m = {
        x: Math.sin(Date.now()/1000)*135,
        y: Math.sin(Date.now()/1000)*490,
        z: Math.sin(Date.now()/1000)*45
      }


      // Simulate vectors in dev mode
      handleIncomingData(r, p, y, g, a, m)
    }, 20)
  } else {
    connectWebSocket()
  }
})

onUnmounted(() => {
  if (socket) socket.close()
  if (simTimer) clearInterval(simTimer)
})
</script>


<style scoped>
/* Sidebar Shell */
.sidebar-menu { 
  position: fixed; 
  top: 0; 
  left: -300px; 
  width: 300px; 
  height: 100vh; 
  background: #1a1a1a; 
  z-index: 1001; 
  transition: left 0.3s cubic-bezier(0.4, 0, 0.2, 1); 
  box-shadow: 5px 0 25px rgba(0,0,0,0.5); 
}
.sidebar-menu.is-active { left: 0; }
.sidebar-mask { 
  position: fixed; 
  top: 0; 
  left: 0; 
  width: 100vw; 
  height: 100vh; 
  background: rgba(0, 0, 0, 0.4); 
  z-index: 1000; 
}
.menu-list a:hover { background-color: #333 !important; }
</style>
