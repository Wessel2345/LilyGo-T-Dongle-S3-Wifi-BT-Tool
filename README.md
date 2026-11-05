# WesPen USB

**Open-source ESP32-S3 WiFi/BLE analyzer for the LILYGO T-Dongle S3.**

Een compacte, passieve WiFi-/Bluetooth-analyzer op de LILYGO **T-Dongle S3** (gewone uitvoering, geen Plus), met 160×80 ST7735-display, BOOT-navigatie, microSD-opnames en RGB-ledindicatie.

> **Projectstatus:** v3-broncode samengesteld uit de laatst gedeelde werkende firmware en de Channel Traffic/Advice-patches. Deze GitHub-bundel is **niet opnieuw op echte hardware gecompileerd of gevalideerd**. Bouw en test eerst voordat je een release markeert.

## Functies

| Functie | Beschrijving |
|---|---|
| WiFi scanner | Automatisch scrollende SSID-lijst met signaalsterkte |
| Bluetooth LE scanner | Automatisch scrollende gevonden BLE-apparaten |
| Channel Analyzer | 13 staafjes, relatieve ontvangen framebytes per 2,4GHz-kanaal |
| Kanaaladvies | `Quiet`, `Busy`, en `Best 1/6/11` op basis van recente samples |
| Packet monitor | Passieve 802.11-frameobservatie |
| EAPOL monitor | Kies eerst WiFi-AP, luister op diens kanaal en tel EAPOL/deauth/disassoc |
| Deauth detector | Passieve detectie, rode knipperende LED bij detectie |
| microSD | Optionele PCAP-opnames met beperkte frame-selectie |
| Bedieningsinterface | 180°-rotatie, leesbare LVGL-tekst, BOOT-knop, dynamische APA102-led |

**Niet inbegrepen:** USB Copy, USB HID, auto-start op Windows, deauth-uitzending, wachtwoordkraken. Er worden **geen** actieve deauthframes verstuurd.

## Benodigdheden

- LILYGO **T-Dongle S3** (ESP32-S3, 16 MB flash, geen PSRAM)
- Kali Linux/Linux, Python 3 en PlatformIO
- `git` en internet voor het ophalen van upstream displaydriver en PlatformIO-libraries
- Optioneel FAT32 microSD-kaart

## Installatie (Kali/Linux)

```bash
git clone https://github.com/JE-GEBRUIKERSNAAM/WesPen-USB.git
cd WesPen-USB
python3 -m venv ~/.venvs/platformio
source ~/.venvs/platformio/bin/activate
python -m pip install platformio
bash scripts/setup_lilygo_driver.sh
pio run -e T-Dongle-S3
```

Vervang `JE-GEBRUIKERSNAAM` door je GitHub-gebruikersnaam na publicatie. Als je al een PlatformIO-omgeving hebt, kun je de venv-stappen overslaan.

**Pas wanneer de build `SUCCESS` geeft:**

```bash
pio device list
pio run -e T-Dongle-S3 -t upload
pio device monitor -b 115200
```

De upstream ST7735-driver wordt opgehaald door `scripts/setup_lilygo_driver.sh` en is bewust niet in deze repo gecommit. De lokale `boards/dongles3.json` is afkomstig van de officiële LILYGO-projectconfiguratie. LVGL en NimBLE-versies zijn vastgezet in `platformio.ini`.

## Gebruik

Zie [docs/CONTROLS.md](docs/CONTROLS.md) voor bediening en uitleg van de kanaalgrafiek en [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) voor bekende bouwproblemen.

## Belangrijke beperkingen

- De verkeersgrafiek is een **relatieve vergelijking van ontvangbare WiFi-frames** (ongeveer 300ms per kanaal), **geen exact kanaalbezettingspercentage** en detecteert geen alle interferentie.
- Alleen 2,4 GHz WiFi; de ESP32-S3 scant geen 5 GHz netwerken.
- EAPOL-telling garandeert geen complete handshake. De capture kan onvolledige, ontbrekende of afgekorte frames bevatten.
- PCAP-opnamen kunnen MAC-adressen en netwerkmetadata bevatten: publiceer deze bestanden niet zonder controle.
- Gebruik monitorfuncties alleen voor eigen of expliciet toegestane netwerken.

## Bouwcontrole op GitHub

GitHub Actions bouwt de firmware bij push/pull request. De workflow haalt de officiële driver op en voert `pio run -e T-Dongle-S3` uit. Een groen resultaat is een compilatiecontrole, **geen** hardwaretest.

## Herkomst en licenties

Deze projectcode en documentatie zijn MIT gelicentieerd (zie [LICENSE](LICENSE)). De ST7735-driver en `dongles3`-boardconfiguratie zijn afgeleid van [Xinyuan-LILYGO/T-Dongle-S3](https://github.com/Xinyuan-LILYGO/T-Dongle-S3) (upstream MIT). De benodigde upstream driver wordt tijdens de setup opgehaald; de rechten en copyrightvermeldingen van de upstream auteurs blijven van toepassing. Bibliotheken LVGL en NimBLE hebben hun eigen licenties.
