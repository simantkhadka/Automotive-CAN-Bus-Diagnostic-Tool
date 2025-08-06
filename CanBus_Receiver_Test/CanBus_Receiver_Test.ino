#include <esp32_can.h>

// Define the LED pins
#define SHIELD_LED_PIN     26  // Green LED for CAN initialization
#define SHIELD_LED_PIN_2   27  // Blue LED for CAN message received

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(SHIELD_LED_PIN, OUTPUT);
  pinMode(SHIELD_LED_PIN_2, OUTPUT);
  digitalWrite(SHIELD_LED_PIN, LOW);
  digitalWrite(SHIELD_LED_PIN_2, LOW);

  Serial.println("------------------------");
  Serial.println("    CAN BUS  ");
  Serial.println("------------------------");

  Serial.println(" CAN...............INIT");
  CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);  //  RX = GPIO 4, TX = GPIO 5

  if (CAN0.begin(500000)) {
    Serial.println(" ✅ CAN...........500Kbps");
    digitalWrite(SHIELD_LED_PIN, HIGH);  // Green LED ON after successful CAN init
  } else {
    Serial.println(" ❌ CAN Init failed");
  }

  CAN0.watchFor();  // Optional: no filters = receive everything
  
}

void loop() {
  CAN_FRAME can_message;

  if (CAN0.read(can_message)) {
    digitalWrite(SHIELD_LED_PIN_2, HIGH);  // Blink blue LED on message receive
    delay(50);
    digitalWrite(SHIELD_LED_PIN_2, LOW);

    // Print received CAN frame
    Serial.print("CAN MSG: 0x");
    Serial.print(can_message.id, HEX);
    Serial.print(" [");
    Serial.print(can_message.length, DEC);
    Serial.print("] <");
    for (int i = 0; i < can_message.length; i++) {
      if (i != 0) Serial.print(":");
      Serial.print(can_message.data.byte[i], HEX);
    }
    Serial.println(">");
  }
}
