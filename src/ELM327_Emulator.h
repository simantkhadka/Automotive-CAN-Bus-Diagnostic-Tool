#ifndef ELM327_H_
#define ELM327_H_

#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include "commbuffer.h"

class CAN_FRAME;

class ELM327Emu {
public:

    ELM327Emu();
    void setup(); //initialization on start up
    void handleTick(); //periodic processes
    void loop();
    void setWiFiClient(WiFiClient *client);
    void sendCmd(String cmd);
    void processCANReply(CAN_FRAME &frame);
    bool getMonitorMode();

private:
    BluetoothSerial serialBT;
    WiFiClient *mClient;
    CommBuffer txBuffer;
    char incomingBuffer[128]; //storage for one incoming line
    char buffer[30]; // a buffer for various string conversions
    bool bLineFeed; //should we use line feeds?
    bool bHeader; //should we produce a header?
    bool bEcho; //should we echo back anything sent to us?
    bool bMonitorMode; //should we output all frames?
    bool bDLC; //output DLC?
    uint32_t ecuAddress;
    int tickCounter;
    int ibWritePtr;
    int currReply;

    void processCmd();
    String processELMCmd(char *cmd);
    void sendTxBuffer();
};

#endif
