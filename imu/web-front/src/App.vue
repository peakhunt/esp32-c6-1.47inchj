<template>
  <div class="app-shell">
    <!-- 1. PERMANENT HEADER -->
    <header class="app-header px-4">
      <div class="is-flex is-align-items-center is-justify-content-space-between h-100">
        <!-- Menu Toggle -->
        <div class="header-side">
          <a class="has-text-white is-flex"
             :class="{ 'is-disabled-click': imuStore.state.isCalibrating }"
             @click="!imuStore.state.isCalibrating && (showMenu = true)"
           >
            <Icon icon="mdi:menu" width="28" height="28" />
          </a>
        </div>

        <!-- Dynamic Title -->
        <div class="header-center">
          <h1 class="title is-5 has-text-white m-0">
            {{ currentView === 'dashboard' ? 'IMU STATUS' : 'SENSOR CALIBRATION' }}
          </h1>
        </div>

        <!-- Replace your current "Hardware Busy" div with this -->
        <div v-if="imuStore.state.isCalibrating" class="is-flex is-align-items-center ml-3 px-2 py-1 rounded-sm" style="background: rgba(255,193,7,0.1); border-radius: 4px;">
          <span class="loader is-loading mr-2" style="width: 14px; height: 14px; border-color: transparent transparent #ffdd57 #ffdd57 !important;"></span>
          <div class="is-flex is-flex-direction-column" style="line-height: 1;">
            <span class="is-size-7 has-text-warning has-text-weight-bold uppercase tracking-wide">
              CALIBRATING...
            </span>
            <span class="is-family-monospace has-text-white" style="font-size: 0.65rem;">
              {{ elapsedSeconds }}s ELAPSED
            </span>
          </div>
        </div>


        <!-- Live Stats (Updated with your Tag Indicator) -->
        <div class="header-side has-text-right">
          <div :class="['status-indicator', imuStore.state.stats.connected ? 'is-live' : 'is-off']">
            <span class="tag is-rounded has-text-weight-bold is-size-7">
              {{ imuStore.state.stats.connected ? 'LIVE' : 'OFFLINE' }}
            </span>
          </div>
        </div>
      </div>
    </header>


    <!-- 2. SCROLLABLE CONTENT AREA -->
    <main class="app-content has-background-light">
      <div class="container p-3">
        <DashboardView v-show="currentView === 'dashboard'" ref="dashboardRef" />
        <CalibrationView v-show="currentView === 'calibration'" ref="calibrationRef" />
      </div>
    </main>

    <!-- FOOTER SECTION -->
    <footer class="app-footer px-4 is-size-7">
      <div class="is-flex is-justify-content-space-between is-align-items-center h-100">
        <!-- LEFT: Static Device Info (Hidden on mobile to save space) -->
        <div class="has-text-grey is-hidden-mobile">
          ESP32-C6-MPU9250
        </div>

        <!-- RIGHT: Stats - Wraps gracefully on mobile -->
        <div class="is-flex is-align-items-center is-flex-wrap-wrap is-justify-content-end" style="gap: 0.5rem 1rem;">
          <div class="has-text-grey">
            CPU: <span class="is-family-monospace has-text-black">{{ imuStore.state.cpuUsage }}%</span>
          </div>
          <div class="has-text-grey">
            Sampling Rate: <span class="is-family-monospace has-text-black">{{ imuStore.state.samplingRate }}Hz</span>
          </div>
          <div class="has-text-grey">
            I2C Transactions: <span class="is-family-monospace has-text-black">{{ imuStore.state.i2cTransactions }}</span>
          </div>
          <div :class="imuStore.state.i2cFailed > 0 ? 'has-text-danger' : 'has-text-grey-light'">
            I2C Fail: <span class="is-family-monospace">{{ imuStore.state.i2cFailed }}</span>
          </div>
        </div>
      </div>
    </footer>



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

    <!-- SIDEBAR & MASK (Outside layout flow) -->
    <div v-if="showMenu" class="sidebar-mask" @click="showMenu = false"></div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { Icon } from '@iconify/vue'
import { useIMUStore } from './store/imuStore'
import DashboardView from './components/DashboardView.vue'
import CalibrationView from './components/CalibrationView.vue'

const SIM_MODE = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'

const showMenu = ref(false)
const currentView = ref('dashboard')

const dashboardRef = ref(null)
const calibrationRef = ref(null)

let socket = null
let simTimer = null

const imuStore = useIMUStore()
const { updateIMU, updateSystemStats, setConnected } = imuStore

const elapsedSeconds = ref(0)
let timerInterval = null


watch(() => imuStore.state.isCalibrating, (isBusy) => {
  if (isBusy) {
    elapsedSeconds.value = 0
    timerInterval = setInterval(() => {
      elapsedSeconds.value++
    }, 1000)
  } else {
    clearInterval(timerInterval)
    // Keep the final time visible for a second or just reset
    // elapsedSeconds.value = 0 
  }
})

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
      const noise = () => (Math.random() - 0.5) * 0.3

      const r = Math.sin(Date.now()/1000)*45 + noise()
      const p = Math.cos(Date.now()/1000)*45 + noise()
      const y = (Date.now()/100)%360 + noise()

      const g = {
        x: Math.sin(Date.now()/1000)*45 + noise(),
        y: Math.sin(Date.now()/1000)*90 + noise(),
        z: Math.sin(Date.now()/1000)*135 + noise()
      }
      const a = {
        x: Math.sin(Date.now() / 1000) * 16 * noise(),
        y: Math.cos(Date.now() / 1200) * 16 * noise(),
        z: Math.sin(Date.now() / 800)  * 16 * noise()
      }

      const t = Date.now() / 1000;
      const radius = 50.0; // The Earth's field strength in uT

      const phi = (t * 7.9) + Math.sin(t * 13.7) * 2.0;   // Chaotic Longitude
      const theta = (t * 5.3) + Math.cos(t * 11.3) * 1.5; // Chaotic Latitude

      const m = {
        x: radius * Math.sin(theta) * Math.cos(phi),
        y: radius * Math.sin(theta) * Math.sin(phi),
        z: radius * Math.cos(theta)
      };

      // Simulate vectors in dev mode
      handleIncomingData(r, p, y, g, a, m)
    }, 20)
  } else {
    connectWebSocket()
  }
})

onUnmounted(() => {
  if (timerInterval) clearInterval(timerInterval)
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

.app-shell {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden; /* Prevents body bounce on mobile */
}

.app-header {
  height: 3.5rem;
  background: #1a1a1a;
  flex-shrink: 0; /* Won't squash */
  z-index: 10;
  box-shadow: 0 2px 10px rgba(0,0,0,0.2);
}

.app-content {
  flex-grow: 1; /* Takes all available space */
  overflow-y: auto; /* Internal scroll only */
  -webkit-overflow-scrolling: touch;
}

.app-footer {
  min-height: 2rem; /* Minimum height */
  height: auto;     /* Allows growth if text wraps */
  background: #ffffff;
  border-top: 1px solid #ddd;
  flex-shrink: 0;   /* Keeps the footer from being squashed */
  padding: 0.25rem 1rem;
}

@media screen and (max-width: 768px) {
  /* On mobile, stack things slightly to avoid horizontal scroll */
  .app-footer .is-flex {
    justify-content: center !important;
    text-align: center;
  }
}

.header-side { width: 80px; } /* Fixed width to keep title centered */
.h-100 { height: 100%; }

</style>
