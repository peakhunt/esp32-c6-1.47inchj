<template>
  <div class="calibration-view px-2">
    
    <!-- ACCELEROMETER SECTION -->
    <div class="card shadow-card mb-4 has-background-white">
      <div class="card-content p-3">
        <p class="heading has-text-weight-bold has-text-grey-light mb-3">ACCELEROMETER (g)</p>
        <div class="columns is-mobile is-multiline is-vcentered is-variable is-1">
          <div class="column is-2-tablet is-6-mobile border-right-tablet">
            <div v-for="axis in ['x','y','z']" :key="axis" class="is-flex mb-1 pr-2">
              <span class="is-size-6" :style="{ width: '30px', color: getAxisColor(axis) }">{{ axis.toUpperCase() }}:</span>
              <span class="is-family-monospace is-size-6 has-text-weight-bold">{{ imuStore.state.accel[axis].toFixed(2) }}</span>
            </div>
          </div>
          <div class="column is-4-tablet is-6-mobile has-text-centered border-left-tablet">
            <div class="mag-visual-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
              <AccelCubeView :target="currentTarget" :done-sides="completedSides" />
            </div>
          </div>
          <div class="column is-6-tablet is-12-mobile mt-3-mobile">
             <div class="chart-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
                <div ref="accelChartEl" class="mini-uplot"></div>
             </div>
          </div>
        </div>
        <!-- ACCEL BUTTON SECTION -->
        <div class="mt-4">
          <!-- Removed the 'columns' and 'column' wrappers that were squashing it -->
          <button 
             class="button is-primary is-small is-fullwidth has-text-weight-bold"
             :disabled="!detectedSide || imuStore.state.isCalibrating"
             @click="apiCalibrate('accel', detectedSide)"
             >
             <template v-if="imuStore.state.isCalibrating && currentTarget === detectedSide">
               RECORDING {{ detectedSide }}...
             </template>
            <template v-else-if="detectedSide">
              {{ calState.accel[detectedSide] ? 'RE-CAPTURE ' + detectedSide : 'CAPTURE ' + detectedSide }}
            </template>
            <template v-else>
              LEVEL SENSOR TO START
            </template>
          </button>

          <!-- Mini Progress Indicators -->
          <div class="is-flex is-justify-content-center mt-2" style="gap: 5px;">
            <div v-for="side in ['+X','-X','+Y','-Y','+Z','-Z']" :key="side"
                 :class="['tag', calState.accel[side] ? 'is-success' : 'is-light']"
                 style="width: 35px; height: 8px; padding: 0; border-radius: 2px;">
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- GYROSCOPE SECTION -->
    <div class="card shadow-card mb-4 has-background-white">
      <div class="card-content p-3">
        <p class="heading has-text-weight-bold has-text-grey-light mb-3">GYROSCOPE (°/s)</p>
        <div class="columns is-mobile is-multiline is-vcentered is-variable is-1">
          <div class="column is-2-tablet is-6-mobile border-right-tablet">
            <div v-for="axis in ['x','y','z']" :key="axis" class="is-flex mb-1 pr-2">
              <span class="is-size-6" :style="{ width: '30px', color: getAxisColor(axis) }">{{ axis.toUpperCase() }}:</span>
              <span class="is-family-monospace is-size-6 has-text-weight-bold">{{ imuStore.state.gyro[axis].toFixed(2) }}</span>
            </div>
          </div>
          <div class="column is-4-tablet is-6-mobile has-text-centered border-left-tablet">
            <div class="mag-visual-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
              <GyroStillnessView :progress="calProgress" :calibrating="isCalibrating" :noise-level="gyroMagnitude" :threshold="0.5"/>
            </div>
          </div>
          <div class="column is-6-tablet is-12-mobile mt-3-mobile">
             <div class="chart-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
                <div ref="gyroChartEl" class="mini-uplot"></div>
             </div>
          </div>
        </div>
        <button class="button is-primary is-small is-fullwidth has-text-weight-bold mt-4"
                :disabled="imuStore.state.isCalibrating"
                @click="apiCalibrate('gyro')">
          {{ imuStore.state.isCalibrating ? 'CALIBRATING BIAS...' : 'EXECUTE BIAS' }}
        </button>
      </div>
    </div>

    <!-- MAGNETOMETER SECTION -->
    <div class="card shadow-card mb-4 has-background-white">
      <div class="card-content p-3">
        <p class="heading has-text-weight-bold has-text-grey-light mb-3">MAGNETOMETER (µT)</p>
        <div class="columns is-mobile is-multiline is-vcentered is-variable is-1">
          <div class="column is-2-tablet is-6-mobile border-right-tablet">
            <div v-for="axis in ['x','y','z']" :key="axis" class="is-flex mb-1 pr-2">
              <span class="is-size-6" :style="{ width: '30px', color: getAxisColor(axis) }">{{ axis.toUpperCase() }}:</span>
              <span class="is-family-monospace is-size-6 has-text-weight-bold">{{ imuStore.state.mag[axis].toFixed(1) }}</span>
            </div>
          </div>
          <div class="column is-4-tablet is-6-mobile has-text-centered border-left-tablet">
            <div class="mag-visual-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
              <MagCloudView ref="magCloudRef" />
            </div>
          </div>
          <div class="column is-6-tablet is-12-mobile mt-3-mobile">
             <div class="chart-wrapper" :style="{ '--wrapper-height': chart_height + 'px' }">
                <div ref="magChartEl" class="mini-uplot"></div>
             </div>
          </div>
        </div>
        <button class="button is-primary is-small is-fullwidth has-text-weight-bold mt-4" 
                :disabled="imuStore.state.isCalibrating" 
                @click="apiCalibrate('mag')">
          {{ imuStore.state.isCalibrating ? 'HARDWARE BUSY' : 'RESET CLOUD' }}
        </button>

      </div>
    </div>

  </div>
</template>

<script setup>
import { reactive, onMounted, onUnmounted, ref, computed, watch } from 'vue'
import { useIMUStore } from '../store/imuStore'
import MagCloudView from './MagCloudView.vue'
import GyroStillnessView from './GyroStillnessView.vue'
import AccelCubeView from './AccelCubeView.vue'
import uPlot from 'uplot'
import 'uplot/dist/uPlot.min.css'

const imuStore = useIMUStore()
const magCloudRef = ref(null)

// Direct Chart Element Refs
const accelChartEl = ref(null)
const gyroChartEl = ref(null)
const magChartEl = ref(null)

const calState = reactive({ accel: { '+X': false, '-X': false, '+Y': false, '-Y': false, '+Z': false, '-Z': false } })
const max_data = 500

const windowWidth = ref(window.innerWidth)
const updateWidth = () => { windowWidth.value = window.innerWidth }

const chart_height = computed(() => windowWidth.value < 769 ? 120 : 250)
const getAxisColor = (axis) => ({ x: '#3246a3', y: '#cc002e', z: '#1ca318' }[axis])

const currentTarget = ref('+Z')
const completedSides = ref([])
const chartInstances = {}

const chartData = {
  gyro: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  accel: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)],
  mag: [Array.from({length: max_data}, (_, i) => i), Array(max_data).fill(0), Array(max_data).fill(0), Array(max_data).fill(0)]
}

const updateCalibrationCharts = () => {
  ['gyro', 'accel', 'mag'].forEach(id => {
    const data = chartData[id]; const sensor = imuStore.state[id]
    data[0].push(data[0][data[0].length - 1] + 1); data[0].shift()
    data[1].push(sensor.x); data[1].shift()
    data[2].push(sensor.y); data[2].shift()
    data[3].push(sensor.z); data[3].shift()
    if (chartInstances[id]) chartInstances[id].setData(data)
  })

  if (magCloudRef.value?.addPoint) {
    magCloudRef.value.addPoint(imuStore.state.mag.x, imuStore.state.mag.y, imuStore.state.mag.z)
  }
}

defineExpose({ updateCalibrationCharts })

onMounted(() => {
  window.addEventListener('resize', updateWidth)

  const opts = (cx, cy, cz, unit) => ({
    width: 0, height: chart_height.value, cursor: { show: false }, select: { show: false }, legend: { show: false },
    scales: { x: { time: false }, y: { range: (u, min, max) => [min - 0.5, max + 0.5] } },
    axes: [{ grid: { stroke: "#f0f0f0" } }, { grid: { stroke: "#f0f0f0" }, values: (u, vals) => vals.map(v => v + unit) }],
    series: [{}, { stroke: cx, width: 2 }, { stroke: cy, width: 2 }, { stroke: cz, width: 2 }],
  })

  chartInstances.gyro = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "°/s"), chartData.gyro, gyroChartEl.value)
  chartInstances.accel = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "g"), chartData.accel, accelChartEl.value)
  chartInstances.mag = new uPlot(opts("#ff0000", "#0000ff", "#00ff00", "µT"), chartData.mag, magChartEl.value)

  const ro = new ResizeObserver(() => {
    chartInstances.accel?.setSize({ width: accelChartEl.value.offsetWidth, height: chart_height.value })
    chartInstances.gyro?.setSize({ width: gyroChartEl.value.offsetWidth, height: chart_height.value })
    chartInstances.mag?.setSize({ width: magChartEl.value.offsetWidth, height: chart_height.value })
  })
  
  ro.observe(accelChartEl.value); ro.observe(gyroChartEl.value); ro.observe(magChartEl.value)
  chartInstances._observer = ro
})

// Fix for chart height change when viewport crosses breakpoint
watch(chart_height, (newH) => {
  chartInstances.accel?.setSize({ width: accelChartEl.value.offsetWidth, height: newH })
  chartInstances.gyro?.setSize({ width: gyroChartEl.value.offsetWidth, height: newH })
  chartInstances.mag?.setSize({ width: magChartEl.value.offsetWidth, height: newH })
})

onUnmounted(() => {
  window.removeEventListener('resize', updateWidth)
  chartInstances._observer?.disconnect()
  Object.values(chartInstances).forEach(i => i?.destroy?.())
})

const runCal = (type, axis = null) => {
  if (type === 'mag' && magCloudRef.value) magCloudRef.value.reset()
  if (type === 'accel' && axis) {
    currentTarget.value = axis
    calState.accel[axis] = true
    if (!completedSides.value.includes(axis)) completedSides.value.push(axis)
  }
}

const calProgress = ref(0); const isCalibrating = ref(false)
const gyroMagnitude = computed(() => {
  const g = imuStore.state.gyro
  return Math.sqrt(g.x**2 + g.y**2 + g.z**2)
})

const runGyroCal = () => {
  /*
  isCalibrating.value = true; calProgress.value = 0
  const timer = setInterval(() => {
    if (gyroMagnitude.value > 0.5) calProgress.value = 0
    else calProgress.value += 1
    if (calProgress.value >= 100) { clearInterval(timer); isCalibrating.value = false }
  }, 50)
  */
  // 1. Lock the whole app
  imuStore.setCalibrating(true);
  console.log("Hardware Lock: ON (5s)");

  // 2. Unlock after 5 seconds
  setTimeout(() => {
    imuStore.setCalibrating(false);
    console.log("Hardware Lock: OFF");
  }, 5000);
}

const detectedSide = computed(() => {
  const a = imuStore.state.accel;
  const threshold = 0.2; // 0.8g to 1.2g range
  
  if (a.z > (1.0 - threshold)) return '+Z';
  if (a.z < (-1.0 + threshold)) return '-Z';
  if (a.x > (1.0 - threshold)) return '+X';
  if (a.x < (-1.0 + threshold)) return '-X';
  if (a.y > (1.0 - threshold)) return '+Y';
  if (a.y < (-1.0 + threshold)) return '-Y';
  
  return null; // Not leveled or moving
});

watch(detectedSide, (newSide) => {
  // If we detect a valid side (+X, -Z, etc.), update the cube's target face
  if (newSide) {
    currentTarget.value = newSide
  }
})

const apiCalibrate = async (sensorType, axis = null) => {
  imuStore.setCalibrating(true);
  
  let url = `/api/calibrate/${sensorType}`;
  
  // 1. Build the query string
  if (axis) {
    url += `?side=${encodeURIComponent(axis)}`;
    
    // If this is the first side, tell the ESP32 to wipe its buffers
    if (sensorType === 'accel' && completedSides.value.length === 0) {
      url += `&reset=1`;
    }
  }

  try {
    const controller = new AbortController();
    const timeoutMs = (sensorType === 'accel') ? 30000 : 90000;
    const timeoutId = setTimeout(() => controller.abort(), timeoutMs);

    const response = await fetch(url, { 
      method: 'POST',
      signal: controller.signal 
    });
    
    clearTimeout(timeoutId);
    if (!response.ok) throw new Error(`Hardware returned ${response.status}`);

    // 2. Local State Updates
    if (sensorType === 'accel' && axis) {
      if (!completedSides.value.includes(axis)) completedSides.value.push(axis);
      calState.accel[axis] = true;
    }

    // 3. Final Solve Trigger
    if (sensorType === 'accel' && completedSides.value.length === 6) {
      const finishResponse = await fetch('/api/calibrate/accel/finish', { method: 'POST' });
      const finalResult = await finishResponse.json();
      console.log("Success:", finalResult);
      alert("Calibration Saved!");
    }

  } catch (err) {
    alert(`Error: ${err.message}`);
  } finally {
    imuStore.setCalibrating(false);
  }
};

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
  .border-right-tablet { border-right: 1px solid #eee; }
  .border-left-tablet { border-left: 1px solid #eee; }
}
@media screen and (max-width: 768px) {
  .mt-3-mobile { margin-top: 0.75rem; padding-top: 0.75rem; border-top: 1px solid #f5f5f5; }
}
.is-family-monospace { font-family: 'Courier New', Courier, monospace; }
.shadow-card { border-radius: 12px; border: 1px solid #efefef; }
.button.is-extra-small { font-size: 0.6rem; height: 1.8rem; padding: 0; }
.mag-visual-wrapper {
  position: relative;
  width: var(--wrapper-height, 250px); 
  height: var(--wrapper-height, 250px);
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}
.mag-visual-wrapper > * { width: 100%; height: 100%; }
</style>
