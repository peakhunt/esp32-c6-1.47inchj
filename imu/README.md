# ESP32-C6 IMU Demo with Web Interface & LVGL

An IMU (Inertial Measurement Unit) orientation tracking project built on the **ESP32-C6**, featuring sensor fusion, a local LCD interface (LVGL), and a web-based telemetry dashboard.

<img src="docs/board.jpg" width="100" alt="Hardware Overview">

## 🚀 Features

- **Orientation Tracking**: Uses Madgwick and Mahony filter algorithms for 9-axis (Accel/Gyro/Mag) orientation estimation.
- **Hardware Support**: 
  - **ESP32-C6** SoC (RISC-V).
  - **MPU9250** IMU (9-DOF).
  - **ST7789** SPI LCD (240x320).
  - **WS2812** Addressable RGB LED for status.
- **Web Dashboard**: Vue 3 / Vite frontend with Chart.js for telemetry and TWGL.js for 3D visualization.
- **On-Device UI**: LVGL-based interface showing stats, calibration status, and system info.
- **Connectivity**: Wi-Fi (SoftAP + Station) for remote monitoring.
- **Calibration**: Basic calibration for Accelerometer, Gyroscope, and Magnetometer with persistence in NVS.
- **System Monitoring**: CPU usage and heap memory tracking.

## 📸 Screen Shots

| Attitude Dashboard | Sensor Calibration | System Settings |
| :---: | :---: | :---: |
| ![Web Dashboard](docs/page1.png) | ![3D View](docs/page2.png) | ![LCD UI](docs/page3.png) |

## 🛠 Tech Stack

- **Firmware**: ESP-IDF v5.x (C), FreeRTOS.
- **Graphics**: LVGL v8.3+, ST7789 driver.
- **Web Frontend**: Vue 3, Vite, Bulma CSS, Chart.js, TWGL.js (WebGL).
- **Communication**: HTTP REST API, SPIFFS for static file serving.

## 🏁 Getting Started

### Prerequisites

- ESP-IDF v5.5.2 or later.
- Node.js & npm (for building the web frontend).

### Hardware Wiring

| Component | ESP32-C6 Pin |
| :--- | :--- |
| **I2C SDA (IMU)** | GPIO 19 |
| **I2C SCL (IMU)** | GPIO 18 |
| **IMU nCS** | GPIO 20 |
| **LCD MOSI** | GPIO 6 |
| **LCD SCLK** | GPIO 7 |
| **LCD CS** | GPIO 14 |
| **LCD DC** | GPIO 15 |
| **LCD RST** | GPIO 21 |
| **LCD BL** | GPIO 22 |
| **User Button** | GPIO 9 |
| **RGB LED** | GPIO 8 |

### Building and Flashing

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd imu_demo
   ```

2. **Build the Web Frontend**:
   The project is configured to build the web UI automatically during the firmware build process, but you can also build it manually:
   ```bash
   cd web-front
   npm install
   npm run build
   cd ..
   ```

3. **Configure and Build Firmware**:
   ```bash
   idf.py build
   ```

4. **Flash and Monitor**:
   ```bash
   idf.py flash monitor
   ```

## 🌐 Web Interface

By default, the device starts in **SoftAP mode**:
- **SSID**: `imu`
- **Password**: None
- **IP Address**: `192.168.4.1`

Once connected, open your browser and navigate to `http://192.168.4.1` to access the telemetry dashboard.

## 🧭 Calibration

Accurate orientation requires sensor calibration:
1. **Gyroscope**: Keep the device still during startup or trigger from the UI.
2. **Accelerometer**: Follow the 6-point tumble calibration (Z+, Z-, X+, X-, Y+, Y-).
3. **Magnetometer**: Rotate the device in a figure-eight pattern.

Calibration data is automatically saved to the ESP32's NVS (Non-Volatile Storage).

## 📂 Project Structure

- `main/`: Main application logic, Wi-Fi, Web Server, and UI tasks.
- `components/imu/`: Custom MPU9250 driver and sensor fusion algorithms.
- `components/nvs_helper/`: NVS storage utilities.
- `web-front/`: Vue 3 source code for the dashboard.
- `docs/`: Hardware and software documentation images.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
