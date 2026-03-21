<template>
  <div class="calibration-view px-2">
    <div v-for="sensor in sensors" :key="sensor.id" class="card shadow-card mb-4 has-background-white">
      <div class="card-content p-3">
        <p class="heading has-text-weight-bold has-text-grey-light mb-3">{{ sensor.label }}({{ sensor.unit}})</p>

        <div class="columns is-mobile is-multiline is-vcentered is-variable is-1">
          <!-- DATA -->
          <div class="column is-2-tablet is-6-mobile border-right-tablet">
            <div v-for="axis in ['x','y','z']" :key="axis" class="is-flex mb-1 pr-2">
              <span class="is-size-6"
                    :style="{ 
                      width: '30px', 
                      color: { x: '#3246a3', y: '#cc002e', z: '#1ca318' }[axis] }">
                {{ axis.toUpperCase() }}:
              </span>
              <span class="is-family-monospace is-size-6 has-text-weight-bold">
                {{ imuStore.state[sensor.id][axis].toFixed(sensor.prec) }}
              </span>
            </div>
          </div>

          <!-- VISUAL COLUMN -->
          <!-- In CalibrationView.vue -->
          <div class="column is-4-tablet is-6-mobile has-text-centered border-left-tablet">
            <div v-if="sensor.id === 'mag'" class="mag-visual-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
              <MagCloudView ref="magCloudRef" />
            </div>
            <div v-if="sensor.id === 'gyro'" class="mag-visual-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
              <GyroStillnessView ref="gyroStillnessRef"
                                 :progress="calProgress"
                                 :calibrating="isCalibrating"
                                 :noise-level="gyroMagnitude"
                                 :threshold="0.5"/>
            </div>
            <div v-else class="visual-container">
              <!-- Gyro/Accel placeholders -->
            </div>
          </div>


          <!-- CHART -->
          <div class="column is-6-tablet is-12-mobile mt-3-mobile">
             <div class="chart-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
                <div :ref="el => setChartRef(el, sensor.id)" class="mini-uplot"></div>
             </div>
          </div>


        </div>

        <div class="mt-4">
           <div v-if="sensor.id === 'accel'" class="columns is-mobile is-variable is-1 mb-0">
              <div v-for="side in ['+X','-X','+Y','-Y','+Z','-Z']" :key="side" class="column">
                <button class="button is-extra-small is-fullwidth has-text-weight-bold" 
                        :class="calState.accel[side] ? 'is-success' : 'is-light'" 
                        @click="runCal('accel', side)">{{ side }}</button>
              </div>
           </div>
           <button v-else class="button is-primary is-small is-fullwidth has-text-weight-bold" @click="runCal(sensor.id)">
             {{ sensor.id === 'gyro' ? 'EXECUTE BIAS' : 'RESET CLOUD' }}
           </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { reactive, onMounted, onUnmounted, ref, computed } from 'vue'
import { useIMUStore } from '../store/imuStore'
import MagCloudView from './MagCloudView.vue'
import GyroStillnessView from './GyroStillnessView.vue'
import uPlot from 'uplot'
import 'uplot/dist/uPlot.min.css'

const imuStore = useIMUStore()
const magCloudRef = ref(null)
const sensors = [
  { id: 'accel', label: 'ACCELEROMETER', unit: 'g', prec: 2 },
  { id: 'gyro', label: 'GYROSCOPE', unit: '°/s', prec: 2 },
  { id: 'mag', label: 'MAGNETOMETER', unit: 'µT', prec: 1 }
]

const calState = reactive({ accel: { '+X': false, '-X': false, '+Y': false, '-Y': false, '+Z': false, '-Z': false } })
const max_data = 500

const windowWidth = ref(window.innerWidth);
const updateWidth = () => {
  windowWidth.value = window.innerWidth;
};

const chart_height = computed(() => {
  return windowWidth.value < 769 ? 120 : 250; 
});

const chartElements = {}
const chartInstances = {}
const chartData = {
  gyro: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  accel: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  mag: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)]
}

const setChartRef = (el, id) => { if (el) chartElements[id] = el }

const updateCalibrationCharts = () => {
  ['gyro', 'accel', 'mag'].forEach(id => {
    const data = chartData[id]; const sensor = imuStore.state[id]
    data[0].push(data[0][data[0].length - 1] + 1); data[0].shift()
    data[1].push(sensor.x); data[1].shift()
    data[2].push(sensor.y); data[2].shift()
    data[3].push(sensor.z); data[3].shift()
    if (chartInstances[id]) chartInstances[id].setData(data)
  })

  // Update the 3D Point Cloud for Magnetometer
  const magVisual = Array.isArray(magCloudRef.value) ? magCloudRef.value[0] : magCloudRef.value;
  if (magVisual && magVisual.addPoint) {
    magVisual.addPoint(imuStore.state.mag.x, imuStore.state.mag.y, imuStore.state.mag.z)
  }
}

defineExpose({ updateCalibrationCharts })

onMounted(() => {
  window.addEventListener('resize', updateWidth);

  const opts = (cx, cy, cz, unit) => ({
    width: 0, height: chart_height.value, cursor: { show: false }, select: { show: false }, legend: { show: false },
    scales: { x: { time: false }, y: { range: (u, min, max) => [min - 0.5, max + 0.5] } },
    //axes: [ { show: false }, { show: false } ],
    axes: [{ grid: { stroke: "#f0f0f0" } }, { grid: { stroke: "#f0f0f0" }, values: (u, vals) => vals.map(v => v + unit) }],
    series: [{}, { stroke: cx, width: 2 }, { stroke: cy, width: 2 }, { stroke: cz, width: 2 }],
  })

  chartInstances.gyro = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "°/s"), chartData.gyro, chartElements.gyro)
  chartInstances.accel = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "g"), chartData.accel, chartElements.accel)
  chartInstances.mag = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "µT"), chartData.mag, chartElements.mag)

  const ro = new ResizeObserver(entries => {
    for (let e of entries) {
      const id = Object.keys(chartElements).find(k => chartElements[k] === e.target)
      if (id && chartInstances[id]) chartInstances[id].setSize({ width: e.contentRect.width, height: chart_height.value })
    }
  })
  Object.values(chartElements).forEach(el => ro.observe(el))
  chartInstances._observer = ro
})

onUnmounted(() => {
  window.removeEventListener('resize', updateWidth);
  chartInstances._observer?.disconnect()
  Object.values(chartInstances).forEach(i => i?.destroy?.())
})

const runCal = (type, axis = null) => {
  if (type === 'mag' && magCloudRef.value) magCloudRef.value.reset()
  if (type === 'accel' && axis) calState.accel[axis] = true
}

// State for the calibration process
const calProgress = ref(0)
const isCalibrating = ref(false)

// Calculate the live "shakiness" of the gyro
const gyroMagnitude = computed(() => {
  const g = imuStore.state.gyro
  // Magnitude formula: sqrt(x² + y² + z²)
  return Math.sqrt(g.x**2 + g.y**2 + g.z**2)
})

// Triggered by your "EXECUTE BIAS" button
const runGyroCal = () => {
  isCalibrating.value = true
  calProgress.value = 0
  
  const timer = setInterval(() => {
    // If the sensor is too shaky (> 0.5 deg/s), reset progress
    if (gyroMagnitude.value > 0.5) {
      calProgress.value = 0
    } else {
      calProgress.value += 1 // Progresses to 100 over ~5 seconds
    }

    if (calProgress.value >= 100) {
      clearInterval(timer)
      isCalibrating.value = false
      // Send the final command to the hardware here
    }
  }, 50) // Runs at 20Hz
}


</script>

<style scoped>

.chart-wrapper {
  display: block;
  width: 100%;
  height: var(--wrapper-height, 250px);
  position: relative;
}

.mini-uplot {
  width: 100% !important;
  height: 100% !important;
  background: #fafafa;
  border-radius: 4px;
}

@media screen and (min-width: 769px) {
  .border-right-tablet {
    border-right: 1px solid #eee;
  }
  .border-left-tablet {
    border-left: 1px solid #eee;
  }
}

@media screen and (max-width: 768px) {
  .mt-3-mobile {
    margin-top: 0.75rem;
    padding-top: 0.75rem;
    border-top: 1px solid #f5f5f5;
  }
}

.is-family-monospace {
  font-family: 'Courier New', Courier, monospace;
}

.mock-gizmo {
  width: 30px;
  height: 30px;
  border: 2px dashed #00d1b2;
  margin: 0 auto;
  border-radius: 4px;
}

.shadow-card {
  border-radius: 12px;
  border: 1px solid #efefef;
}

.button.is-extra-small {
  font-size: 0.6rem;
  height: 1.8rem;
  padding: 0;
}

.mag-visual-wrapper {
  position: relative;
  /* Use the variable for BOTH to keep it a perfect square */
  width: var(--wrapper-height, 250px); 
  height: var(--wrapper-height, 250px);
  margin: 0 auto; /* Center it horizontally in its column */
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

/* Ensure the visualizer fills the wrapper */
.mag-visual-wrapper > * {
  width: 100%;
  height: 100%;
}
</style>
