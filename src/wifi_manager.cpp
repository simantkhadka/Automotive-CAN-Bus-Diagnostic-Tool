#include "config.h"
#include "wifi_manager.h"
#include "gvret_comm.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include "ELM327_Emulator.h"

static IPAddress broadcastAddr(255, 255, 255, 255);

WiFiManager::WiFiManager()
{
    lastBroadcast = 0;
}

void WiFiManager::setup()
{
    if (settings.wifiMode == 1) // Station mode: connect to existing AP
    {
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(true);
        WiFi.begin((const char *)settings.SSID, (const char *)settings.WPA2Key);

        // Handle WiFi disconnection events
        WiFi.onEvent([](WiFiEvent_t /*event*/, WiFiEventInfo_t info)
                     {
            Serial.print("WiFi lost connection. Reason: ");
            Serial.println(info.wifi_sta_disconnected.reason);
            SysSettings.isWifiConnected = false;

            // Hard-reboot for certain failure reasons
            if (info.wifi_sta_disconnected.reason == 202 || info.wifi_sta_disconnected.reason == 3)
            {
                Serial.println("Connection failed, rebooting to recover.");
                esp_sleep_enable_timer_wakeup(10);
                esp_deep_sleep_start();
                delay(100);
            } }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    }
    else if (settings.wifiMode == 2) // AP mode: create our own network
    {
        WiFi.mode(WIFI_AP);
        WiFi.setSleep(true);
        WiFi.softAP((const char *)settings.SSID, (const char *)settings.WPA2Key);

        // Optional: board-specific control pin example
        pinMode(26, OUTPUT);
        digitalWrite(26, HIGH);
    }
}

void WiFiManager::loop()
{
    bool needServerInit = false;

    if (settings.wifiMode > 0) // WiFi enabled
    {
        if (!SysSettings.isWifiConnected) // first connection attempt
        {
            if (WiFi.isConnected()) // STA connected
            {
                Serial.print("WiFi connected to SSID: ");
                Serial.println((const char *)settings.SSID);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.print("RSSI: ");
                Serial.println(WiFi.RSSI());
                needServerInit = true;
            }
            if (settings.wifiMode == 2) // AP mode
            {
                Serial.print("AP SSID: ");
                Serial.println((const char *)settings.SSID);
                Serial.println("Password: Nepal123");
                Serial.print("AP IP: ");
                Serial.println(WiFi.softAPIP());
                needServerInit = true;
            }

            if (needServerInit) // Setup mDNS and TCP servers
            {
                SysSettings.isWifiConnected = true;

                if (!MDNS.begin(deviceName))
                {
                    Serial.println("Error setting up mDNS responder!");
                }
                MDNS.addService("telnet", "tcp", 23);
                MDNS.addService("ELM327", "tcp", 1000);

                wifiServer.begin(23);
                wifiServer.setNoDelay(true);
                wifiOBDII.begin(1000);
                wifiOBDII.setNoDelay(true);

                Serial.println();
                Serial.println("WiFi services ready.");
            }
        }
        else // Already connected
        {
            if (WiFi.isConnected() || settings.wifiMode == 2)
            {
                // Accept new GVRET telnet clients
                if (wifiServer.hasClient())
                {
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (!SysSettings.clientNodes[i] || !SysSettings.clientNodes[i].connected())
                        {
                            if (SysSettings.clientNodes[i])
                                SysSettings.clientNodes[i].stop();
                            SysSettings.clientNodes[i] = wifiServer.available();
                            if (!SysSettings.clientNodes[i])
                            {
                                Serial.println("Couldn't accept telnet client!");
                            }
                            else
                            {
                                Serial.print("New GVRET client slot ");
                                Serial.print(i);
                                Serial.print(" from ");
                                Serial.println(SysSettings.clientNodes[i].remoteIP());
                            }
                            break;
                        }
                    }
                    // Reject if no free slot
                    if (SysSettings.clientNodes[MAX_CLIENTS - 1] && wifiServer.hasClient())
                        wifiServer.available().stop();
                }

                // Accept new ELM327 TCP clients
                if (wifiOBDII.hasClient())
                {
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (!SysSettings.wifiOBDClients[i] || !SysSettings.wifiOBDClients[i].connected())
                        {
                            if (SysSettings.wifiOBDClients[i])
                                SysSettings.wifiOBDClients[i].stop();
                            SysSettings.wifiOBDClients[i] = wifiOBDII.available();
                            if (!SysSettings.wifiOBDClients[i])
                            {
                                Serial.println("Couldn't accept ELM327 client!");
                            }
                            else
                            {
                                Serial.print("New ELM327 client slot ");
                                Serial.print(i);
                                Serial.print(" from ");
                                Serial.println(SysSettings.wifiOBDClients[i].remoteIP());
                            }
                            break;
                        }
                    }
                    // Reject if no free slot
                    if (SysSettings.wifiOBDClients[MAX_CLIENTS - 1] && wifiOBDII.hasClient())
                        wifiOBDII.available().stop();
                }

                // Handle GVRET client input
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
                    {
                        while (SysSettings.clientNodes[i].available())
                        {
                            uint8_t inByt = SysSettings.clientNodes[i].read();
                            SysSettings.isWifiActive = true;
                            wifiGVRET.processIncomingByte(inByt);
                        }
                    }
                    else if (SysSettings.clientNodes[i])
                    {
                        SysSettings.clientNodes[i].stop();
                    }

                    // Link ELM327 WiFi client to emulator
                    if (SysSettings.wifiOBDClients[i] && SysSettings.wifiOBDClients[i].connected())
                    {
                        elmEmulator.setWiFiClient(&SysSettings.wifiOBDClients[i]);
                    }
                    else if (SysSettings.wifiOBDClients[i])
                    {
                        SysSettings.wifiOBDClients[i].stop();
                        elmEmulator.setWiFiClient(nullptr);
                    }
                }
            }
            else if (settings.wifiMode == 1) // STA lost connection
            {
                Serial.println("WiFi disconnected.");
                SysSettings.isWifiConnected = false;
                SysSettings.isWifiActive = false;
            }
        }
    }

    // Broadcast heartbeat every second
    if (SysSettings.isWifiConnected && ((micros() - lastBroadcast) > 1000000ul))
    {
        uint8_t buff[4] = {0x1C, 0xEF, 0xAC, 0xED};
        lastBroadcast = micros();
        wifiUDPServer.beginPacket(broadcastAddr, 17222);
        wifiUDPServer.write(buff, 4);
        wifiUDPServer.endPacket();
    }
}

void WiFiManager::sendBufferedData()
{
    // Push GVRET buffered output to all connected telnet clients
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        size_t wifiLength = wifiGVRET.numAvailableBytes();
        uint8_t *buff = wifiGVRET.getBufferedBytes();
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected() && wifiLength)
        {
            SysSettings.clientNodes[i].write(buff, wifiLength);
        }
    }
    wifiGVRET.clearBufferedBytes();
}
