#include "config.h"
#include <esp32_can.h>
#include <SPI.h>
#include <Preferences.h>
#include "Logger.h" // <-- needed for Logger::setLoglevel
#include "ELM327_Emulator.h"
#include "wifi_manager.h"
#include "gvret_comm.h"
#include "can_manager.h"

// Buffer flush timing
uint32_t lastFlushMicros = 0;

EEPROMSettings settings;
SystemSettings SysSettings;
Preferences nvPrefs;
char deviceName[20];
char otaHost[40];      // consider removing + extern in config.h if unused
char otaFilename[100]; // consider removing + extern in config.h if unused

ELM327Emu elmEmulator;
WiFiManager wifiManager;

GVRET_Comm_Handler serialGVRET; // gvret protocol over the serial to USB connection
GVRET_Comm_Handler wifiGVRET;   // GVRET over the wifi telnet port
CANManager canManager;          // keeps track of bus load and abstracts away some details

CAN_COMMON *canBuses[NUM_BUSES];

void loadSettings()
{
    for (int i = 0; i < NUM_BUSES; i++)
        canBuses[i] = nullptr;

    nvPrefs.begin(PREF_NAME, false);

    settings.useBinarySerialComm = nvPrefs.getBool("binarycomm", false);
    settings.logLevel = nvPrefs.getUChar("loglevel", 1); // info
    settings.wifiMode = nvPrefs.getUChar("wifiMode", 2); // default: create AP
    settings.enableBT = nvPrefs.getBool("enable-bt", false);
    settings.systemType = 0; // A0

    if (settings.systemType == 0)
    {
        canBuses[0] = &CAN0;
        SysSettings.LED_CANTX = 26;
        SysSettings.LED_CANRX = 27;
        SysSettings.LED_LOGGING = 35;
        SysSettings.LED_CONNECTION_STATUS = 0;
        SysSettings.logToggle = false;
        SysSettings.txToggle = true;
        SysSettings.rxToggle = true;
        SysSettings.numBuses = 1;
        SysSettings.isWifiActive = false;
        SysSettings.isWifiConnected = false;

        strcpy(deviceName, MACC_NAME);
        strcpy(otaHost, "");
        strcpy(otaFilename, "");

        pinMode(SysSettings.LED_CANTX, OUTPUT);
        pinMode(SysSettings.LED_CANRX, OUTPUT);
        digitalWrite(SysSettings.LED_CANTX, LOW);
        digitalWrite(SysSettings.LED_CANRX, LOW);

        delay(100);
        CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5); // shield pins
    }

    if (nvPrefs.getString("SSID", settings.SSID, 32) == 0)
    {
        strcpy(settings.SSID, deviceName);
        strcat(settings.SSID, "-Wireless");
    }

    if (nvPrefs.getString("wpa2Key", settings.WPA2Key, 64) == 0)
    {
        strcpy(settings.WPA2Key, "Nepal123");
    }
    if (nvPrefs.getString("btname", settings.btName, 32) == 0)
    {
        strcpy(settings.btName, "OBDII");
    }

    char buff[80];
    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        sprintf(buff, "can%ispeed", i);
        settings.canSettings[i].nomSpeed = nvPrefs.getUInt(buff, 500000);
        sprintf(buff, "can%i_en", i);
        settings.canSettings[i].enabled = nvPrefs.getBool(buff, (i < 2) ? true : false);
        sprintf(buff, "can%i-listenonly", i);
        settings.canSettings[i].listenOnly = nvPrefs.getBool(buff, false);
        sprintf(buff, "can%i-fdspeed", i);
        settings.canSettings[i].fdSpeed = nvPrefs.getUInt(buff, 5000000);
        sprintf(buff, "can%i-fdmode", i);
        settings.canSettings[i].fdMode = nvPrefs.getBool(buff, false);
    }

    nvPrefs.end();

    Logger::setLoglevel((Logger::LogLevel)settings.logLevel);
}

void setup()
{
    Serial.begin(115200);
    SysSettings.isWifiConnected = false;

    loadSettings();

    settings.enableBT = true;
    strcpy(settings.btName, "OBDII");
    elmEmulator.setup();

    wifiManager.setup();

    Serial.println();
    Serial.println("===================================");
    Serial.println("        CAN-BUS DIAGONOSTICS        ");
    Serial.println("===================================");
    Serial.println();

    canManager.setup();
}

/* Kept for compatibility with old headers; delete if you also remove the prototype. */
void sendMarkTriggered(int which)
{
    CAN_FRAME frame;
    frame.id = 0xFFFFFFF8ull + which;
    frame.extended = true;
    frame.length = 0;
    frame.rtr = 0;
    canManager.displayFrame(frame, 0);
}

void loop()
{
    canManager.loop();
    wifiManager.loop();

    const size_t wifiLength = wifiGVRET.numAvailableBytes();
    const size_t serialLength = serialGVRET.numAvailableBytes();
    const size_t maxLength = (wifiLength > serialLength) ? wifiLength : serialLength;

    // flush buffered data periodically or when buffers are nearly full
    if ((micros() - lastFlushMicros > SER_BUFF_FLUSH_INTERVAL) || (maxLength > (WIFI_BUFF_SIZE - 40)))
    {
        lastFlushMicros = micros();

        if (serialLength > 0)
        {
            Serial.write(serialGVRET.getBufferedBytes(), serialLength);
            serialGVRET.clearBufferedBytes();
        }
        if (wifiLength > 0)
        {
            wifiManager.sendBufferedData();
        }
    }

    // pull any serial bytes into GVRET
    int serialCnt = 0;
    while ((Serial.available() > 0) && serialCnt < 128)
    {
        serialCnt++;
        const uint8_t in_byte = (uint8_t)Serial.read();
        serialGVRET.processIncomingByte(in_byte);
    }

    // ELM327 over BT or WiFi
    elmEmulator.loop();
}
