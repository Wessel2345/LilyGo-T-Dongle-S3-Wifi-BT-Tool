# Troubleshooting

This guide covers common issues when building, flashing, and running **WesPen USB** on the LILYGO T-Dongle S3.

## `UnknownBoard: Unknown board ID 'dongles3'`

Make sure the project contains the custom board definition:

`boards/dongles3.json`

Run PlatformIO from the repository's root directory and verify the configuration:

```bash
pio project config
```

If the board definition is missing, download it from the official [LILYGO T-Dongle S3 repository](https://github.com/Xinyuan-LILYGO/T-Dongle-S3).

## `lv_conf.h: No such file or directory`

Check that `platformio.ini` contains the following build flag:

```ini
build_flags =
    -DLV_CONF_SKIP
    -DLV_COLOR_DEPTH=16
```

This repository configures LVGL through PlatformIO build flags instead of using a separate `lv_conf.h` file.

## `esp_lcd_st7735.h: No such file or directory`

Run the display driver installation script:

```bash
bash scripts/setup_lilygo_driver.sh
```

The script downloads the official ST7735 display driver from the LILYGO repository.

After installation, rebuild the firmware:

```bash
pio run -e T-Dongle-S3
```

## `NimBLEAdvertisedDevice` Pointer or Operator Errors

NimBLE-Arduino 1.4.3 returns `getDevice(i)` as an object rather than a pointer.

The correct implementation is:

```cpp
NimBLEAdvertisedDevice dev = results.getDevice(i);
```

Access device properties using the dot operator:

```cpp
dev.getRSSI();
dev.getName();
dev.getAddress();
```

Do not use pointer-style access (`dev->`) or pointer validity checks such as:

```cpp
if (!dev)
```

These are incompatible with the object returned by this library version.

## Black Screen or Corrupted Text

Make sure you are using the standard **LILYGO T-Dongle S3**, **not the Plus version**.

The firmware uses:

- ST7735 display driver
- DMA-compatible display buffer
- Proper SPI transfer completion handling
- GPIO38 for the display backlight (active-low)
- 180° display orientation

If the screen remains black, check the display initialization and backlight configuration.

If text appears as missing pixels, incomplete characters, or corrupted lines, verify that the display buffer is not reused before the SPI transfer has completed.

**Recommended diagnostic procedure:**

1. Flash the official LILYGO factory display example.
2. Verify that the screen works correctly.
3. Compare the working display initialization with the WesPen USB implementation.
4. Rebuild and flash the firmware.

## Serial Port Not Found

List available serial devices:

```bash
pio device list
```

If PlatformIO does not automatically detect the correct port, specify it manually:

```bash
pio run -e T-Dongle-S3 -t upload --upload-port /dev/ttyACM0
```

Replace `/dev/ttyACM0` with the actual device path.

To enter the ESP32-S3 download mode:

1. Disconnect the device from USB.
2. Hold down the **BOOT** button.
3. Reconnect the device while holding BOOT.
4. Release the button and upload the firmware.
5. Disconnect and reconnect the device normally after flashing.

If the port is still unavailable, check USB permissions and confirm that your USB cable supports data transfer.

## microSD Card Not Detected

Make sure the microSD card is formatted as **FAT32** and correctly inserted.

Check the following:

- The card is fully inserted into the slot.
- The card uses a supported FAT32 filesystem.
- The USB connection provides stable power.
- The SD card initialization succeeds during startup.

Avoid disconnecting the device or removing the microSD card while PCAP files are being written.

Interrupting an active write operation may corrupt the capture file or filesystem.

## Additional Debugging

If compilation fails after making changes, perform a clean build:

```bash
pio run -e T-Dongle-S3 -t clean
pio run -e T-Dongle-S3
```

To view additional build details:

```bash
pio run -e T-Dongle-S3 -v
```

To inspect runtime messages:

```bash
pio device monitor -b 115200
```

If the problem persists, include the full compiler error or serial output when reporting an issue on GitHub.

---

**WesPen USB — LILYGO T-Dongle S3 WiFi & BLE Analyzer**

For installation instructions and available features, see the main [README](../README.md).
