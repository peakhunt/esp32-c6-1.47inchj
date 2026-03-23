<template>
  <div class="settings-view px-2 pb-6">
    
    <!-- 1. PERFORMANCE (STAYS LIVE) -->
    <div class="column is-12 p-0">
      <div class="card shadow-card has-background-white mb-5">
        <div class="card-content px-5 py-5">
          <p class="heading has-text-weight-bold has-text-black mb-5">PERFORMANCE & THROTTLING</p>
          <div class="columns is-variable is-8">
            <div class="column is-6">
              <div class="is-flex is-justify-content-between mb-2">
                <span class="is-size-7 has-text-grey-light uppercase tracking-wide">Packet Drop</span>
                <span class="is-family-monospace is-size-5 has-text-weight-bold has-text-link">{{ packetDrop }}ms</span>
              </div>
              <input class="slider is-fullwidth is-link m-0" type="range" min="0" max="500" step="10" v-model.number="packetDrop">
            </div>
            <div class="column is-6">
              <div class="is-flex is-justify-content-between mb-2">
                <span class="is-size-7 has-text-grey-light uppercase tracking-wide">Display Sync</span>
                <span class="is-family-monospace is-size-5 has-text-weight-bold has-text-link">{{ displaySync }}ms</span>
              </div>
              <input class="slider is-fullwidth is-link m-0" type="range" min="0" max="1000" step="50" v-model.number="displaySync">
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 2. IMU ENGINE (LOCAL DRAFT) -->
    <div class="column is-12 mt-5 p-0">
      <div class="card shadow-card has-background-white mb-5">
        <div class="card-content px-5 py-5">
          <p class="heading has-text-weight-bold has-text-black mb-5">IMU ENGINE CONFIG</p>
          <div class="columns is-multiline">
            <div class="column is-6 py-1">
              <label class="label is-size-7 has-text-grey-light uppercase">AHRS Algorithm</label>
              <div class="field has-addons">
                <p class="control is-expanded">
                  <button class="button is-small is-fullwidth is-family-monospace" 
                    :class="localData.imu.ahrs_mode === 'Mahony' ? 'is-link' : 'is-light'" 
                    @click="localData.imu.ahrs_mode = 'Mahony'">MAHONY</button>
                </p>
                <p class="control is-expanded">
                  <button class="button is-small is-fullwidth is-family-monospace" 
                    :class="localData.imu.ahrs_mode === 'Madgwick' ? 'is-link' : 'is-light'" 
                    @click="localData.imu.ahrs_mode = 'Madgwick'">MADGWICK</button>
                </p>
              </div>
            </div>
            <div class="column is-6 py-1">
              <label class="label is-size-7 has-text-grey-light uppercase">Declination (°)</label>
              <input class="input is-small is-family-monospace" type="number" step="0.001" v-model.number="localData.mag_declination">
            </div>

            <!-- DYNAMIC PARAMS -->
            <template v-if="localData.imu.ahrs_mode === 'Madgwick'">
              <div class="column is-12 py-1">
                <div class="is-flex is-justify-content-between mb-1">
                  <span class="is-size-7 has-text-grey-light uppercase">Madgwick Beta</span>
                  <span class="is-family-monospace is-size-6 has-text-weight-bold has-text-link">
                    {{ localData.imu.beta?.toFixed(3) }}
                  </span>
                </div>
                <input class="slider is-fullwidth is-link is-small m-0" type="range" min="0" max="1" step="0.001"
                       v-model.number="localData.imu.beta">
              </div>
            </template>

            <template v-if="localData.imu.ahrs_mode === 'Mahony'">
              <div class="column is-6 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">twoKp</label>
                <input class="input is-small is-family-monospace" type="number" step="0.001" v-model.number="localData.imu.twoKp">
              </div>
              <div class="column is-6 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">twoKi</label>
                <input class="input is-small is-family-monospace" type="number" step="0.001" v-model.number="localData.imu.twoKi">
              </div>
            </template>
          </div>
          <!-- ACTION BAR: LIVE PARAMETER PATCH -->
          <div class="mt-5 pt-4 border-top-dashed">
            <div class="is-flex is-align-items-center is-justify-content-space-between mb-3">
              <div class="is-flex is-align-items-center">
                <Icon :icon="tuneIcon" class="has-text-info mr-2" style="font-size: 1rem;" />
                <span class="is-size-7 has-text-grey-dark has-text-weight-bold uppercase tracking-wide">
                  Live-patch AHRS fusion logic
                </span>
              </div>
            </div>

            <button class="button is-fullwidth btn-engineering-save"
                    @click="updateEngineOnly" >
              <span class="is-flex is-align-items-center">
                <Icon :icon="autoFixIcon" class="mr-2" style="font-size: 1.1rem;" />
                APPLY 
              </span>
            </button>
          </div>

        </div>
      </div>
    </div>

    <!-- 3. WIFI (LOCAL DRAFT) -->
    <div class="column is-12 mt-5 p-0">
      <div class="card shadow-card has-background-white">
        <div class="card-content px-5 py-5">
          <p class="heading has-text-weight-bold has-text-black mb-5">WIFI INTERFACES</p>
          <div class="is-flex is-align-items-center is-justify-content-space-between mb-5 pb-4 border-bottom">
            <span class="is-size-7 has-text-grey-light uppercase tracking-wide">Station Mode Uplink</span>
            <label class="checkbox">
              <input type="checkbox" v-model="localData.wifi.sta_enabled">
              <span class="ml-2 is-size-7 has-text-weight-bold uppercase">Enable</span>
            </label>
          </div>

          <div v-if="localData.wifi.sta_enabled" class="mb-5 pb-5 border-bottom">
            <div class="columns is-multiline">
              <div class="column is-6 py-1"><label class="label is-size-7 has-text-grey-light uppercase">SSID</label>
                <input class="input is-small is-family-monospace" v-model="localData.wifi.sta_ssid">
              </div>
              <div class="column is-6 py-1"><label class="label is-size-7 has-text-grey-light uppercase">Password</label>
                <input class="input is-small is-family-monospace" type="password" v-model="localData.wifi.sta_password">
              </div>
            </div>
          </div>
          <!-- ACCESS POINT: THE LOCAL DOOR -->
          <div>
            <p class="is-size-7 has-text-grey-darker has-text-weight-bold mb-3 uppercase tracking-wide">Access Point (Local)</p>
            <div class="columns is-multiline">
              <div class="column is-4 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">AP SSID</label>
                <input class="input is-small is-family-monospace" v-model="localData.wifi.ap_ssid">
              </div>
              <div class="column is-4 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">AP Password</label>
                <input class="input is-small is-family-monospace" type="password" v-model="localData.wifi.ap_password">
              </div>
              <div class="column is-4 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">Channel</label>
                <div class="select is-small is-fullwidth">
                  <select v-model.number="localData.wifi.channel" class="is-family-monospace">
                    <option v-for="n in 13" :key="n" :value="n">CH {{ n }}</option>
                  </select>
                </div>
              </div>

              <div class="column is-6 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">AP IP Address</label>
                <input class="input is-small is-family-monospace" v-model="localData.wifi.ap_ip" placeholder="192.168.4.1">
              </div>
              <div class="column is-6 py-1">
                <label class="label is-size-7 has-text-grey-light uppercase">Subnet Mask</label>
                <input class="input is-small is-family-monospace" v-model="localData.wifi.ap_mask" placeholder="255.255.255.0">
              </div>
            </div>
          </div>
          <div class="mt-6 pt-5 border-top-dashed">
            <div class="is-flex is-align-items-center is-justify-content-space-between mb-4">
              <div class="is-flex is-align-items-center">
                <Icon :icon="alertCircleOutline" class="has-text-warning mr-2" style="font-size: 1.2rem;" />
                <span class="is-size-7 has-text-grey-dark has-text-weight-semibold uppercase tracking-wide">
                  Hardware restart required to apply network changes
                </span>
              </div>
            </div>
            
            <button 
              class="button is-fullwidth btn-engineering-save"
              @click="saveAll"
            >
              <span class="is-flex is-align-items-center">
                <Icon :icon="contentSaveOutline" class="mr-2" style="font-size: 1.1rem;" />
                Apply & REBOOT
              </span>
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import { useIMUStore } from '../store/imuStore'
import { Icon } from '@iconify/vue'
import alertCircleOutline from '@iconify-icons/mdi/alert-circle-outline'
import contentSaveOutline from '@iconify-icons/mdi/content-save-outline'
import tuneIcon from '@iconify-icons/mdi/tune-vertical'
import autoFixIcon from '@iconify-icons/mdi/auto-fix'

const imuStore = useIMUStore()
const { state } = imuStore

// --- 1. PERFORMANCE (LIVE / NO SANDBOX) ---
// These update the store and LocalStorage IMMEDIATELY
const packetDrop = computed({
  get: () => state.packetDropMs,
  set: (val) => imuStore.setPacketDrop(val)
})

const displaySync = computed({
  get: () => state.displayUpdateMs,
  set: (val) => imuStore.setDisplayRate(val)
})

// --- 2. HARDWARE (SANDBOXED / DRAFT) ---
const localData = ref({
  wifi: {},
  imu: {},
  mag_declination: 0
})

const syncFromStore = () => {
  localData.value = JSON.parse(JSON.stringify({
    wifi: state.settings.wifi,
    imu: state.settings.imu,
    mag_declination: state.settings.calibration.mag_declination
  }))
}

// Reset hardware draft on entry, but LEAVE performance settings alone
watch(() => state.currentView, (newView) => {
  if (newView === 'settings') syncFromStore()
})

onMounted(syncFromStore)
</script>

<style scoped>
.shadow-card {
  box-shadow: 0 2px 15px rgba(0,0,0,0.05);
  border-radius: 8px;
}

.is-family-monospace {
  font-family: 'Courier New', Courier, monospace;
}

.uppercase {
  text-transform: uppercase; letter-spacing: 0.1em;
}

.slider {
  cursor: pointer;
  height: 8px;
}

.border-bottom {
  border-bottom: 1px solid #f0f0f0;
}

.input.is-small, .select.is-small select {
  background-color: #f7f9fa;
  border: 1px solid #eee;
  transition: all 0.2s ease;
  color: #2c3e50;
}

.input.is-small:focus, .select.is-small select:focus {
  background-color: #fff;
  border-color: #485fc7;
  box-shadow: none;
}

.label.is-size-7 {
  margin-bottom: 0.25rem !important;
  color: #7a7a7a !important;
  letter-spacing: 0.05em;
}

/* Dashed separator for engineering feel */
.border-top-dashed {
  border-top: 1px dashed #e0e0e0;
}

/* The Professional 'No-Slop' Button */
.btn-engineering-save {
  background-color: #2c3e50; /* Deep Slate */
  color: #ffffff;
  border: none;
  border-radius: 6px;
  height: 48px;
  text-transform: uppercase;
  letter-spacing: 0.15em;
  font-weight: 700;
  font-size: 0.75rem;
  box-shadow: 0 4px 0 #1a252f; /* Fixed bottom shadow for depth */
  transition: all 0.1s ease;
}

.btn-engineering-save:hover {
  background-color: #34495e;
  color: white;
  transform: translateY(1px);
  box-shadow: 0 3px 0 #1a252f;
}

.btn-engineering-save:active {
  transform: translateY(4px);
  box-shadow: none;
}

/* Warning text styling */
.uppercase { text-transform: uppercase; }
.tracking-wide { letter-spacing: 0.08em; }
</style>
