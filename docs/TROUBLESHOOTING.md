# Probleemoplossing

## `UnknownBoard: Unknown board ID dongles3`

Het project moet de map `boards/dongles3.json` bevatten. Bouw vanuit de repositoryroot en controleer `pio project config`.

## `lv_conf.h: No such file or directory`

Controleer of in `platformio.ini` de vlag `-DLV_CONF_SKIP` staat. Deze repo configureert LVGL via buildflags.

## `esp_lcd_st7735.h: No such file or directory`

Voer `bash scripts/setup_lilygo_driver.sh` uit. Het script haalt de officiële driver uit LILYGO's repository.

## `NimBLEAdvertisedDevice` pointer-/operatorfouten

NimBLE-Arduino 1.4.3 geeft `getDevice(i)` als object terug. Deze broncode gebruikt `NimBLEAdvertisedDevice dev` en `dev.getRSSI()`, zonder `if(!dev)`.

## Scherm zwart of tekst beschadigd

Gebruik de gewone T-Dongle S3, **niet** Plus. Het scherm gebruikt de ST7735-driver en de bestaande DMA-buffer/flush-completion-fix. Controleer of de achtergrondverlichting GPIO38 active-low is en houd de 180°-oriëntatie aan. Als een fabrieksvoorbeeld wel werkt, vergelijk de displayinitialisatie.

## Geen seriële poort

Probeer `pio device list` en selecteer de poort met `pio run -t upload --upload-port /dev/ttyACM0` (pas aan). Voor downloadmodus: houd BOOT ingedrukt terwijl je aansluit en ontkoppel daarna voor normale start.

## SD niet herkend

Gebruik FAT32 en verifieer voeding en correcte kaartplaatsing. Schakel de voeding niet uit tijdens actieve PCAP-schrijfacties.
