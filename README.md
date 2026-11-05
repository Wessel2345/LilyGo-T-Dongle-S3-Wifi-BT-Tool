# WesPen USB — Built with ChatGPT | LILYGO T-Dongle S3 WiFi & BLE Tool

**An open-source ESP32-S3 WiFi and Bluetooth Low Energy (BLE) security analyzer for the LILYGO T-Dongle S3.**

WesPen USB is a compact, passive WiFi and Bluetooth analysis tool built for the standard **LILYGO T-Dongle S3** (not the Plus version). It features a 160×80 ST7735 color display, BOOT-button navigation, optional microSD packet captures, and dynamic RGB LED indicators. This repo shows the power of mini tools. FOR EDUCATIONAL PURPOSES ONLY, OR IN ENVIRONMENTS WHERE EXPLICIT PERMISSION HAS BEEN GRANTED.

This project was developed with the assistance of **OpenAI ChatGPT**, combining custom firmware, hardware integration, and WiFi security analysis capabilities into a portable USB-sized device.

> **Project Status:** Version 3 (v3) combines the latest working firmware with the Channel Traffic and Channel Advice updates. Some texts on the display are not optimised for the screen. Use this repo to make changes your self. 

## Features

| Feature                 | Description                                                                                                   |
| ----------------------- | ------------------------------------------------------------------------------------------------------------- |
| WiFi Scanner            | Automatically scrolling list of detected WiFi networks with signal strength                                   |
| Bluetooth LE Scanner    | Automatically scrolling list of nearby Bluetooth Low Energy devices                                           |
| Channel Analyzer        | 13-bar graph displaying relative received WiFi traffic across 2.4 GHz channels                                |
| Channel Recommendations | Displays `Quiet`, `Busy`, and `Best 1/6/11` based on recent channel measurements                              |
| Packet Monitor          | Passive monitoring of IEEE 802.11 frames                                                                      |
| EAPOL Monitor           | Select a WiFi access point, monitor its channel, and count EAPOL, deauthentication, and disassociation frames |
| Deauth Detector         | Passive deauthentication frame detection with a flashing red LED alert                                        |
| microSD Recording       | Optional PCAP captures containing selected WiFi frames                                                        |
| User Interface          | 180° display rotation, readable LVGL interface, BOOT-button controls, and dynamic APA102 RGB LED effects      |

**Not included:** Active deauthentication attacks, or password cracking.

WesPen USB operates as a passive monitoring tool and **does not transmit deauthentication frames**.

## Hardware Requirements

- **LILYGO T-Dongle S3** (ESP32-S3, 16 MB flash, no PSRAM)
- Kali Linux or another Linux distribution
- Python 3 and PlatformIO
- Git and an internet connection to download the upstream display driver and required libraries
- Optional FAT32-formatted microSD card for packet captures

## Installation (Kali Linux / Linux)

Clone the repository:

```bash
git clone https://github.com/Wessel2345/LilyGo-T-Dongle-S3-Wifi-BT-Tool.git
cd LilyGo-T-Dongle-S3-Wifi-BT-Tool
```

Create a Python virtual environment and install PlatformIO:

```bash
python3 -m venv ~/.venvs/platformio
source ~/.venvs/platformio/bin/activate
python -m pip install platformio
```

Install the official LILYGO display driver:

```bash
bash scripts/setup_lilygo_driver.sh
```

Build the firmware:

```bash
pio run -e T-Dongle-S3
```

If you already have PlatformIO installed, you can skip the virtual environment setup.

### Flashing the Firmware

**Only proceed when the build finishes successfully (`SUCCESS`).**

Connect your LILYGO T-Dongle S3 to your computer.

List available serial devices:

```bash
pio device list
```

Upload the firmware:

```bash
pio run -e T-Dongle-S3 -t upload
```

Open the serial monitor:

```bash
pio device monitor -b 115200
```

After flashing, disconnect and reconnect the device if necessary.

The official ST7735 display driver is downloaded through `scripts/setup_lilygo_driver.sh` and is intentionally not committed to this repository.

The local `boards/dongles3.json` configuration originates from the official LILYGO project. LVGL and NimBLE library versions are pinned in `platformio.ini`.

## Usage

WesPen USB uses its built-in BOOT button for navigation.

- **Short press:** Navigate through menu options, devices, or channels.
- **Long press:** Select an option or return to the previous menu.

The interface is optimized for the compact 160×80 display.

For detailed instructions, see:

- [Controls and Navigation](docs/CONTROLS.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)

## WiFi Channel Analyzer

The Channel Analyzer provides a visual overview of received WiFi traffic across the 2.4 GHz spectrum.

It measures relative received frame data for approximately 300 milliseconds per channel.

The display includes:

- **Quiet:** The channel with the lowest observed traffic.
- **Busy:** The channel with the highest observed traffic.
- **Best 1/6/11:** The least busy channel among the commonly recommended non-overlapping 20 MHz channels.

Colored bars make it easier to compare channel activity at a glance.

**Important:** These measurements represent relative observed traffic, not exact channel utilization percentages. The analyzer cannot detect every source of interference, and traffic on overlapping channels may affect the results.

## EAPOL and Deauthentication Monitoring

The EAPOL Monitor allows users to select a detected WiFi access point and passively observe traffic on its operating channel.

It can count:

- EAPOL authentication frames
- Deauthentication frames
- Disassociation frames

When deauthentication or disassociation activity is detected, the RGB LED flashes red to provide a visual alert.

The monitor stays on the selected channel to reduce the chance of missing relevant frames.

**Note:** Detecting EAPOL frames does not guarantee that a complete WPA2 handshake has been captured.

## Important Limitations

- **2.4 GHz only:** The ESP32-S3 does not support scanning 5 GHz WiFi networks.
- **Relative traffic measurements:** Channel activity is estimated using received WiFi frames, not professional spectrum analysis.
- **Limited packet capture:** PCAP files may contain incomplete, missing, or truncated frames.
- **Handshake limitations:** EAPOL detection does not guarantee a complete or usable WPA2 handshake.
- **Privacy considerations:** PCAP recordings may contain MAC addresses and other network metadata. Review capture files before sharing them publicly.
- **Authorized testing only:** Use monitoring and analysis features only on networks you own or have explicit permission to assess.

## GitHub Actions — Automated Builds

This repository includes a GitHub Actions workflow that automatically builds the firmware when changes are pushed or pull requests are opened.

The workflow:

1. Sets up the PlatformIO environment.
2. Downloads the official LILYGO display driver.
3. Builds the firmware using `pio run -e T-Dongle-S3`.
4. Reports whether compilation succeeds.

A successful build confirms that the source code compiles, but **does not guarantee successful operation on physical hardware**.

## Project Background

WesPen USB was created as an experimental WiFi and Bluetooth security analysis project using the LILYGO T-Dongle S3.

The goal is to provide a lightweight, portable tool for learning about wireless networks, monitoring authentication activity, examining channel traffic, and experimenting with ESP32-S3 development.

The firmware and documentation were developed with assistance from **ChatGPT by OpenAI**, including troubleshooting, display integration, PlatformIO configuration, and feature development.

## Credits and Licensing

The original WesPen USB project code and documentation are licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

The ST7735 display driver and `dongles3` board configuration are derived from the official [Xinyuan-LILYGO/T-Dongle-S3](https://github.com/Xinyuan-LILYGO/T-Dongle-S3) project.

The upstream display driver is downloaded during setup, and the original authors' copyright notices and licensing terms remain applicable.

Third-party dependencies, including **LVGL** and **NimBLE-Arduino**, are subject to their respective licenses.

---

**WesPen USB — A portable WiFi & BLE analysis tool built with ChatGPT.**

Developed for the LILYGO T-Dongle S3.
