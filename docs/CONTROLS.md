# Bediening

De gewone **T-Dongle S3** heeft een BOOT-knop en een 160×80 TFT-scherm.

- In het hoofdmenu: **kort** = volgende optie; **lang** = openen.
- WiFi- en BLE-scanner: **kort** = door apparaten bladeren; **lang** = terug.
- Channel analyzer: **kort** = nieuwe meting; **lang** = terug.
- EAPOL AP-keuze: **kort** = volgende AP; **lang** = kies AP (of terug bij `Back`).
- EAPOL/packet/deauth-monitor: **kort** = kanaalwissel; **lang** = terug. Bij verlaten resetten tellers.
- MicroSD: bevat optionele PCAP-bestanden van passief ontvangen frames.

Het scherm is ingesteld op 180° rotatie. De RGB-led toont moduskleuren en knippert kort rood bij ontvangen deauth/disassociationframes in de monitoringmodi.

## Channel analyzer

Elke 2,4GHz-kanaalsample duurt ongeveer 300 ms. De grafiek vergelijkt de **ontvangen WiFi-framebytes** op kanalen 1 t/m 13. `Quiet` en `Busy` zijn relatief. `Best 1/6/11` kiest de rustigste van de klassieke niet-overlappende 20MHz-kanalen.

Het is **geen** meting van werkelijk kanaalgebruik in procenten, geen spectrumanalyzer, en de waarden kunnen beïnvloed worden door de ontvangstsituatie en overlapping. Een EAPOL-monitor op één gekozen kanaal is geschikter dan kanaalhoppen voor een verbindingstest.
