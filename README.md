ESP32 CAN Bus Diagnostic Tool
A compact, open-source automotive diagnostic tool built on the ESP32 platform for real-time CAN bus monitoring, multi-mode OBD-II requests, and ECU data analysis.

Features
CAN Bus Communication — Supports 11-bit standard CAN frames at configurable baud rates.

OBD-II Modes 01–06 Support:

Mode 01 — Live sensor data (e.g., Engine RPM, Vehicle Speed, Coolant Temp).

Mode 02 — Freeze frame data at the time of a fault.

Mode 03 — Stored Diagnostic Trouble Codes (DTCs).

Mode 04 — Clear DTCs and reset MIL.

Mode 05 — Oxygen sensor monitoring (for older protocols).

Mode 06 — On-board monitoring test results (e.g., catalyst efficiency, EGR performance).

Live Data Streaming — Capture and display CAN traffic for diagnostics and reverse engineering.

LED Status Indicators — Green LED for successful CAN initialization, blue LED for message activity.

Cross-Platform Connectivity — Works via USB or Bluetooth for integration with laptops, mobile devices, and diagnostic platforms.

Open and Extensible — Modular codebase designed for easy customization and feature expansion.

Use Cases
Full-spectrum OBD-II diagnostics and troubleshooting.

ECU reverse engineering.

Automotive R&D.

Educational projects involving CAN bus and diagnostics.

Requirements
ESP32 development board.

Compatible CAN transceiver/shield.

Arduino IDE or PlatformIO for firmware flashing.

License
Released under the MIT License.
Free to use, modify, and distribute.
