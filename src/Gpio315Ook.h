#pragma once

#include <Arduino.h>

// Experimental, low-power OOK from a GPIO clock harmonic. This is not a
// calibrated 315 MHz transmitter; the output also contains other harmonics.
class Gpio315Ook {
public:
  // ESP32: GPIO0. ESP32-C3: GPIO9. ESP32-S3: GPIO4.
  // Returns false on unsupported targets or if clock setup fails.
  bool begin();

  // rc-switch protocol 1, MSB first. Defaults match a 24-bit remote.
  // Blocking; call from loop(), not an ISR.
  bool send(uint32_t code, uint8_t bits = 24, uint16_t pulseUs = 350,
            uint8_t repeats = 2);

  uint8_t pin() const;
  bool ready() const;

private:
  void carrier(bool on);
  void pulse(uint32_t highUs, uint32_t lowUs);
  bool setClockEnabled(bool enabled);
  bool ready_ = false;
};
