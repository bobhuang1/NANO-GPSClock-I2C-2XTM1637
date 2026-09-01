# NANO-GPSClock-I2C-2XTM1637

A GPS-synced clock for cars, built on an Arduino Nano.

Features:
1. Two 7-segment display modules: one shows hour:minute, the other cycles
   between month/day and cabin temperature.
2. A DS3231 real-time clock module keeps time as a backup, so the clock
   works immediately on power-up and continues even when GPS signal is
   unavailable (common in tunnels/parking garages).
3. The RTC is automatically corrected from GPS whenever a fix is available.
4. A cheap 10K NTC thermistor (the kind commonly used in 3D printers) reads
   the vehicle's internal temperature.

## Hardware

See [Wiring.txt](Wiring.txt) for the full pin mapping (GPS module, two
TM1637 displays, DS3231 RTC, thermistor).

## Dependencies

- [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
- [TM1637TinyDisplay](https://github.com/jasonacox/TM1637TinyDisplay)
- [RTClib](https://github.com/adafruit/RTClib) (DS3231 support)
- [Time](https://github.com/PaulStoffregen/Time) (`TimeLib.h`)
- `StringHelpers.h`/`.cpp` ([source](https://github.com/bobhuang1/ESP8266-Functions-Common/tree/master/StringHelpers)),
  vendored directly into this repo - re-copy from there if it's updated.
