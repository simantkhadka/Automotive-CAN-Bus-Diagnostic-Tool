/*
 * config.h
 *
 * allows the user to configure static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.
 *
 * Copyright (c) 2013-2020 Collin Kidder, Michael Neuweiler, Charles Galpin
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <WiFi.h>
#include "esp32_can.h"
#include <Preferences.h>

// Buffer sizes / timings
#define SER_BUFF_SIZE 1024            // serial write buffer
#define WIFI_BUFF_SIZE 2048           // GVRET/ELM TCP buffer (fits within typical 2312 MTU)
#define SER_BUFF_FLUSH_INTERVAL 20000 // us between forced flushes

// Build / prefs / names
#define CFG_BUILD_NUM 618
#define CFG_VERSION "Alpha Nov 29 2020"
#define PREF_NAME "ESP32RET"
#define EVTV_NAME "ESP32RET"
#define MACC_NAME "CAN"

// Buses
#define NUM_BUSES 5

// LED blink pacing (higher = slower)
#define BLINK_SLOWNESS 100

// Board control pins
#define SW_EN 2
#define SW_MODE0 26
#define SW_MODE1 27

// WiFi
#define MAX_CLIENTS 1

// Optional CAN filter struct (currently unused but kept for compatibility)
struct FILTER
{
    uint32_t id;
    uint32_t mask;
    boolean extended;
    boolean enabled;
} __attribute__((__packed__));

struct CANFDSettings
{
    uint32_t nomSpeed;
    uint32_t fdSpeed;
    boolean enabled;
    boolean listenOnly;
    boolean fdMode;
};

struct EEPROMSettings
{
    CANFDSettings canSettings[NUM_BUSES];

    boolean useBinarySerialComm; // use binary protocol for frames over serial?

    uint8_t logLevel;   // 0..4
    uint8_t systemType; // 0 = A0RET, 1 = EVTV ESP32 Board, 2 = Macchina 5-CAN

    boolean enableBT; // enable Bluetooth ELM?
    char btName[32];

    // WiFi settings
    uint8_t wifiMode; // 0 = off, 1 = connect to AP, 2 = create AP
    char SSID[32];
    char WPA2Key[64];
} __attribute__((__packed__));

struct SystemSettings
{
    // LED pins (set to 255 to disable a given LED)
    uint8_t LED_CANTX;
    uint8_t LED_CANRX;
    uint8_t LED_LOGGING;
    uint8_t LED_CONNECTION_STATUS;

    // LED state toggles
    boolean txToggle;
    boolean rxToggle;
    boolean logToggle;

    // bus / connectivity state
    int8_t numBuses;
    WiFiClient clientNodes[MAX_CLIENTS];
    WiFiClient wifiOBDClients[MAX_CLIENTS];
    boolean isWifiConnected;
    boolean isWifiActive;
};

class GVRET_Comm_Handler;
class CANManager;
class ELM327Emu;

extern EEPROMSettings settings;
extern SystemSettings SysSettings;
extern Preferences nvPrefs;
extern GVRET_Comm_Handler serialGVRET;
extern GVRET_Comm_Handler wifiGVRET;
extern CANManager canManager;
extern ELM327Emu elmEmulator;
extern char deviceName[20];
extern char otaHost[40];
extern char otaFilename[100];
extern CAN_COMMON *canBuses[NUM_BUSES];

#endif /* CONFIG_H_ */
