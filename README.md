# Digital Projections Watch

An extensible watch system for the ESP32-S3 with a 2.06-inch 410 x 502 AMOLED display.

Version 0.1.0 is in development. The integrated firmware has not yet passed hardware acceptance testing.

## Applications

The system includes 2048, GravitySphere, calculator, drawing, gallery, music, recording, video, settings, and AI chat. Factory applications are adapted from the Waveshare 1.75-inch Brookesia source to the 2.06-inch hardware. Original licensing and attribution are retained.

2048 uses the accelerometer for directional tilt and gyroscope readings to reject abrupt rotation. Return the watch to neutral after each move. BOOT toggles pause, a double press calibrates, and a two-second hold starts a new game. GravitySphere uses the same calibration; holding BOOT centres its ball.

Calibration is shared across motion apps and saved to NVS. Keep the watch flat and still during calibration. A successful stationary measurement corrects accelerometer and gyroscope bias; it does not perform six-face scale calibration. Calibration reports loading, success, unavailable, or error and has an eight-second deadline.

## Development

- `python scripts/test_watch.py` runs host tests against the production game and calibration code.
- `scripts\watch_build.cmd build` builds the complete watch system with ESP-IDF 5.5.4. Set `IDF_PATH` if installed elsewhere.
- `python scripts/flash_watch.py --port COM12 --backup PATH` validates a committed build and recovery snapshot; add `--write` to install. Use `--migrate-original-nvs` only for the first migration from the factory image.
- Firmware source: `FirmWare/watch`.
- Upstream examples remain in `examples` as hardware references.

Private full-flash backups stay outside Git. Flashing requires verified backup, passing tests, a successful firmware build, and a committed revision. A successful build alone does not establish that display, touch, motion, audio, or SD work on the device.

## Source provenance

Forked from [Waveshare 2.06](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.06). Application suite imported from [Waveshare 1.75](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75/tree/main/firmware/brookesia). See LICENSE and source headers for upstream terms.
