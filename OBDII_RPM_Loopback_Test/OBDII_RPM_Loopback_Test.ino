#include <esp32_can.h>

#define GREEN_LED 26  // Green = CAN OK
#define BLUE_LED 27   // Blue = message received

void setup() {
  Serial.begin(115200);
  delay(200);  // Allow serial monitor to connect

  // Set up indicator LEDs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  Serial.println("Booting...");

  // Set CAN TX/RX pins for MrDIY shield (TX = 5, RX = 4)
  CAN0.setCANPins(GPIO_NUM_5, GPIO_NUM_4);

  // Start CAN at 500 kbps, in loopback mode
  if (!CAN0.begin(500000, true)) {
    Serial.println("❌ CAN start failed");
    while (1)
      ;  // Stop if CAN fails
  }

  // Enable receiving messages
  CAN0.watchFor();

  Serial.println("✅ CAN started in loopback mode");
  digitalWrite(GREEN_LED, HIGH);  // Green LED = OK
}

void loop() {
  CAN_FRAME test;

  // Step 1: Create test message (simulate a simple frame)
  test.id = 0x123;  // Arbitrary test ID
  test.length = 2;
  test.extended = false;
  test.rtr = 0;
  test.data.byte[0] = 0xAB;  // Arbitrary data
  test.data.byte[1] = 0xCD;

  Serial.println("📤 Sending test CAN frame...");
  CAN0.sendFrame(test);

  delay(200);  // Give time for loopback

  CAN_FRAME incoming;
  if (CAN0.read(incoming)) {
    Serial.print("✅ Received frame: ID 0x");
    Serial.print(incoming.id, HEX);
    Serial.print(" Data: ");
    for (int i = 0; i < incoming.length; i++) {
      Serial.print(incoming.data.byte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Blink blue LED to show data received
    digitalWrite(BLUE_LED, HIGH);
    delay(200);
    digitalWrite(BLUE_LED, LOW);
  } else {
    Serial.println("⏳ Still no message in buffer");
  }

  delay(1000);
}
