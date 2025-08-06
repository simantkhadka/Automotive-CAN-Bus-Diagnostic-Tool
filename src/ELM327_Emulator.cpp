/*
 * ELM327_Emu.cpp
 *
 * This file implements an ELM327 emulator class.
 * It provides a Bluetooth or WiFi-based OBD-II interface that
 * communicates with a vehicle's CAN bus using standard OBD-II protocols.
 *
 * Key features:
 * - Accepts ELM327 AT commands over Bluetooth Serial or TCP.
 * - Parses and responds to basic AT commands.
 * - Sends OBD-II PID requests over CAN.
 * - Processes and returns CAN replies in ELM327-compatible format.
 */

#include "ELM327_Emulator.h"
#include "BluetoothSerial.h"
#include "config.h"
#include "Logger.h"
#include "utility.h"
#include "esp32_can.h"
#include "can_manager.h"

// Constructor - initialize state variables
ELM327Emu::ELM327Emu()
{
    tickCounter = 0;
    ibWritePtr = 0;
    ecuAddress = 0x7DF; // Default ECU address for engine control
    mClient = 0;
    bEcho = false;
    bHeader = false;
    bLineFeed = true;
    bMonitorMode = false;
    bDLC = false;
}

// Initialize Bluetooth with the configured device name
void ELM327Emu::setup()
{
    serialBT.begin(settings.btName);
}

// Assign an active WiFi client for TCP mode
void ELM327Emu::setWiFiClient(WiFiClient *client)
{
    mClient = client;
}

// Returns true if the emulator is in CAN monitor mode
bool ELM327Emu::getMonitorMode()
{
    return bMonitorMode;
}

// Send an AT command to the connected client (over BT or WiFi)
void ELM327Emu::sendCmd(String cmd)
{
    txBuffer.sendString("AT");
    txBuffer.sendString(cmd);
    txBuffer.sendByteToBuffer(13); // CR
    sendTxBuffer();
    loop(); // Immediately process response
}

// Reads and processes incoming data from Bluetooth or WiFi client.
// When a complete line is received (terminated by CR), it is passed to processCmd().
void ELM327Emu::loop()
{
    int incoming;
    if (!mClient) // Bluetooth mode
    {
        while (serialBT.available())
        {
            incoming = serialBT.read();
            if (incoming != -1)
            {
                if (incoming == 13 || ibWritePtr > 126)
                {
                    incomingBuffer[ibWritePtr] = 0;
                    ibWritePtr = 0;
                    if (Logger::isDebug())
                        Logger::debug(incomingBuffer);
                    processCmd();
                }
                else
                {
                    if (incoming > 20 && bMonitorMode)
                    {
                        Logger::debug("Exiting monitor mode");
                        bMonitorMode = false;
                    }
                    if (incoming != 10 && incoming != ' ')
                        incomingBuffer[ibWritePtr++] = (char)tolower(incoming);
                }
            }
            else
                return;
        }
    }
    else // WiFi mode
    {
        while (mClient->available())
        {
            incoming = mClient->read();
            if (incoming != -1)
            {
                if (incoming == 13 || ibWritePtr > 126)
                {
                    incomingBuffer[ibWritePtr] = 0;
                    ibWritePtr = 0;
                    if (Logger::isDebug())
                        Logger::debug(incomingBuffer);
                    processCmd();
                }
                else
                {
                    if (incoming != 10 && incoming != ' ')
                        incomingBuffer[ibWritePtr++] = (char)tolower(incoming);
                }
            }
            else
                return;
        }
    }
}

// Send the current TX buffer contents over Bluetooth or WiFi
void ELM327Emu::sendTxBuffer()
{
    if (mClient)
    {
        size_t wifiLength = txBuffer.numAvailableBytes();
        uint8_t *buff = txBuffer.getBufferedBytes();
        if (mClient->connected())
        {
            mClient->write(buff, wifiLength);
        }
    }
    else
    {
        serialBT.write(txBuffer.getBufferedBytes(), txBuffer.numAvailableBytes());
    }
    txBuffer.clearBufferedBytes();
}

// Process a complete incoming AT or PID command string
void ELM327Emu::processCmd()
{
    String retString = processELMCmd(incomingBuffer);
    txBuffer.sendString(retString);
    sendTxBuffer();
    if (Logger::isDebug())
    {
        char buff[300];
        retString = "Reply:" + retString;
        retString.toCharArray(buff, 300);
        Logger::debug(buff);
    }
}

// Interpret an AT command or PID request and return the reply string
String ELM327Emu::processELMCmd(char *cmd)
{
    String retString;
    String lineEnding = bLineFeed ? "\r\n" : "\r";

    if (bEcho)
    {
        retString.concat(cmd);
        retString.concat(lineEnding);
    }

    if (!strncmp(cmd, "at", 2))
    {
        // Handle AT commands
        if (!strcmp(cmd, "atz"))
        {
            retString.concat(lineEnding);
            retString.concat("ELM327 v1.3a");
        }
        else if (!strncmp(cmd, "atsh", 4))
        {
            size_t idSize = strlen(cmd + 4);
            ecuAddress = Utility::parseHexString(cmd + 4, idSize);
            Logger::debug("New ECU address: %x", ecuAddress);
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "ate", 3))
        {
            bEcho = (cmd[3] == '1');
        }
        else if (!strncmp(cmd, "ath", 3))
        {
            bHeader = (cmd[3] == '1');
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atl", 3))
        {
            bLineFeed = (cmd[3] == '1');
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "at@1"))
        {
            retString.concat("OBDLink MX");
        }
        else if (!strcmp(cmd, "ati"))
        {
            retString.concat("ELM327 v1.5");
        }
        else if (!strncmp(cmd, "atat", 4) || !strncmp(cmd, "atsp", 4))
        {
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "atdp"))
        {
            retString.concat("can11/500");
        }
        else if (!strcmp(cmd, "atdpn"))
        {
            retString.concat("6");
        }
        else if (!strncmp(cmd, "atd0", 4))
        {
            bDLC = false;
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atd1", 4))
        {
            bDLC = true;
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "atd"))
        {
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atma", 4))
        {
            Logger::debug("ENTERING monitor mode");
            bMonitorMode = true;
        }
        else if (!strncmp(cmd, "atm", 3))
        {
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "atrv"))
        {
            retString.concat("14.2V");
        }
        else
        {
            retString.concat("OK");
        }
    }
    else
    {
        // PID request
        CAN_FRAME outFrame;
        outFrame.id = ecuAddress;
        outFrame.extended = false;
        outFrame.length = 8;
        outFrame.rtr = 0;
        outFrame.data.byte[3] = 0xAA;
        outFrame.data.byte[4] = 0xAA;
        outFrame.data.byte[5] = 0xAA;
        outFrame.data.byte[6] = 0xAA;
        outFrame.data.byte[7] = 0xAA;

        size_t cmdSize = strlen(cmd);
        if (cmdSize == 4)
        {
            uint32_t valu = strtol(cmd, NULL, 16);
            uint8_t pidnum = (uint8_t)(valu & 0xFF);
            uint8_t mode = (uint8_t)((valu >> 8) & 0xFF);
            Logger::debug("Mode: %i, PID: %i", mode, pidnum);
            outFrame.data.byte[0] = 2;
            outFrame.data.byte[1] = mode;
            outFrame.data.byte[2] = pidnum;
        }
        if (cmdSize == 6)
        {
            uint32_t valu = strtol(cmd, NULL, 16);
            uint16_t pidnum = (uint8_t)(valu & 0xFFFF);
            uint8_t mode = (uint8_t)((valu >> 16) & 0xFF);
            Logger::debug("Mode: %i, PID: %i", mode, pidnum);
            outFrame.data.byte[0] = 3;
            outFrame.data.byte[1] = mode;
            outFrame.data.byte[2] = pidnum >> 8;
            outFrame.data.byte[3] = pidnum & 0xFF;
        }

        canManager.sendFrame(&CAN0, outFrame);
    }

    retString.concat(lineEnding);
    retString.concat(">"); // ELM prompt
    return retString;
}

// Package and send a CAN reply in ELM327-compatible text format
void ELM327Emu::processCANReply(CAN_FRAME &frame)
{
    char buff[8];
    if (bHeader || bMonitorMode)
    {
        sprintf(buff, "%03X", frame.id);
        txBuffer.sendString(buff);
    }
    if (bDLC)
    {
        sprintf(buff, "%u", frame.length);
        txBuffer.sendString(buff);
    }
    for (int i = 0; i < frame.data.byte[0]; i++)
    {
        sprintf(buff, "%02X", frame.data.byte[1 + i]);
        txBuffer.sendString(buff);
    }
    sendTxBuffer();
}
