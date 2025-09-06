# 🚗 Automotive CAN Bus Diagnostic Tool  

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)  
![Stars](https://img.shields.io/github/stars/simantkhadka/Automotive-CAN-Bus-Diagnostic-Tool?style=social)  
![Last Commit](https://img.shields.io/github/last-commit/simantkhadka/Automotive-CAN-Bus-Diagnostic-Tool)  

---

## 📖 Overview  

The **Automotive CAN Bus Diagnostic Tool** is an **ESP32-based open-source platform** for analyzing and interacting with vehicle networks through the OBD-II port.  

It enables:  
- 📊 Real-time vehicle data monitoring  
- 🔍 CAN traffic sniffing and reverse engineering  
- 🛠️ Diagnostic Trouble Code (DTC) reading/clearing  
- 🌐 Connectivity via USB, Bluetooth, and Wi-Fi  

This project bridges **embedded firmware, automotive diagnostics, and cybersecurity research** — providing a **professional-grade tool** for enthusiasts, engineers, and researchers.  

---

## ✨ Features  

| Feature | Description |
|---------|-------------|
| 🛠️ **OBD-II Modes 01–06** | Live data, freeze frames, DTC retrieval/clearing, O₂ sensor monitoring, and on-board test results — a complete diagnostic workflow. |
| 📡 **Real-Time CAN Analysis** | Sniff, log, and replay raw CAN frames for reverse engineering, anomaly detection, and hidden signal discovery. |
| 🔗 **Flexible Connectivity** | USB Serial, Bluetooth ELM327 emulation (works with Torque, Car Scanner), or Wi-Fi AP with dashboard/API. |
| ⚡ **Portable ESP32 Design** | Compact hardware with LED indicators for CAN init and RX/TX activity. Plug-and-play across vehicles. |
| 🔒 **Security & Research Ready** | Inject custom CAN frames and perform MITM experiments for cybersecurity research. |
| 🌍 **Cross-Platform & Extensible** | Works on Windows, Linux, macOS, Android. Modular firmware lets you add new PIDs and extend features. |

---
## DEMOS
[![Watch on YouTube](https://img.youtube.com/vi/T9zk9gBA490/maxresdefault.jpg)](https://youtu.be/T9zk9gBA490)




## 🔌 Hardware Setup  

### Required Components  
- ESP32 Development Board  
- MCP2515 CAN Controller / Shield  
- OBD-II Adapter Cable  
- Power source (vehicle battery or bench supply)  

### Shield Schematic
<img src="schematic-esp32-shield-can.jpg" width="600">

### Wiring
<img src="obd2-connector-pinout-socket.jpg" width="600">


### Example Hardware Setup  
<img src="docs/images/device_setup.jpg" width="500">  

---

## 🛠️ Getting Started  

### Requirements  
- **Hardware**: ESP32 + MCP2515 + OBD-II cable  
- **Software**: PlatformIO (recommended) or Arduino IDE  

### Installation  

```bash
# Clone the repository
git clone https://github.com/simantkhadka/Automotive-CAN-Bus-Diagnostic-Tool.git
cd Automotive-CAN-Bus-Diagnostic-Tool

# Build & upload firmware (PlatformIO)
pio run --target upload
