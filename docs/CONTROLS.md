# Controls and Navigation

The standard **LILYGO T-Dongle S3** features a BOOT button and a 160×80 TFT display.

- **Main Menu:** Short press = next option; long press = select.
- **WiFi and BLE Scanners:** Short press = scroll through detected networks or devices; long press = return to the main menu.
- **Channel Analyzer:** Short press = start a new measurement; long press = return to the main menu.
- **EAPOL AP Selection:** Short press = next access point; long press = select the access point (or go back when `Back` is selected).
- **EAPOL / Packet / Deauth Monitor:** Short press = switch channels; long press = return to the main menu. Counters automatically reset when leaving a monitoring mode.
- **microSD:** Stores optional PCAP files containing passively captured WiFi frames.

The display is configured with a **180° rotation** for improved readability.

The RGB LED displays different colors depending on the active mode and briefly flashes red when deauthentication or disassociation frames are detected during monitoring.

## Channel Analyzer

Each 2.4 GHz channel is sampled for approximately **300 milliseconds**.

The graph compares the **amount of received WiFi frame data (in bytes)** across channels 1–13.

- **`Quiet`:** The channel with the lowest observed traffic.
- **`Busy`:** The channel with the highest observed traffic.
- **`Best 1/6/11`:** Selects the least busy channel among the three commonly used non-overlapping 20 MHz channels.

These values are relative measurements intended to help identify which WiFi channels appear busier or quieter.

**Important limitations:**

- The graph does **not** represent exact channel utilization percentages.
- WesPen USB is **not a spectrum analyzer**.
- Measurements may be affected by reception conditions, overlapping channels, and interference.
- For monitoring a specific WiFi connection, keeping the EAPOL monitor on a single selected channel is generally more reliable than continuously switching channels.
