<template>
  <div class="calibration-view px-2">
    
    <div v-for="sensor in [
      { id: 'accel', label: 'ACCELEROMETER', unit: 'm/s²', prec: 2 },
      { id: 'gyro', label: 'GYROSCOPE', unit: '°/s', prec: 2 },
      { id: 'mag', label: 'MAGNETOMETER', unit: 'µT', prec: 1 }
    ]" :key="sensor.id" class="card shadow-card mb-4 has-background-white">
      
      <div class="card-content p-3">
        <p class="heading has-text-weight-bold has-text-grey-light mb-3">{{ sensor.label }} {{ sensor.unit}}</p>

        <!-- THE 2-ROW MOBILE / 3-COL TABLET GRID -->
        <div class="columns is-mobile is-multiline is-vcentered is-variable is-1">
          
          <!-- ROW 1 / COL 1: DATA -->
          <div class="column is-3-tablet is-6-mobile border-right-tablet">
            <div v-for="axis in ['x','y','z']" :key="axis" class="is-flex is-justify-content-space-between mb-1 pr-2">
              <span class="is-size-7 has-text-grey-light">{{ axis.toUpperCase() }}:</span>
              <span class="is-family-monospace is-size-6 has-text-weight-bold">
                {{ imuStore.state[sensor.id][axis].toFixed(sensor.prec) }}
              </span>
            </div>
          </div>

          <!-- ROW 1 / COL 2: VISUAL (Next to data on mobile) -->
          <div class="column is-4-tablet is-6-mobile has-text-centered border-left-tablet">
             <div class="visual-container">
                <!-- We will put the specific sensor visual here later -->
                <div class="visual-placeholder-box">
                  <p class="is-size-7 has-text-grey-light mb-1">VISUAL</p>
                  <div class="mock-gizmo"></div>
                </div>
             </div>
          </div>

          <!-- ROW 2 / COL 3: THE CHART (Full width on mobile, middle on tablet) -->
          <div class="column is-5-tablet is-12-mobile mt-3-mobile">
             <div class="chart-wrapper">
                <div :ref="el => setChartRef(el, sensor.id)" class="mini-uplot"></div>
             </div>
          </div>

        </div>

        <!-- CONTROLS -->
        <div class="mt-4">
           <!-- (Keep your existing Execute / 6-Side buttons here) -->
           <button class="button is-primary is-small is-fullwidth has-text-weight-bold">
             {{ sensor.id === 'accel' ? 'CAPTURE STEP' : 'EXECUTE' }}
           </button>
        </div>

      </div>
    </div>
  </div>
</template>

<script setup>
import { reactive, onMounted, onUnmounted } from 'vue'
import { useIMUStore } from '../store/imuStore'
import uPlot from 'uplot'
import 'uplot/dist/uPlot.min.css'

const imuStore = useIMUStore()

// State for Accel buttons
const calState = reactive({
  accel: { '+X': false, '-X': false, '+Y': false, '-Y': false, '+Z': false, '-Z': false }
})

const max_data = 1000

// 1. Chart Management
const chartElements = {} // Stores the DOM elements
const chartInstances = {} // Stores uPlot instances
const chartData = {
  gyro: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  accel: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  mag: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)]
}

// 2. The function that was missing!
const setChartRef = (el, id) => {
  if (el) chartElements[id] = el
}

// 3. High-frequency update called by App.vue
const updateCalibrationCharts = () => {
  ['gyro', 'accel', 'mag'].forEach(id => {
    const data = chartData[id]
    const sensor = imuStore.state[id]

    // Shift time axis
    data[0].push(data[0][data[0].length - 1] + 1); data[0].shift()
    // Shift sensor axes
    data[1].push(sensor.x); data[1].shift()
    data[2].push(sensor.y); data[2].shift()
    data[3].push(sensor.z); data[3].shift()

    if (chartInstances[id]) {
      chartInstances[id].setData(data)
    }
  })
}

// Expose to parent (App.vue)
defineExpose({ updateCalibrationCharts })

// 4. Initialize uPlots
onMounted(() => {
  const commonOpts = (colorX, colorY, colorZ) => ({
    width: 0, // Will be set by resizeObserver
    height: 70,
    pxAlign: false,
    cursor: { show: false },
    select: { show: false },
    legend: { show: false },
    scales: { x: { time: false }, y: { range: (u, min, max) => [min - 1, max + 1] } },
    axes: [ { show: false }, { show: false } ],
    series: [
      {},
      { stroke: colorX, width: 2, points: { show: false } },
      { stroke: colorY, width: 2, points: { show: false } },
      { stroke: colorZ, width: 2, points: { show: false } },
    ],
  })

  // Create instances
  chartInstances.gyro = new uPlot(commonOpts("#485fc7", "#ff3860", "#ffdd57"), chartData.gyro, chartElements.gyro)
  chartInstances.accel = new uPlot(commonOpts("#485fc7", "#ff3860", "#ffdd57"), chartData.accel, chartElements.accel)
  chartInstances.mag = new uPlot(commonOpts("#485fc7", "#ff3860", "#ffdd57"), chartData.mag, chartElements.mag)

  // Resize handling for mobile/desktop toggle
  const resizeObserver = new ResizeObserver(entries => {
    for (let entry of entries) {
      const id = Object.keys(chartElements).find(key => chartElements[key] === entry.target)
      if (id && chartInstances[id]) {
        chartInstances[id].setSize({ width: entry.contentRect.width, height: 70 })
      }
    }
  })

  Object.values(chartElements).forEach(el => resizeObserver.observe(el))
  chartInstances._observer = resizeObserver
})

onUnmounted(() => {
  if (chartInstances._observer) chartInstances._observer.disconnect()
  Object.values(chartInstances).forEach(i => i?.destroy?.())
})

const runCal = async (type, axis = null) => {
  console.log(`Calibrating: ${type} ${axis || ''}`)
  if (type === 'accel' && axis) calState.accel[axis] = true
}
</script>

<style scoped>
.mini-uplot {
  width: 100%;
  height: 70px; /* Short but wide for mobile sweep */
  background: #fafafa;
  border-radius: 4px;
}

/* Tablet+ (Desktop) Layout: 3 Columns side-by-side */
@media screen and (min-width: 769px) {
  .border-right-tablet { border-right: 1px solid #eee; }
  .border-left-tablet { border-left: 1px solid #eee; }
  .mini-uplot { height: 90px; }
}

/* Mobile Layout: 2-Row Stacking logic */
@media screen and (max-width: 768px) {
  .mt-3-mobile { 
    margin-top: 0.75rem; 
    padding-top: 0.75rem;
    border-top: 1px solid #f5f5f5; /* Separates top row from chart */
  }
}

.is-family-monospace { font-family: 'Courier New', Courier, monospace; }
.mock-gizmo { width: 40px; height: 40px; border: 2px dashed #00d1b2; margin: 0 auto; border-radius: 6px; }
.shadow-card { border-radius: 12px; border: 1px solid #efefef; }
</style>
