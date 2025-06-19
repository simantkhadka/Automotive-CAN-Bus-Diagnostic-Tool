#include <esp32_can.h>
#include <esp_now.h>
#include <WiFi.h>

// Used later if I want to send CAN data wirelessly
// uint8_t receiver_mac[] = {0xc0, 0x5d, 0x89, 0xdd, 0x24, 0xe0};

struct can_struct {
  uint16_t id;
  uint8_t length;
  uint8_t message[8];
};

can_struct CANMessage;

#define SHIELD_LED_PIN 26  // LED1 - turns on when CAN is initialized
#define SHIELD_LED_PIN_2 27  // LED2 - blinks when message is received

void setup() {
  Serial.begin(115200);

  pinMode(SHIELD_LED_PIN, OUTPUT);
  pinMode(SHIELD_LED_PIN_2, OUTPUT);
  digitalWrite(SHIELD_LED_PIN, LOW);
  digitalWrite(SHIELD_LED_PIN_2, LOW);

  Serial.println("------------------------");
  Serial.println("     MrDIY CAN SHIELD");
  Serial.println("------------------------");

  // Set CAN RX to GPIO 5 and TX to GPIO 4 (for v1.3 shield)
  CAN0.setCANPins(GPIO_NUM_5, GPIO_NUM_4);
  CAN0.setDebuggingMode(true);     // For serial debug prints
  CAN0.begin(500000);              // 500 Kbps CAN speed
  CAN0.watchFor();                 // Watch for all messages

  Serial.println("CAN ready at 500Kbps");

  digitalWrite(SHIELD_LED_PIN, HIGH);  // Turn on LED1 after init
}

void loop() {
  CAN_FRAME can_message;

  // Try reading a CAN message
  if (CAN0.read(can_message)) {
    digitalWrite(SHIELD_LED_PIN_2, HIGH);  // Turn on LED2 when message received

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
  } else {
    digitalWrite(SHIELD_LED_PIN_2, LOW);  // Turn off LED2 if nothing read
  }

  delay(100);  // Add small delay to reduce serial spam
}
