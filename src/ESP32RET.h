#pragma once

#include <Arduino.h>
#include "esp32_can.h"

void loadSettings();
void processDigToggleFrame(CAN_FRAME &frame);
void sendDigToggleMsg();
void sendMarkTriggered(int which);
