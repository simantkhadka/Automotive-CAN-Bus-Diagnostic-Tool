#include <Arduino.h>
#include "can_manager.h"
#include "esp32_can.h"
#include "config.h"
#include "gvret_comm.h"
#include "ELM327_Emulator.h"

// Set a given LED pin HIGH or LOW
static void setLED(uint8_t which, boolean hi)
{
    if (which == 255) // invalid pin
        return;
    pinMode(which, OUTPUT); // safe even if already set
    digitalWrite(which, hi ? HIGH : LOW);
}

// Blink RX LED at a slower rate to indicate incoming CAN traffic
static void toggleRXLED()
{
    static int counter = 0;
    if (++counter >= BLINK_SLOWNESS)
    {
        counter = 0;
        SysSettings.rxToggle = !SysSettings.rxToggle;
        setLED(SysSettings.LED_CANRX, SysSettings.rxToggle);
    }
}

// Blink TX LED at a slower rate to indicate outgoing CAN traffic
static void toggleTXLED()
{
    static int counter = 0;
    if (++counter >= BLINK_SLOWNESS)
    {
        counter = 0;
        SysSettings.txToggle = !SysSettings.txToggle;
        setLED(SysSettings.LED_CANTX, SysSettings.txToggle);
    }
}
// --- end LED helpers ---

CANManager::CANManager()
{
}

// Initialize CAN buses and related parameters
void CANManager::setup()
{
    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        if (settings.canSettings[i].enabled)
        {
            canBuses[i]->enable();

            // Classic CAN or FD mode initialization
            if ((settings.canSettings[i].fdMode == 0) || !canBuses[i]->supportsFDMode())
            {
                canBuses[i]->begin(settings.canSettings[i].nomSpeed, 255);
                Serial.printf("CAN%u Speed: %u\n", i, settings.canSettings[i].nomSpeed);

                // Special handling for boards with software-controlled transceiver enable pin
                if ((i == 0) && (settings.systemType == 2))
                {
                    digitalWrite(SW_EN, HIGH); // enable SWCAN mode
                    Serial.println("Enabling SWCAN Mode");
                }
                if ((i == 1) && (settings.systemType == 2))
                {
                    digitalWrite(SW_EN, LOW); // enable CAN1, disable CAN0
                    Serial.println("Enabling CAN1 will force CAN0 off.");
                }
            }
            else
            {
                // Initialize in CAN FD mode
                canBuses[i]->beginFD(settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
                Serial.printf("Enabled CAN%u in FD mode, Nominal %u, Data %u\n",
                              i, settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
            }

            // Set listen-only mode if configured
            if (settings.canSettings[i].listenOnly)
                canBuses[i]->setListenOnlyMode(true);
            else
                canBuses[i]->setListenOnlyMode(false);

            canBuses[i]->watchFor(); // accept all frames by default
        }
        else
        {
            canBuses[i]->disable(); // disable bus if not in use
        }
    }

    // Macchina 5-CAN board: configure MCP2517FD standby/output mode
    if (settings.systemType == 2)
    {
        uint8_t stdbymode;
        for (int i = 1; i < 5; i++)
        {
            MCP2517FD *can = (MCP2517FD *)canBuses[i];
            stdbymode = can->Read8(0xE04);
            stdbymode |= 0x40; // enable XSTBY mode
            can->Write8(0xE04, stdbymode);
            stdbymode = can->Read8(0xE04);
            stdbymode &= 0xFE; // GPIO0 as output
            can->Write8(0xE04, stdbymode);
        }
    }

    // Initialize bus load tracking
    for (int j = 0; j < NUM_BUSES; j++)
    {
        busLoad[j].bitsPerQuarter = settings.canSettings[j].nomSpeed / 4;
        busLoad[j].bitsSoFar = 0;
        busLoad[j].busloadPercentage = 0;
        if (busLoad[j].bitsPerQuarter == 0)
            busLoad[j].bitsPerQuarter = 125000; // fail-safe default
    }

    busLoadTimer = millis();
}

// Accumulate bit count for classic CAN frame
void CANManager::addBits(int offset, CAN_FRAME &frame)
{
    if (offset < 0 || offset >= NUM_BUSES)
        return;
    busLoad[offset].bitsSoFar += 41 + (frame.length * 9); // base frame overhead + data
    if (frame.extended)
        busLoad[offset].bitsSoFar += 18; // extra bits for extended ID
}

// Accumulate bit count for CAN FD frame
void CANManager::addBits(int offset, CAN_FRAME_FD &frame)
{
    if (offset < 0 || offset >= NUM_BUSES)
        return;
    busLoad[offset].bitsSoFar += 41 + (frame.length * 9);
    if (frame.extended)
        busLoad[offset].bitsSoFar += 18;
}

// Send classic CAN frame on specified bus and blink TX LED
void CANManager::sendFrame(CAN_COMMON *bus, CAN_FRAME &frame)
{
    int whichBus = 0;
    for (int i = 0; i < NUM_BUSES; i++)
        if (canBuses[i] == bus)
            whichBus = i;
    bus->sendFrame(frame);
    addBits(whichBus, frame);
    toggleTXLED();
}

// Send CAN FD frame on specified bus and blink TX LED
void CANManager::sendFrame(CAN_COMMON *bus, CAN_FRAME_FD &frame)
{
    int whichBus = 0;
    for (int i = 0; i < NUM_BUSES; i++)
        if (canBuses[i] == bus)
            whichBus = i;
    bus->sendFrameFD(frame);
    addBits(whichBus, frame);
    toggleTXLED();
}

// Send a received classic CAN frame to the correct output buffer
void CANManager::displayFrame(CAN_FRAME &frame, int whichBus)
{
    if (SysSettings.isWifiActive)
        wifiGVRET.sendFrameToBuffer(frame, whichBus);
    else
        serialGVRET.sendFrameToBuffer(frame, whichBus);
}

// Send a received CAN FD frame to the correct output buffer
void CANManager::displayFrame(CAN_FRAME_FD &frame, int whichBus)
{
    if (SysSettings.isWifiActive)
        wifiGVRET.sendFrameToBuffer(frame, whichBus);
    else
        serialGVRET.sendFrameToBuffer(frame, whichBus);
}

// Main loop: poll CAN buses, forward frames, track bus load
void CANManager::loop()
{
    CAN_FRAME incoming;
    CAN_FRAME_FD inFD;

    // Track current output buffer usage (wifi vs serial)
    size_t wifiLength = wifiGVRET.numAvailableBytes();
    size_t serialLength = serialGVRET.numAvailableBytes();
    size_t maxLength = (wifiLength > serialLength) ? wifiLength : serialLength;

    // Every 250ms, calculate bus load percentage and reset counter
    if (millis() > (busLoadTimer + 250))
    {
        busLoadTimer = millis();
        busLoad[0].busloadPercentage =
            ((busLoad[0].busloadPercentage * 3) +
             (((busLoad[0].bitsSoFar * 1000) / busLoad[0].bitsPerQuarter) / 10)) /
            4;

        // Minimum 1% if there was any traffic
        if (busLoad[0].busloadPercentage == 0 && busLoad[0].bitsSoFar > 0)
            busLoad[0].busloadPercentage = 1;

        busLoad[0].bitsPerQuarter = settings.canSettings[0].nomSpeed / 4;
        busLoad[0].bitsSoFar = 0;
    }

    // Read from each enabled CAN bus
    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        if (!canBuses[i])
            continue;
        if (!settings.canSettings[i].enabled)
            continue;

        // Read frames while buffer space allows
        while ((canBuses[i]->available() > 0) && (maxLength < (WIFI_BUFF_SIZE - 80)))
        {
            if (settings.canSettings[i].fdMode == 0)
            {
                // Handle classic CAN
                canBuses[i]->read(incoming);
                addBits(i, incoming);
                displayFrame(incoming, i);

                // Forward certain frames to ELM327 emulator
                if ((incoming.id > 0x7DF && incoming.id < 0x7F0) || elmEmulator.getMonitorMode())
                {
                    elmEmulator.processCANReply(incoming);
                }
            }
            else
            {
                // Handle CAN FD
                canBuses[i]->readFD(inFD);
                addBits(i, inFD);
                displayFrame(inFD, i);
            }

            toggleRXLED(); // blink RX LED on any received frame

            // Update buffer usage to avoid overflow
            wifiLength = wifiGVRET.numAvailableBytes();
            serialLength = serialGVRET.numAvailableBytes();
            maxLength = (wifiLength > serialLength) ? wifiLength : serialLength;
        }
    }
}
