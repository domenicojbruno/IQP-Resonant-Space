# VL53L0X ToF Sensor — Arduino Uno R3 Range Test Findings

**Date:** 2026-09-21
**Board:** GY-VL53L0XV2 breakout (CJMCU/DIYmore module)
**Host:** Arduino Uno R3

## Wiring Used

| VL53L0X Pin | Arduino Uno Pin | Notes |
|---|---|---|
| VCC | 5V | Board has onboard 2.8V regulator + level shifting, so 5V is fine |
| GND | GND | |
| SCL | SCL (A5) | |
| SDA | SDA (A4) | |
| GPIO1 | D2 | Interrupt pin — wired but unused in polling test |
| XSHUT | D7 | **Moved from originally planned D0** |

### Issue found: XSHUT on D0
Original plan was XSHUT → D0. D0 is the Uno's hardware RX pin, shared with the
USB-to-serial chip. Driving it as a digital output conflicts with sketch
uploads and with the Serial Monitor. Resolved by moving XSHUT to D7.
**This will not be an issue on the ESP32-WROOM target** (no equivalent RX/XSHUT
conflict), so XSHUT can go on any free GPIO there.

## Firmware

- Library: `Adafruit_VL53L0X` (Arduino Library Manager), depends on
  `Adafruit_BusIO`.
- Sensor configured with `VL53L0X_SENSE_LONG_RANGE` (longer timing budget +
  lower signal-rate limit) to maximize detection distance for range testing,
  at the cost of some speed/accuracy vs. default mode.
- Polls `rangingTest()` every 100 ms and prints distance (mm and inches) to
  Serial Monitor at 115200 baud; prints "Out of range" when `RangeStatus == 4`.
- Sketch delivered to user: `vl53l0x_distance_test.ino`.

## Result

Sensor confirmed working for the intended application (hallway threshold
detection). Long-range mode expected to give ~2 m typical indoor range,
less in bright/direct sunlight (IR-based ToF sensors are sensitive to
ambient IR).

## Open Items for ESP32-WROOM Integration

- Port sketch from Arduino Uno to ESP32-WROOM (I2C pins, XSHUT pin choice
  unconstrained by RX conflict).
- Multiple ToF sensors on one I2C bus will each need a unique I2C address,
  set at boot by sequentially bringing sensors out of shutdown via their
  individual XSHUT pins and calling `setAddress()` before the next sensor
  is enabled.
- Threshold-crossing logic to trigger BTS7960 H-Bridge-driven linear motors
  on the wall, based on distance readings.
