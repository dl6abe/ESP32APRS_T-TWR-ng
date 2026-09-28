# Changelog

All notable changes to this fork are documented here, newest first. This
covers what changed for someone flashing/using the firmware — for the
detailed technical reasoning behind each fix (root causes, what was tried
and rejected, verification steps), see `FORK_NOTES.md`.

## stable-fork-v0.4 — since forking from upstream `V0.4`

### Breaking Changes 🚨

- The on-device OLED menu's firmware update (OTA over plain HTTP, no TLS,
  no signature check) has been **removed entirely** for security reasons.
  Use the web UI's file-upload update instead (System page → Firmware
  Update).
- The web UI's firmware-update endpoint now **requires authentication**.
  Any automated/unattended update script must supply credentials — it
  previously accepted an unauthenticated upload from anyone with network
  access to the device.
- Time zone configuration changed: the old fixed UTC-offset field (no DST)
  is superseded by a **POSIX TZ string** (e.g. `CET-1CEST,M3.5.0,M10.5.0/3`)
  for correct daylight-saving-time handling. Re-enter your time zone in the
  new format on the System page.

### New Features 💫

- Color APRS symbol icons on the web dashboard (previously
  black-and-white).
- All APRS symbol icons are now served locally/offline — no more broken
  images from dead external hotlinks.
- GPS HDOP shown in the dashboard's GPS Info panel.
- Real battery percentage from the PMU's fuel gauge (not a voltage guess),
  plus a new "Power Info" panel (voltage/percent/source) and "System Info"
  panel (uptime/RAM/PSRAM/SD storage) on the dashboard.
- Automatic APRS-IS status message and telemetry reporting every 10
  minutes.
- POSIX TZ string support for correct DST handling (see Breaking Changes).
- OLED display timeout, dim mode, and contrast are now configurable from
  the web UI and serial console (previously only reachable from the
  on-device menu, or not wired up at all).
- Configuration backup/restore as a downloadable file, from the System
  page.
- USB-serial recovery console (`set wifi_ssid`/`set wifi_pass`/`save`) to
  recover a device that's become unreachable over WiFi.
- Runtime-toggleable, per-category debug logging, with optional syslog
  forwarding, now including a Bluetooth category.
- The WiFi station list is now dynamic — add/remove up to 5 networks with
  "+"/delete buttons, instead of always showing 5 fixed slots.
- Dashboard "Last Heard" callsigns are now clickable, linking to
  aprs.fi.

### Other Changes ☀️

- Fixed a buffer overrun that could silently corrupt received APRS
  packets.
- Fixed a stored XSS vulnerability: callsigns/paths from received
  RF/APRS-IS traffic were not HTML-escaped before being shown on the
  dashboard.
- Fixed a WiFi-QR-code buffer overflow and a stack overflow in the radio
  module's command handling, plus several other memory-safety bugs found
  via stricter compiler warnings.
- Added WiFi SSID/password validation against real 802.11/WPA2 limits.
- Fixed a cross-core data race on the APRS-IS connection that could drop
  outgoing traffic.
- Fixed a cross-task display-bus corruption bug affecting the OLED.
- Fixed telemetry being posted under the wrong station identity (missing
  SSID suffix).
- Removed jQuery in favor of native `fetch()`, reducing flash usage by
  roughly 31 KB.
- Fixed the dashboard going blank after the jQuery removal (a script
  re-execution edge case).
- Fixed garbled console log output and stray blank lines in logs.
- Fixed the serial recovery console not receiving input on some setups.
- Updated the vendored Adafruit GFX graphics library from 1.5.6 to 1.12.6
  (years of upstream bug fixes).
- Removed stale promotional text pointing at the original upstream
  author's servers.
- Reworded several filter checkboxes for clarity (e.g. "Filter repeater"
  → "Digipeat these types") — behavior is unchanged, only the confusing
  label was fixed.
- Fixed the boot-screen version text running off-screen.
- Fixed broken image links (symbol icons, donate button).
- Switched the Bluetooth LE stack from Bluedroid to NimBLE-Arduino,
  reducing flash usage by roughly 365 KB (~18%) and RAM usage by ~18%
  with no change in Bluetooth functionality.
