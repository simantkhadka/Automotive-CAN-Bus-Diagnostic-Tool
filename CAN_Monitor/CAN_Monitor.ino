// Reads and prints CAN frame bits when a change is detected

#include <esp32_can.h>

#define TARGET_CAN_ID 0x273  // Renamed for clarity

unsigned long lastPrintTime = 0;
const unsigned long printInterval = 0;

CAN_FRAME previousFrame;
bool firstFrame = true;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting CAN Bit Tracker...");

  // Setup CAN RX/TX pins (GPIO 4 = RX, GPIO 5 = TX)
  CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);
  CAN0.begin(500000);  // Start CAN at 500kbps
  CAN0.watchFor(TARGET_CAN_ID);  // Watch for a specific CAN ID
}

void loop() {
  CAN_FRAME currentFrame;

  if (CAN0.read(currentFrame)) {
    if (currentFrame.id == TARGET_CAN_ID) {
      if (firstFrame || !framesEqual(previousFrame, currentFrame)) {
        previousFrame = currentFrame;
        lastPrintTime = millis();
        printFrameBits(previousFrame);
        firstFrame = false;
      }
    }
  }
}

// Compare frame data byte by byte
bool framesEqual(const CAN_FRAME &frame1, const CAN_FRAME &frame2) {
  if (frame1.length != frame2.length) return false;
  for (int i = 0; i < frame1.length; i++) {
    if (frame1.data.byte[i] != frame2.data.byte[i]) return false;
  }
  return true;
}

// Print frame data in binary format
void printFrameBits(CAN_FRAME &frame) {
  if (frame.length < 1) return;

  Serial.println("---- New Frame Detected ----");
  Serial.print("Timestamp: ");
  Serial.println(millis());

  for (int i = 0; i < frame.length; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    printByteInBinary(frame.data.byte[i]);
  }

  Serial.println("----------------------------\n");
}

// Helper: Print a byte in binary
void printByteInBinary(uint8_t byte) {
  for (int i = 7; i >= 0; i--) {
    Serial.print((byte >> i) & 0x01);
  }
  Serial.println();
}
