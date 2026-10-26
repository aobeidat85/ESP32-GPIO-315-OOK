# ESP32 GPIO 315 OOK

An Arduino library for sending **rc-switch protocol 1 OOK** frames using a
clock harmonic from an ESP32 GPIO pin. It needs no separate RF transmitter
module. The output is experimental: it is not a calibrated or filtered radio
transmitter, and the GPIO clock produces other harmonics.

## Why this project exists

The user's VEVOR lights all respond to the same handheld remote. Their
ON/OFF button appears to toggle each light's *current* state: if one light is
off while another is on, the same press can turn the first on and the second
off. This project records the remote's eight captured commands and lets an
ESP32 send them from software, so the commands can be tested and integrated
into a PC or automation project without carrying the remote.

The shared fixed code does **not** identify an individual lamp. Sending ON/OFF
cannot guarantee that a group of lamps ends in the same state, and this
library does not claim individual lamp control. A separate pairing or
addressing capability would be needed for that.

## Supported chip paths

| Chip | GPIO | Clock path | Lamp test |
| --- | ---: | --- | --- |
| Original ESP32 (D0WD-V3) | 0 | ~63 MHz from APLL; fifth harmonic ~315 MHz | ON/OFF toggled twice in the original project |
| ESP32-C3 | 9 (BOOT) | ~105 MHz from I2S 240 MHz source; third harmonic ~315 MHz | ON/OFF toggled twice in the C3 project |
| ESP32-S3 | 4 | ~63 MHz from I2S 160 MHz source; fifth harmonic ~315 MHz | Builds; this library path has not been hardware tested |

The original working eight-button firmware is preserved in the separate
`esp32-315-ook` project, and the working C3 firmware is preserved in
`esp32-c3-315-ook`. This library packages those transmission methods; it does
not replace the saved projects. Chip and board layout affect coupling and
range. GPIO4 did not operate the lamp on the tested C3, while GPIO9 did.

## Install and use

For Arduino IDE, copy this folder into your Arduino `libraries` directory and
open **File → Examples → ESP32 GPIO 315 OOK → VevorSolarLamp**. Select the
matching ESP32 board, upload, open Serial Monitor at 115200 baud, and send a
single character from `2` to `9`.

For PlatformIO, open `examples/VevorSolarLamp` as a project, select `esp32`,
`esp32c3`, or `esp32s3` from its `platformio.ini`, and build. It references
this library through `lib_extra_dirs = ../..`; the example itself uses only
`#include <Gpio315Ook.h>` and `radio.send(...)` for transmission.

Basic library use:

```cpp
#include <Gpio315Ook.h>

Gpio315Ook radio;

void setup() {
  radio.begin();
}

void loop() {
  radio.send(0x3AB602, 24, 350, 8); // ON/OFF from one captured remote
  delay(5000);
}
```

`begin()` returns `false` for unsupported chips or a failed I2S clock update.
`send(code, bits, pulseUs, repeats)` returns `false` if setup failed or a
parameter is outside its supported range. It is blocking. The example sends
eight repeats on C3 and two on original ESP32 and S3. The 350 microsecond
pulse base is the rc-switch protocol 1 default; the exact remote pulse length
was not measured in this project.

## VEVOR lamp example

The [VEVOR RBTYNLD-002 solar street light](https://www.vevor.com/solar-street-light-c_11583/vevor-8000w-solar-street-lights-outdoor-450000lm-commercial-parking-lot-lights-dusk-to-dawn-led-flood-security-lamp-with-motion-sensor-ip65-waterproof-outdoor-lighting-for-driveway-yard-3-pack-p_010321461696)
is listed with an RF remote, motion sensing, timer settings, and adjustable
brightness. The codes below were **captured from one user's remote**, then
replayed through a CC1101 at a 315.00 MHz receiver setting. They are not
manufacturer-supplied universal codes; another remote may have different
codes or even a different protocol.

| Key | Remote button | Code |
| --- | --- | --- |
| `2` | ON/OFF | `0x3AB602` |
| `3` | Radar | `0x3AB608` |
| `4` | Auto | `0x3AB604` |
| `5` | 2H | `0x3AB601` |
| `6` | 3H | `0x3AB60C` |
| `7` | 4H | `0x3AB607` |
| `8` | + | `0x3AB603` |
| `9` | − | `0x3AB60A` |

All eight codes worked in the earlier CC1101 replay tests. The ESP32-C3 GPIO9
path has been confirmed for ON/OFF, including one lamp toggle from this
library's example after flashing it to the C3. The other seven C3 buttons
have not yet been individually checked. The `315.00 MHz` CC1101 setting was successful but
was not a precision measurement of the original remote's carrier.

Use only with devices you own or are authorized to control. A GPIO harmonic
can radiate unwanted frequencies; measure the output and follow local radio
rules before extending its range or connecting an antenna.

## Build check

`pio run -d examples/VevorSolarLamp` compiles all three targets. Hardware
behavior still depends on the board, pin routing, and the receiver.

## License

MIT; see [LICENSE](LICENSE).
