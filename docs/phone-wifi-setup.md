# Phone Wi-Fi setup

Digital Projections Watch 0.1.4 adds on-demand Bluetooth Low Energy provisioning for 2.4 GHz Wi-Fi. The existing manual password entry remains available.

1. Install [ESP BLE Provisioning by Espressif](https://play.google.com/store/apps/details?id=com.espressif.provble) on Android.
2. Open **Settings > Bluetooth > Start phone setup** on the watch. There is also a **Set up with phone** shortcut in Wi-Fi settings.
3. In the phone app, choose the option for a device without a QR code, select the `PROV_DP_` device shown on the watch, and enter its setup code. If the app asks for a security version, choose **Security 1**.
4. Select a 2.4 GHz network and enter its password on the phone. Wait for **Wi-Fi connected** on the watch.
5. The watch saves the network and stops Bluetooth provisioning automatically. You can repeat setup later. Cancel or leave the page to stop setup and restore the prior network when setup has not succeeded.

The setup code is generated afresh for each session and displayed only on the watch. Credentials travel through ESP-IDF Security 1 provisioning (X25519 handshake with proof of possession and encrypted transport). The console does not print Wi-Fi passwords or the setup code. This feature configures Wi-Fi; it does not provide phone notifications or Bluetooth audio.

## Verification

- `python scripts/test_watch.py`: shared rules, motion, swipe and provisioning state tests.
- `python scripts/preview_watch.py`: rendered game and phone-setup state/action tests.
- `phone_setup` on the USB console: state and resource status, without credentials. `phone_setup start` and `phone_setup stop` exercise service lifecycle.
- Install `scripts/requirements-watch-ble.txt` in a local virtual environment and run `scripts/check_watch_ble.py` for over-the-air discovery and read-only protocol verification.
- Real Android discovery, code entry, network selection, successful connection, invalid-password retry, cancellation, repeat setup and saved-network reboot must be checked separately from host tests.

Upstream protocol and Android implementation: [Espressif provisioning library](https://github.com/espressif/esp-idf-provisioning-android).
