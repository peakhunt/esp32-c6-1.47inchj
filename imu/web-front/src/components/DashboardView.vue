<template>
  <div>
    <!-- Main Stats Container -->
    <div class="columns is-mobile is-multiline mb-5 has-background-white shadow-card p-4">
      <!-- ... (Keep all your STATUS, CPU, LOOP, I2C columns here) ... -->
      <!-- Use 'state' exactly like you did in App.vue -->
       <div class="column is-6-mobile has-text-centered">
          <p class="heading">STATUS</p>
          <div :class="['status-indicator', state.stats.connected ? 'is-live' : 'is-off']">
            <span class="tag is-rounded has-text-weight-bold">{{ state.stats.connected ? 'LIVE' : 'OFFLINE' }}</span>
          </div>
        </div>
        <!-- CPU: Half width on mobile -->
        <div class="column is-6-mobile has-text-centered">
          <p class="heading">CPU</p>
          <p class="title is-5 has-text-black">{{ state.cpuUsage }}%</p>
        </div>

        <!-- LOOP: Half width on mobile -->
        <div class="column is-6-mobile has-text-centered">
          <p class="heading">LOOP</p>
          <p class="title is-5 has-text-black">{{ state.samplingRate }}Hz</p>
        </div>

        <!-- I2C TX: Half width on mobile -->
        <div class="column is-6-mobile has-text-centered">
          <p class="heading">I2C TX</p>
          <p class="title is-5 has-text-black">{{ state.i2cTransactions.toLocaleString() }}</p>
        </div>

        <!-- I2C FAIL: Full width on mobile to emphasize errors -->
        <div class="column is-12-mobile has-text-centered">
          <p class="heading">I2C FAIL</p>
          <p class="title is-5" :class="state.i2cFailed > 0 ? 'has-text-danger' : 'has-text-grey-light'">
          {{ state.i2cFailed }}
          </p>
        </div>
    </div>

    <!-- GAUGES SECTION -->
    <div class="columns is-multiline is-variable is-2">
      <div class="column is-4-desktop is-12-mobile" v-for="type in ['ROLL', 'PITCH', 'YAW']" :key="type">
        <div class="card has-background-white has-text-centered shadow-card">
          <div class="card-content px-2 py-5">
            <p class="heading has-text-weight-bold has-text-black mb-4">{{ type }}</p>
            <component :is="getGauge(type)" :[type.toLowerCase()]="state[type.toLowerCase()]" />
            <p class="title is-2 has-text-black">{{ state[type.toLowerCase()].toFixed(type === 'YAW' ? 0 : 1) }}°</p>
          </div>
        </div>
      </div>
    </div>

    <!-- TELEMETRY CHART -->
    <div class="column is-12 mt-5 p-0">
      <div class="card has-background-white shadow-card">
        <div class="card-content">
          <p class="heading has-text-weight-bold has-text-black mb-4">REAL-TIME TELEMETRY</p>
          <div ref="chartRef" class="uplot-wrapper"></div>
        </div>
      </div>
    </div>

    <!-- 3D ATTITUDE VIEW -->
    <div class="column is-12 mt-5 p-0">
      <div class="card has-background-white shadow-card">
        <div class="card-content">
          <Attitude3D :roll="state.roll" :pitch="state.pitch" :yaw="state.yaw" />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { useIMUStore } from '../store/imuStore'
import uPlot from 'uplot'
import 'uplot/dist/uPlot.min.css'
import Attitude3D from './Attitude3D.vue'
import PitchGauge from './PitchGauge.vue'
import RollGauge from './RollGauge.vue'
import YawGauge from './YawGauge.vue'

const { state } = useIMUStore()
const chartRef = ref(null)
let uplotInstance = null

const chartData = [Array.from({length: 1000}, (_, i) => i), Array(1000).fill(0), Array(1000).fill(0), Array(1000).fill(0)]
const getGauge = (type) => ({ ROLL: RollGauge, PITCH: PitchGauge, YAW: YawGauge }[type])

// This is the bridge: App.vue calls this to update the chart
const updateChart = (r, p, y) => {
  chartData[0].push(chartData[0][chartData[0].length - 1] + 1); chartData[0].shift()
  chartData[1].push(r); chartData[1].shift()
  chartData[2].push(p); chartData[2].shift()
  chartData[3].push(y); chartData[3].shift()
  if (uplotInstance) uplotInstance.setData(chartData)
}

// Expose the function so the parent can see it
defineExpose({ updateChart })

onMounted(() => {
  uplotInstance = new uPlot({
    width: chartRef.value.offsetWidth, height: 250,
    series: [{}, { stroke: "#485fc7", label: "Roll" }, { stroke: "#ff3860", label: "Pitch" }, { stroke: "#ffdd57", label: "Yaw" }],
    axes: [{ grid: { stroke: "#f0f0f0" } }, { grid: { stroke: "#f0f0f0" }, values: (u, vals) => vals.map(v => v + "°") }],
    cursor: { show: false }
  }, chartData, chartRef.value)

  const resizeObserver = new ResizeObserver(entries => {
    for (let entry of entries) {
      if (uplotInstance && entry.contentRect.width > 0) {
        uplotInstance.setSize({ width: entry.contentRect.width, height: 250 })
      }
    }
  })
  if (chartRef.value) resizeObserver.observe(chartRef.value)
  uplotInstance._observer = resizeObserver
})

onUnmounted(() => {
  if (uplotInstance) {
    if (uplotInstance._observer) uplotInstance._observer.disconnect()
    uplotInstance.destroy()
  }
})
</script>

<style scoped>
/* uPlot Layout - Specific to DashboardView.vue */
.uplot-wrapper { 
  width: 100%; 
  display: flex; 
  justify-content: center; 
  background: #fff; 
}

:deep(.uplot) { 
  margin: 0 auto; 
}
</style>
