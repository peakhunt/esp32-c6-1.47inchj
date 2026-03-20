import { reactive, readonly } from 'vue'

const state = reactive({
  roll: 0.0,
  pitch: 0.0,
  yaw: 0.0,
  
  // SENSOR VECTORS (Engineered Units)
  gyro: { x: 0, y: 0, z: 0 },
  accel: { x: 0, y: 0, z: 0 },
  mag: { x: 0, y: 0, z: 0 },

  // SYSTEM METRICS
  cpuUsage: 0,
  samplingRate: 0,
  i2cTransactions: 0,
  i2cFailed: 0,
  
  stats: {
    packetsReceived: 0,
    errors: 0,
    connected: false,
    startTime: Date.now()
  }
})

// HIGH-FREQUENCY UPDATER
const updateIMU = (r, p, y, g, a, m) => {
  state.roll = r
  state.pitch = p
  state.yaw = y
  
  // Direct assignment is faster for 50Hz updates
  if (g) state.gyro = g
  if (a) state.accel = a
  if (m) state.mag = m
  
  state.stats.packetsReceived++
}

const updateSystemStats = (cpu, rate, tx, fail) => {
  state.cpuUsage = cpu
  state.samplingRate = rate
  state.i2cTransactions = Number(tx)
  state.i2cFailed = Number(fail)
}

const setConnected = (status) => {
  state.stats.connected = status
}

export const useIMUStore = () => {
  return {
    state: readonly(state),
    updateIMU,
    updateSystemStats,
    setConnected
  }
}
