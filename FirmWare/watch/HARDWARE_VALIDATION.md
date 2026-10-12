# Digital Projections Watch hardware acceptance

Hardware: ESP32-S3, 32 MB flash, 8 MB octal PSRAM, 410 x 502 CO5300 AMOLED, FT3168 touch, QMI8658 IMU.

Before installation, verify the private full-flash backup, pass host and LVGL tests, build the complete image and commit its source. Keep restore material outside Git.

Use USB console commands after boot:

1. `watch`: list all installed applications; confirm 2048 and GravitySphere appear in the launcher.
2. `motion`: inspect sensor state and units. A stationary watch should have acceleration magnitude close to 1 g.
3. `calibrate`: with the watch flat and still, wait for `motion` to show ready and a nonzero generation.
4. Open both motion apps using `watch open ID`, return using `watch home`, and verify the same calibration generation. Repeat launch/close cycles and monitor free internal memory and reset/panic logs.
5. Reboot, query `motion`, and verify that the calibration generation and corrected stationary readings persist.
6. `sd`: mount and report the card capacity. `sd test`: verify a newly created 16 KiB temporary file by CRC; the test removes its own file.
7. Physically test tilt in four directions, return to neutral, BOOT pause/resume, double-press calibration, long-press reset, touch navigation, and display orientation.
8. Inspect audio, recording, playback, settings, and AI chat separately; successful compilation does not verify those device or network paths.

Host tests and render evidence are separate from physical button, touch, display and motion evidence. Record failures and remaining checks in the release report.
