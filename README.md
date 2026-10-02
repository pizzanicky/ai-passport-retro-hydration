<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Terminal Hydration Badge

A FoloToy AI Passport community application with an original retro green terminal interface and artwork. The identity page shows `USER-01` and an original retro cartoon character. The water page provides a fixed daily reminder and a master alert switch; the medicine page reserves a future entry and does not yet generate alerts.

Use UP/DOWN on the home page to choose a page and OK to open it. Hold OK to return. On the water page, OK toggles the master alert switch when no alert is pending; when an alert is pending, OK confirms drinking and UP turns all alerts off immediately. The switch is saved on the device and defaults to ON when no saved switch setting exists. Previous interval and workday settings are ignored. Alerts run every day from 09:00 to 21:00 local time at a fixed 30-minute interval, starting when the active window begins or when alerts are turned on. After confirmation, the next 30 minutes start from that action. Turning alerts off clears a pending alert; turning them back on begins a fresh interval. The firmware currently fixes the local timezone to Asia/Shanghai (UTC+8). After 30 seconds without input, the backlight switches off even when an alert is pending. The alert remains pending, and any key wakes the screen to show it.

For first-time clock setup, open SYSTEM and press OK to start Bluetooth provisioning. On a phone, use the exact mini-program name in the [Chinese setup instructions](README.zh_CN.md), select `BLUFI_Hydration`, and provide a 2.4 GHz Wi-Fi network. Wait for time sync. Hold OK to close provisioning. DOWN on SYSTEM clears stored Wi-Fi credentials. The reminder waits for a valid synchronized clock; Wi-Fi turns off after sync to reduce battery use.

The factory AI Passport profile Mini Program expects the factory firmware's identity service. Its unsupported-function message does not indicate a failure of this application's BLUFI provisioning service. Use the separate provisioning Mini Program named in the Chinese instructions. A displayed time is trusted only after this boot completes SNTP synchronization.

The board's passive NTAG213 tag is electrically independent of the firmware. Continue using the existing work card until both access control and attendance readers accept the tag in real tests.

See the [community release draft](docs/assets/community-release-draft.md) for prepared metadata and remaining device checks.
