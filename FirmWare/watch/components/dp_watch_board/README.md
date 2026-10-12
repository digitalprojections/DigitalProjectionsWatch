# Digital Projections Watch board support

This local BSP targets the Waveshare ESP32-S3 Touch AMOLED 2.06: 410 x 502 CO5300, FT3168 touch, and the 2.06 board audio and SD wiring. It adapts the modern LVGL adapter and audio lifecycle from the upstream 1.75 BSP while retaining upstream license notices.

The QMI8658 is owned by the shared `dp_motion` service. Applications must consume that service rather than create another IMU driver instance.
