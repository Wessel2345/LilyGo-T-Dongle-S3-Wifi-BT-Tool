# Controls and Navigation

The standard **LILYGO T-Dongle S3** features a BOOT button and a 160×80 TFT display.

## Button Controls

- **Main Menu:** Short press = next option; long press = select.
- **WiFi and BLE Scanners:** Short press = browse detected networks or devices; long press = return.
- **Channel Analyzer:** Short press = start a new measurement; long press = return.
- **EAPOL AP Selection:** Short press = next access point; long press = select the AP (or return when `Back` is selected).
- **EAPOL / Deauth Monitor:** Short press = switch channels; long press = return.
- **Packet Monitor:** Short press = switch channels; long press = return.

Monitoring counters reset when leaving a monitoring mode.

## Packet Monitor 2.0

The Packet Monitor continuously displays a live WiFi traffic graph.

The display includes:

- **Packets/sec:** Number of received WiFi packets per second.
- **KB/s:** Amount of received frame data per second.
- **Live Graph:** A continuously updating 16-bar traffic graph.
- **Channel:** Currently monitored WiFi channel.

The live graph remains visible at all times. There is no automatic switching to frame statistics.

### RGB LED

- **Purple pulse:** Packet Monitor is active.
- **Red flashing:** Deauthentication or disassociation frames detected.
- **Purple resumes:** After the detection alert ends.

The Packet Monitor operates passively and does not transmit deauthentication frames.

## Channel Analyzer

Each 2.4 GHz channel is sampled for approximately 300 milliseconds.

The graph compares received WiFi frame bytes across channels 1–13.

- **Quiet:** Channel with the lowest observed traffic.
- **Busy:** Channel with the highest observed traffic.
- **Best 1/6/11:** Least busy of the commonly used non-overlapping 20 MHz channels.

These values are relative estimates, not exact channel utilization percentages.

The ESP32-S3 is not a spectrum analyzer. Measurements may be affected by signal conditions, overlapping channels, and interference.

## Display and Storage

- **Display:** 160×80 ST7735 TFT with 180° rotation.
- **Navigation:** BOOT button.
- **RGB LED:** Dynamic colors depending on the active mode.
- **microSD:** Optional PCAP recordings of passively received WiFi frames.

For monitoring a specific WiFi connection, staying on a selected channel is generally more reliable than channel hopping.
