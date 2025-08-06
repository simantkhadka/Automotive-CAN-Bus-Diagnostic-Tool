#include <esp32_can.h>

// LED pin definitions
#define SHIELD_LED_GREEN 26  // Green LED for successful CAN initialization
#define SHIELD_LED_BLUE  27  // Blue LED blinks on message receive

unsigned long lastRequestTime = 0;
const unsigned long requestInterval = 3000; // Send RPM request every 3 seconds

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(SHIELD_LED_GREEN, OUTPUT);
  pinMode(SHIELD_LED_BLUE, OUTPUT);
  digitalWrite(SHIELD_LED_GREEN, LOW);
  digitalWrite(SHIELD_LED_BLUE, LOW);

  Serial.println("\n===============================");
  Serial.println("         CAN SHIELD TEST        ");
  Serial.println("===============================");
  Serial.println("Initializing CAN interface...");

  CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);  // Adjust pins for your board

  if (CAN0.begin(500000)) {
    Serial.println("✅ CAN initialized at 500Kbps");
    digitalWrite(SHIELD_LED_GREEN, HIGH);
  } else {
    Serial.println("❌ CAN initialization failed!");
    return;
  }

  CAN0.watchFor();  // Accept all messages
  Serial.println("Ready to send RPM PID requests.\n");
}

void loop() {
  // Periodically send RPM request
  if (millis() - lastRequestTime >= requestInterval) {
    lastRequestTime = millis();

    CAN_FRAME rpmRequest;
    rpmRequest.id = 0x7DF;
    rpmRequest.extended = false;
    rpmRequest.length = 8;
    rpmRequest.data.byte[0] = 0x02;
    rpmRequest.data.byte[1] = 0x01;
    rpmRequest.data.byte[2] = 0x0C;
    for (int i = 3; i < 8; i++) rpmRequest.data.byte[i] = 0x00;

    if (CAN0.sendFrame(rpmRequest)) {
      Serial.println("📤 Sent: PID 010C (Engine RPM)");
    } else {
      Serial.println("❌ Failed to send PID 010C");
    }
  }

  // Receive and display CAN messages
  CAN_FRAME rx;
  if (CAN0.read(rx)) {
    digitalWrite(SHIELD_LED_BLUE, HIGH);
    delay(50);
    digitalWrite(SHIELD_LED_BLUE, LOW);

    Serial.print("📥 Received: ID=0x");
    Serial.print(rx.id, HEX);
    Serial.print(" [");
    Serial.print(rx.length);
    Serial.print("] Data: ");

    for (int i = 0; i < rx.length; i++) {
      if (i > 0) Serial.print(":");
      Serial.print(rx.data.byte[i], HEX);
    }
    Serial.println();

    // Check for Engine RPM reply
    if ((rx.id >= 0x7E8 && rx.id <= 0x7EF) &&
        rx.data.byte[1] == 0x41 && rx.data.byte[2] == 0x0C) {
      int A = rx.data.byte[3];
      int B = rx.data.byte[4];
      int rpm = ((A * 256) + B) / 4;
      Serial.print("🌀 Engine RPM: ");
      Serial.print(rpm);
      Serial.println(" RPM\n");
    }
  }
}
