# Digital Projections Watch 0.1.10

The Date & time timezone control opens a bounded list inside the settings page. Japan is the first fully visible row. The list has large touch targets, a Back control, balanced side margins and bottom clearance for the rounded screen. Periodic clock updates do not reset the scroll position.

AI Chat previously reported a generic service-unavailable state. USB logs reproduced repeated `Failed to create AFE processing task` failures before service discovery. The AFE and microphone input workers now allocate their stacks in PSRAM and free them using the matching ESP-IDF task API. TLS record and handshake allocations also use PSRAM; the configured 16 KB inbound TLS buffer exceeded the observed free internal RAM. Certificate validation stays enabled. Local memory/audio errors show recovery guidance separately from connection errors.

Device identifiers, saved credentials, activation endpoint and original recovery backup are preserved. The failure observed before this patch does not establish a subscription requirement or account denial. A subsequent provider rejection must be assessed separately.

Repeatable checks:

- `python scripts/test_watch.py`: rules, calibration, gestures, provisioning, recordings, time and AI error presentation.
- `python scripts/preview_watch.py`: production LVGL views, timezone first/last row bounds, pointer selection, Back, refresh stability, repeated open/close.
- `python scripts/test_clock_usb.py --port COM12`: internet time, saved Japan/UTC UI selection and status-bar bounds.
- `python scripts/check_aichat_usb.py --port COM12 --connect`: explicitly connect to the configured AI provider; print only numeric state and memory data. Raw logs stay under ignored `.local`.

AI diagnostic states: 0 Wi-Fi required, 1 preparing, 2 activation required, 3 connecting, 4 ready, 5 listening, 6 processing, 7 speaking, 8 error. Reaching Ready proves transport setup; a spoken request and audible reply still need a physical watch test.

Release hardware verification is pending.
