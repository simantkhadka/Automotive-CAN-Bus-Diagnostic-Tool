# 🚗 Automotive CAN Bus Diagnostic Tool  

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)  
[![GitHub stars](https://img.shields.io/github/stars/simantkhadka/CAN-BUS-Diagonostic-tool?style=social)](https://github.com/simantkhadka/CAN-BUS-Diagonostic-tool/stargazers)  
[![GitHub last commit](https://img.shields.io/github/last-commit/simantkhadka/CAN-BUS-Diagonostic-tool)](https://github.com/simantkhadka/CAN-BUS-Diagonostic-tool)  

---

## 📖 Overview  

The **Automotive CAN Bus Diagnostic Tool** is an **ESP32-based open-source platform** for analyzing and interacting with vehicle networks through the **OBD-II port**.  

It enables:  
- 📊 Real-time vehicle data monitoring  
- 🔍 CAN traffic sniffing and reverse engineering  
- 🛠️ Diagnostic Trouble Code (DTC) reading/clearing  
- 🌐 Connectivity via USB, Bluetooth, and Wi-Fi  

This project bridges **embedded firmware, automotive diagnostics, and cybersecurity research** — providing a professional-grade tool for enthusiasts, engineers, and researchers.  

---

## ⚡ Key Features  

| Feature | Description |  
|---------|-------------|  
| 🛠️ **OBD-II Modes 01–06** | Full support for **live data**, freeze frames, DTC retrieval/clearing, O₂ sensor monitoring, and on-board test results — a **complete diagnostic workflow**. |  
| 📡 **Real-Time CAN Analysis** | Sniff, log, and replay raw CAN frames for **reverse engineering, anomaly detection, and hidden signal discovery** across ECUs. |  
| 🔗 **Flexible Connectivity** | Access via **USB Serial**, **Bluetooth ELM327 emulation** (works with apps like Torque, Car Scanner), or **Wi-Fi Access Point** with built-in dashboard/API. |  
| ⚡ **Portable ESP32 Design** | Compact hardware with **LED indicators** for CAN init and RX/TX activity. Reliable, plug-and-play, works across vehicles. |  
| 🔒 **Security & Research Ready** | Inject **custom CAN frames** and perform **MITM experiments** — enabling **automotive cybersecurity research** and educational use. |  
| 🌍 **Cross-Platform & Extensible** | Works on **Windows, Linux, macOS, and mobile devices**. Modular firmware makes it easy to add new PIDs and extend features. |  

---

## 🔌 Hardware Reference  

- **OBD-II Connector Pinout**  
  ![OBD-II Pinout](images/obd2-pinout.png)  

- **CAN Bus Signaling (CAN High & CAN Low)**  
  ![CAN Bus Wiring](images/canbus-wiring.png)  

---

## 🛠️ Getting Started  

### Requirements  
- **Hardware**: ESP32 Dev Board, MCP2515 CAN module/shield, OBD-II adapter cable  
- **Software**: PlatformIO (recommended) or Arduino IDE  

### Installation  
```bash
# Clone the repository
git clone https://github.com/simantkhadka/CAN-BUS-Diagonostic-tool.git
cd CAN-BUS-Diagonostic-tool

# Build & upload firmware (PlatformIO)
pio run --target upload
