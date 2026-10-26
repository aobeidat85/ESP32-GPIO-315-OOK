#include <Gpio315Ook.h>

// Codes captured from one working VEVOR remote. Other units may use different
// fixed codes; capture your own remote if these do not control your lamp.
struct Button { char key; const char* label; uint32_t code; };
constexpr Button buttons[] = {
  {'2', "ON/OFF", 0x3AB602}, {'3', "Radar", 0x3AB608},
  {'4', "Auto",   0x3AB604}, {'5', "2H",    0x3AB601},
  {'6', "3H",     0x3AB60C}, {'7', "4H",    0x3AB607},
  {'8', "+",      0x3AB603}, {'9', "-",     0x3AB60A},
};

Gpio315Ook radio;

#if CONFIG_IDF_TARGET_ESP32C3
constexpr uint8_t REPEATS = 8; // Tested on C3 GPIO9.
#else
constexpr uint8_t REPEATS = 2; // Tested on original ESP32 GPIO0.
#endif

void setup() {
  Serial.begin(115200);
  delay(1200);
  if (!radio.begin()) {
    Serial.println("315 MHz GPIO clock setup failed or chip unsupported");
    return;
  }
  Serial.printf("Ready on GPIO%u. Send keys 2-9.\n", radio.pin());
  Serial.println("READY keys 2-9");
}

void loop() {
  if (!Serial.available()) return;
  const char key = Serial.read();
  if (key == '?') {
    Serial.println("READY keys 2-9");
    return;
  }
  for (const Button& button : buttons) {
    if (key != button.key) continue;
    if (radio.send(button.code, 24, 350, REPEATS)) {
      Serial.printf("SENT %c %s 0x%06lX x%u\n", key, button.label,
                    static_cast<unsigned long>(button.code), REPEATS);
    } else {
      Serial.println("Send failed; check radio setup and parameters");
    }
    break;
  }
}
