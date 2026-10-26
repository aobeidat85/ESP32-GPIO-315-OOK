# ESP32 GPIO 315 OOK

An Arduino library for transmitting 315 MHz On-Off Keying (OOK) commands from
an ESP32 GPIO pin without a separate RF transmitter module. It includes an
example for a [VEVOR solar street light](https://www.vevor.com/solar-street-light-c_11583/vevor-8000w-solar-street-lights-outdoor-450000lm-commercial-parking-lot-lights-dusk-to-dawn-led-flood-security-lamp-with-motion-sensor-ip65-waterproof-outdoor-lighting-for-driveway-yard-3-pack-p_010321461696)
remote captured during this project.

> **Experimental transmitter:** The library uses a GPIO clock harmonic. Its
> carrier frequency and radiated power have not been calibrated, and the
> output can contain emissions at other frequencies. Results vary by board.

## Purpose

The tested VEVOR lights respond to the same fixed-code remote. The ON/OFF
button toggles each lamp's current state, so lamps that start in different
states can remain out of sync after a shared command. This library makes the
captured commands available to ESP32 applications for testing and automation.
The fixed code does not address individual lamps, and the ON/OFF command
cannot force a group of lamps into a known common state.

## Operation

OOK represents data by switching a radio carrier **on** and **off** in timed
pulses. The library gates a chip-specific GPIO clock and transmits bits in
rc-switch protocol 1 format, most significant bit first. Depending on the
chip, the clock's third or fifth harmonic is nominally near 315 MHz.

The CC1101 used during development served **only as a receiver to capture**
the handheld remote's codes. The ESP32 GPIO generates transmissions; this
library does not require a CC1101 or another RF transmitter module.

## Hardware status

| Target | Output | Nominal clock and harmonic | Verification |
| --- | ---: | --- | --- |
| Original ESP32 (D0WD-V3) | GPIO0 | ~63 MHz APLL clock; fifth harmonic ~315 MHz | ON/OFF toggled the lamp twice with the original firmware |
| ESP32-C3 | GPIO9 (BOOT) | ~105 MHz I2S clock; third harmonic ~315 MHz | ON/OFF toggled the lamp with this library's example |
| ESP32-S3 | GPIO4 | ~63 MHz I2S clock; fifth harmonic ~315 MHz | Builds; this library path has not been tested on S3 hardware |

GPIO4 did not control the lamp on the tested C3 board; GPIO9 did. Keep the
C3 BOOT button released during transmission.

## Installation

### Arduino IDE

1. Download this repository and place the `ESP32-GPIO-315-OOK` folder in your
   Arduino `libraries` directory.
2. Restart Arduino IDE and open **File → Examples → ESP32 GPIO 315 OOK →
   VevorSolarLamp**.
3. Select the matching ESP32 board and upload the sketch.
4. Open Serial Monitor at **115200 baud** and send a key from `2` through `9`.

### PlatformIO

Open `examples/VevorSolarLamp` as a PlatformIO project and select its
`esp32`, `esp32c3`, or `esp32s3` environment. To build all targets:

```sh
pio run -d examples/VevorSolarLamp
```

The example references this library through `lib_extra_dirs = ../..`. It
includes `Gpio315Ook.h` and calls `radio.send(...)`; it does not duplicate
the transmitter implementation. The Espressif platform is pinned to
**6.12.0**, matching the confirmed builds and C3 lamp test.

## Library API

```cpp
#include <Gpio315Ook.h>

Gpio315Ook radio;

void setup() {
  Serial.begin(115200);
  if (!radio.begin()) Serial.println("Radio clock setup failed");
}

void loop() {
  if (Serial.available() && Serial.read() == '2') {
    radio.send(0x3AB602, 24, 350, 8); // Captured ON/OFF code; C3 repeats
  }
}
```

`begin()` configures the supported chip's clock and output pin and returns
`false` if initialization fails. `send(code, bits, pulseUs, repeats)` sends a
blocking rc-switch protocol 1 frame and returns `false` for invalid
parameters or an uninitialized transmitter. The VEVOR example uses eight
repeats on C3 and two on original ESP32 and S3. Its 350 µs base pulse is the
rc-switch protocol 1 default; the remote's exact pulse length was not
measured in this project.

## VEVOR lamp example

The [example sketch](examples/VevorSolarLamp/VevorSolarLamp.ino) contains the
following 24-bit codes captured from **one** remote for the VEVOR
RBTYNLD-002 lamp. These are not manufacturer-supplied universal codes; other
remotes may use different codes or protocols.

| Serial key | Remote button | Code |
| --- | --- | --- |
| `2` | ON/OFF | `0x3AB602` |
| `3` | Radar | `0x3AB608` |
| `4` | Auto | `0x3AB604` |
| `5` | 2H | `0x3AB601` |
| `6` | 3H | `0x3AB60C` |
| `7` | 4H | `0x3AB607` |
| `8` | + | `0x3AB603` |
| `9` | − | `0x3AB60A` |

The CC1101 captured these codes at a **315.00 MHz receiver setting**. This
setting was not a precision measurement of the remote's carrier. The
library-based C3 example has been confirmed to toggle the lamp with ON/OFF;
its other seven buttons have not been individually verified on the C3.

## Limitations

- GPIO harmonic output is not a calibrated or filtered RF transmitter.
- Range depends on the board, pin routing, receiver, and placement; no
  maximum range is claimed.
- ON/OFF is a toggle, not an absolute ON or OFF command.
- The library does not address individual lamps, receive commands, or confirm
  lamp state.

Use the library only with devices you own or are authorized to control.
Measure the RF output and follow applicable local radio rules before adding
an antenna or extending its range.

## License

MIT. See [LICENSE](LICENSE).
