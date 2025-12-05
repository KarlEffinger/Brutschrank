/* ============================================================================
 * LUFTBEFEUCHTER-STEUERUNG MIT LVGL GUI UND WEB-INTERFACE
 * ============================================================================
 * 
 * PROJEKTBESCHREIBUNG
 * -------------------
 * Automatische Luftfeuchte-Steuerung für Brutschränke, Terrarien oder 
 * Gewächshäuser mit DHT22 Sensor, Ultraschall-Vernebler, Wasserstands-
 * Überwachung und grafischer Benutzeroberfläche. Speichert 14 Tage 
 * Messdaten und bietet Verlaufsdarstellung sowie CSV-Export über 
 * Web-Interface.
 *
 * 
 * HAUPTFUNKTIONEN
 * ---------------
 * ✓ Automatische Luftfeuchte-Regelung mit Hysterese (1%)
 * ✓ Einstellbare Sprühstoß-Dauer (parametrierbar 1-60s)
 *      ACHTUNG:
 *      Da die Luftfeuchte durch die Sprühstöße nur erhöht, aber nicht ab-
 *      gesenkt werden kann, ist die Sprühstoßdauer vorsichtig anzupassen,
 *      damit durch einen einzelnen Sprühstoß die Feuchte im jeweiligen Raum
 *      nicht deutlich ÜBER den Zielwert gebracht wird.
 * ✓ Wasserstands-Überwachung mit visueller Warnung
 * ✓ 14 Tage Datenspeicherung (10.080 Messungen @ 2min Intervall)
 * ✓ Persistente Speicherung in LittleFS (überlebt Neustart)
 * ✓ Web-Interface mit Verlaufs-Chart und CSV-Download
 * ✓ LVGL GUI auf 1.69" TFT-Display mit Live-Anzeigen
 * ✓ Rotary Encoder für Ziel-Luftfeuchte (40-90%)
 * ✓ WiFi mit mDNS (erreichbar unter brutschrank.local)
 * ✓ Offline-Betrieb bei WiFi-Ausfall (volle Funktionalität)
 * ✓ IP-Adresse direkt auf Display sichtbar
 * 
 * SICHERHEITSFUNKTIONEN
 * ---------------------
 * ✓ Vernebler stoppt automatisch bei niedrigem Wasserstand
 * ✓ Vernebler stoppt bei DHT22-Sensor-Fehler
 * ✓ Automatische Erkennung von Verbindungsproblemen
 * ✓ Offline-Betrieb ohne Funktionseinschränkung
 * ✓ Overflow-Schutz für millis() nach 49 Tagen
 * ✓ Plausibilitätsprüfung beim Laden gespeicherter Daten
 * 
 * ============================================================================
 * HARDWARE-SETUP
 * ============================================================================
 * 
 * KOMPONENTEN
 * -----------
 * - ESP32 DevKit (z.B. ESP32-WROOM-32)
 * - TFT-Display 1.69" 280x240 mit ST7789v2 Treiber (SPI)
 * - DHT22 Sensor (Temperatur + Luftfeuchte)
 * - Rotary Encoder HW-040 mit Taster (mit Software Pull-Up)
 * - Relais-Modul (z.B. 5V, 1-Kanal)
 * - Ultraschall-Vernebler (über Relais geschaltet)
 * - Wasserstands-Sensor (2 Kontakte/Drähte im Wasserbehälter)
 * 
 * PIN-BELEGUNG ESP32
 * ------------------
 * DHT22 Sensor:
 *   DATA → GPIO 26
 *   VCC  → 3.3V
 *   GND  → GND
 *   Hinweis: 10kΩ Pull-Up zwischen DATA und VCC empfohlen
 * 
 * TFT-Display (ST7789v2):
 *   CS   → GPIO 21
 *   RST  → GPIO 18
 *   DC   → GPIO 19
 *   SCLK → GPIO 17
 *   MOSI → GPIO 5
 *   VCC  → 3.3V
 *   GND  → GND
 *   LED  → 3.3V (Hintergrundbeleuchtung)
 * 
 * Rotary Encoder:
 *   CLK  → GPIO 34
 *   DT   → GPIO 35
 *   SW   → GPIO 32 (Taster, mit INPUT_PULLUP)
 *   VCC  → GPIO 33 (optional, kann auch fest an 3.3V)
 *   GND  → GND
 * 
 * Relais (Vernebler):
 *   IN   → GPIO 27
 *   VCC  → 5V (oder 3.3V, je nach Modul)
 *   GND  → GND
 * 
 * Wasserstands-Sensor:
 *   Kontakt 1 → GPIO 25
 *   Kontakt 2 → GND
 *   Funktion: Wasser schließt Kontakt zwischen GPIO 25 und GND
 *   Hinweis: INPUT_PULLUP aktiv, daher LOW = Wasser vorhanden
 * 
 * WASSERSTANDS-SENSOR AUFBAU
 * ---------------------------
 * Variante A - Einfache Kontakte (billig):
 * - 2 blanke Kupferdrähte oder Edelstahl-Schrauben
 * - Abstand: ca. 5-10mm
 * - Position: Ca. 1cm über Boden des Wasserbehälters
 * - Isolation: Nur Spitzen im Wasser, Rest isoliert
 * 
 * WICHTIG - Korrosionsschutz:
 * - Edelstahl oder verzinkte Kontakte verwenden
 * - KEIN Kupfer (oxidiert schnell)
 * - Regelmäßig reinigen (Kalkablagerungen)
 * 
 * 
 * ============================================================================
 * SOFTWARE-ARCHITEKTUR
 * ============================================================================
 * 
 * BIBLIOTHEKEN (erforderliche Versionen)
 * --------------------------------------
 * Externe Bibliotheken (über Arduino Library Manager):
 * - LVGL v8.3.11                  → GUI Framework         https://github.com/lvgl/lvgl
 * - TFT_eSPI v2.5.43              → Display-Treiber       https://github.com/Bodmer/TFT_eSPI
 * - DHT sensor library v1.4.4     → DHT22 Sensor          https://github.com/adafruit/DHT-sensor-library
 * - AiEsp32RotaryEncoder v1.7     → Drehgeber mit Taster  https://github.com/igorantolic/ai-esp32-rotary-encoder
 * 
 * ESP32 Core Bibliotheken (integriert):
 * - LittleFS        → Dateisystem für Datenspeicherung
 * - WiFi            → WiFi-Verbindung
 * - ESPmDNS         → Hostname-Auflösung (brutschrank.local)
 * - WebServer       → HTTP Server
 * - Preferences     → NVS Speicher für Konfiguration
 * 
 * HINWEIS: TFT_eSPI User_Setup.h muss manuell angepasst werden!
 * Pin-Belegung siehe oben unter "PIN-BELEGUNG".
 * 
 * SPEICHERKONZEPT
 * ---------------
 * 
 * 1. NVS (Non-Volatile Storage) - Konfiguration:
 *    Gespeicherte Daten:
 *    - WiFi SSID
 *    - WiFi Passwort
 *    - Ziel-Luftfeuchte (40-90%)
 *    - Sprühstoß-Dauer (1-60s)
 *    Größe: ~500 Bytes
 *    Schreibzyklen: ~100.000 (Flash-schonend durch Änderungserkennung)
 *    Speicherort: Partition "nvs" (20 KB)
 * 
 * 2. LittleFS (Filesystem) - Messdaten:
 *    Datei: /ringdata.bin
 *    Inhalt:
 *    - Version-Byte (Kompatibilitätsprüfung)
 *    - Ring-Index und Full-Flag
 *    - 10.080 Messungen (komprimiert)
 *    Größe: 60.483 Bytes
 *    Auto-Save: Alle 30 Minuten
 *    Manuell: Über Web-Interface Button
 *    Speicherort: Partition "spiffs" (1,5 MB, ~4% genutzt)
 * 
 * 3. RAM (Heap) - Aktive Daten:
 *    Dynamisch allokiert in setup():
 *    - Ringspeicher-Array: 60.480 Bytes
 *    Statisch allokiert:
 *    - LVGL Display-Buffer: ~22 KB
 *    - WiFi/TCP Stack: ~40 KB
 *    - Strings, Variablen: ~10 KB
 *    Gesamt: ~132 KB von 327 KB genutzt (40%)
 *    Verfügbar: ~195 KB für dynamische Operationen
 * 
 * DATENSTRUKTUR RINGSPEICHER
 * --------------------------
 * struct MessungCompact {
 *   uint16_t timestamp;      // 2 Bytes: Minuten seit Start (0-65535 = 45 Tage)
 *   uint16_t temperatur;     // 2 Bytes: Temp × 100 (0.01°C Auflösung, 0-100°C)
 *   uint8_t istFeuchte;      // 1 Byte:  Feuchte × 2.55 (0.4% Auflösung, 0-100%)
 *   uint8_t sollFeuchte;     // 1 Byte:  Ziel-Feuchte (40-100%)
 * };
 * Gesamt: 6 Bytes pro Messung
 * 
 * Komprimierungs-Algorithmus:
 * - Temperatur:  temp × 100 → uint16_t (2345 = 23.45°C)
 * - Luftfeuchte: feuchte × 2.55 → uint8_t (153 = 60%)
 * 
 * Encode/Decode-Funktionen:
 * - encodeTemperatur(float) → uint16_t
 * - decodeTemperatur(uint16_t) → float
 * - encodeFeuchte(float) → uint8_t
 * - decodeFeuchte(uint8_t) → float
 * 
 * RINGSPEICHER-VERWALTUNG
 * -----------------------
 * Funktionsweise:
 * 1. Array mit 10.080 Einträgen (14 Tage × 720 Messungen/Tag)
 * 2. Neue Messung überschreibt älteste (zirkulär)
 * 3. Index zeigt auf nächste freie Position
 * 4. Full-Flag zeigt an, ob Array komplett gefüllt
 * 
 * Beispiel:
 * - Tag 1-14: Array füllt sich linear (Index 0-10079)
 * - Ab Tag 15: Index springt auf 0, überschreibt älteste Daten
 * - Ergebnis: Immer die letzten 14 Tage verfügbar
 * 
 * ============================================================================
 * REGELUNGS-ALGORITHMUS
 * ============================================================================
 * 
 * LUFTFEUCHTE-REGELUNG
 * --------------------
 * Mess- und Regelzyklus (alle 2 Minuten):
 * 
 * 1. DHT22 Messung
 *    - Temperatur (0-100°C, ±0.5°C Genauigkeit)
 *    - Luftfeuchte (0-100%, ±2-5% Genauigkeit)
 *    - Plausibilitätsprüfung (NaN-Erkennung)
 * 
 * 2. Wasserstands-Prüfung
 *    - LOW = Wasser vorhanden (Kontakt geschlossen)
 *    - HIGH = Kein Wasser (Kontakt offen)
 *    - Bei LOW: Warnung anzeigen, Vernebler sperren
 * 
 * 3. Regelungs-Entscheidung
 *    IF (Ist-Feuchte < Soll-Feuchte - Hysterese)
 *    AND (Sensor funktioniert)
 *    AND (Wasser vorhanden)
 *    THEN → Sprühstoß starten
 * 
 * 4. Sprühstoß-Ausführung
 *    - Relais AN (GPIO 27 → HIGH)
 *    - Timer läuft für eingestellte Dauer (z.B. 10s)
 *    - Nach Ablauf: Relais AUS (GPIO 27 → LOW)
 *    - Timestamp für "Letzter Spray" speichern
 * 
 * 5. Datenspeicherung
 *    - Messwerte in Ringspeicher schreiben
 *    - Alle 30 Minuten: Auto-Save nach LittleFS
 * 
 * PARAMETER & KONSTANTEN
 * ----------------------
 * Mess-Intervall:        120 Sekunden (2 Minuten) - FEST
 * Hysterese:             ±1% - FEST
 * Sprühstoß-Dauer:       Parametrierbar, Standard: 10s (1-60s)
 * Ziel-Luftfeuchte:      Parametrierbar, 40-90% (via Encoder/Web)
 * Auto-Save:             30 Minuten - FEST
 * Config-Save:           5 Minuten (nur bei Änderung) - FEST
 * 
 * HYSTERESE-LOGIK (Detailliert)
 * ------------------------------
 * Beispiel: Ziel-Luftfeuchte = 60%
 * 
 * Schwellwerte:
 * - Unter-Schwelle: 60% - 1% = 59%
 * - Über-Schwelle:  60% + 1% = 61%
 * 
 * Verhalten:
 * - Bei 58% → Sprühstoß startet
 * - Sprühstoß läuft 10s (unabhängig von Feuchte-Änderung)
 * - Pause bis nächste Messung (2 min)
 * - Bei 59% → Erneuter Sprühstoß
 * - Bei 60-61% → Kein Sprühstoß (Zielbereich)
 * - Bei 62% → Kein Sprühstoß (zu hoch, nur Anzeige invertiert)
 * 
 * Vorteil gegenüber 0% Hysterese:
 * - Verhindert Dauerlauf bei exakter Ziel-Feuchte
 * - Schont Vernebler (weniger Ein/Aus-Zyklen)
 * - Stabiles Regelverhalten ohne "Flattern"
 * 
 * SICHERHEITS-ABSCHALTUNGEN
 * -------------------------
 * 1. DHT22-Fehler:
 *    - Erkennung: readHumidity() oder readTemperature() = NaN
 *    - Reaktion: sensorValid = false, Vernebler SOFORT AUS
 *    - Display: Ist-Temperatur/Feuchte grau
 *    - Hinweis: "Sensor disconnected!" im Serial Monitor
 * 
 * 2. Wassermangel:
 *    - Erkennung: Wassersensor = HIGH (Kontakt offen)
 *    - Reaktion: waterLevelLow = true, Vernebler gesperrt
 *    - Display: "WASSER LEER!" blinkt (1 Hz, rot auf gelb)
 *    - Vernebler startet NICHT mehr, auch bei niedriger Feuchte
 * 
 * 3. Millis-Overflow (nach 49 Tagen):
 *    - Erkennung: millis() < verneblerStartTime oder lastSprayTime
 *    - Reaktion: Timer zurücksetzen auf aktuellen millis()-Wert
 *    - Hinweis: "millis() Overflow" im Serial Monitor
 * 
 * 4. LittleFS-Fehler beim Laden:
 *    - Versionsprüfung: Wenn Version ≠ 1 → Daten verwerfen
 *    - Index-Check: Wenn ringIndex >= RING_SIZE → Reset
 *    - Größenprüfung: Wenn Bytes gelesen ≠ erwartete Größe → Reset
 * 
 * ============================================================================
 * GUI (LVGL) - DISPLAY-LAYOUT
 * ============================================================================
 * 
 * BILDSCHIRM-AUFTEILUNG (280×240 Pixel)
 * --------------------------------------
 * 
 * [Y=20]  Temperatur:        "Temp: 23.5°C"       (Montserrat 18)
 * 
 * [Y=50]  Ist-Luftfeuchte:   "Ist: 58%"           (Montserrat 18, FARBE)
 *         Soll-Luftfeuchte:  "Ziel: 60%"          (Montserrat 18, rechts)
 * 
 * [Y=80]  Letzter Spray:     "Spray: 3 min"       (Montserrat 16)
 *         Blink-Indikator:   [●]                  (10×10px, grün, 1 Hz)
 * 
 * [Y=150] Vernebler-Status:  "Vernebler: aus"     (Montserrat 16)
 * 
 * [Y=180] WiFi-Status:       "WiFi: 192.168.1.42" (Montserrat 16)
 *                        oder "WiFi: Offline"
 *                        oder "WiFi: Keine Config"
 * 
 * [Y=210] Wasser-Warnung:    "WASSER LEER!"     (Montserrat 16, rot/gelb)
 *                            (nur bei Wassermangel sichtbar, blinkt 1 Hz)
 * 
 * FARBCODES IST-LUFTFEUCHTE
 * -------------------------
 * Grün (RGB: 0,170,0):
 *   - Bedingung: (Soll - 1%) ≤ Ist ≤ (Soll + 1%)
 *   - Bedeutung: Optimaler Bereich
 *   - Beispiel: Soll=60%, Ist=59-61% → Grün
 * 
 * Rot (RGB: 255,0,0):
 *   - Bedingung: Ist < (Soll - 1%)
 *   - Bedeutung: Zu niedrig, Vernebler aktiv
 *   - Beispiel: Soll=60%, Ist=58% → Rot
 * 
 * Invertiert Rot (weißer Text auf rotem Hintergrund):
 *   - Bedingung: Ist > (Soll + 1%)
 *   - Bedeutung: Zu hoch (z.B. nach manueller Befeuchtung)
 *   - Beispiel: Soll=60%, Ist=63% → Invertiert
 *   - Hinweis: Keine automatische Entfeuchtung möglich!
 * 
 * Grau (RGB: 136,136,136):
 *   - Bedingung: Sensor-Fehler (NaN-Werte)
 *   - Bedeutung: DHT22 nicht erreichbar oder defekt
 *   - Aktion: Verkabelung/Sensor prüfen
 * 
 * DYNAMISCHE ANZEIGEN
 * -------------------
 * "Spray: X Min":
 *   - Zeigt Zeit seit letztem Sprühstoß
 *   - "Spray: ---" → Noch kein Sprühstoß seit Boot
 *   - "Spray: 0 Min" → Gerade gesprüht (< 1 min)
 *   - "Spray: 15 Min" → Vor 15 Minuten gesprüht
 *   - Update: Jede Sekunde
 * 
 * Blink-Indikator (grüner Punkt):
 *   - Funktion: System-Heartbeat (zeigt: Code läuft)
 *   - Frequenz: 1 Hz (an/aus jede Sekunde)
 *   - Position: Rechts neben "Spray:"
 * 
 * WiFi-Status:
 *   - "WiFi: 192.168.1.42" → Verbunden, zeigt aktuelle IP
 *   - "WiFi: Offline" → Nicht verbunden, arbeitet lokal
 *   - "WiFi: Keine Config" → Keine SSID gespeichert
 *   - "WiFi: Verbindungsfehler" → SSID falsch oder nicht erreichbar
 * 
 * Wasser-Warnung:
 *   - Normal: Versteckt (LV_OBJ_FLAG_HIDDEN)
 *   - Bei Wassermangel: Sichtbar und blinkt (1 Hz)
 *   - Farbe: Roter Text auf gelbem Hintergrund
 *   - Verschwindet automatisch wenn Wasser nachgefüllt
 * 
 * 
 * ============================================================================
 * WEB-INTERFACE
 * ============================================================================
 * 
 * ZUGRIFF AUF WEB-INTERFACE
 * -------------------------
 * Methode 1 - mDNS (empfohlen):
 *   URL: http://brutschrank.local
 *   Voraussetzung: Router/Gerät unterstützt mDNS
 *   Vorteil: IP-Adresse muss nicht bekannt sein
 * 
 * Methode 2 - IP-Adresse:
 *   URL: http://[IP-ADRESSE]
 *   IP ablesen: Display (Zeile WiFi-Status) oder Serial Monitor
 *   Beispiel: http://192.168.1.42
 * 
 * Methode 3 - Router:
 *   Router-Webinterface öffnen
 *   → DHCP-Liste oder verbundene Geräte
 *   → Nach "brutschrank" suchen
 * 
 * VERFÜGBARE SEITEN
 * -----------------
 * 1. Startseite (/)
 *    Funktion: Konfiguration
 *    Features:
 *    - WiFi SSID/Passwort ändern
 *    - Sprühstoß-Dauer einstellen (1-60s)
 *    - Start-Zielluftfeuchte setzen (40-90%)
 *    - [Speichern] Button → Neustart
 *    - Links zu Chart, Download, Manuelles Speichern
 * 
 * 2. Verlaufs-Chart (/chart)
 *    Funktion: Grafische Darstellung der letzten 14 Tage
 *    Technologie: Chart.js (von CDN geladen)
 *    Datenpunkte: Alle 10.080 Messungen
 *    Kurven:
 *    - Ist-Luftfeuchte: Grün, durchgezogen
 *    - Soll-Luftfeuchte: Rot, gestrichelt
 *    - Temperatur: Blau, rechte Y-Achse
 *    Performance:
 *    - Desktop: ~2-5 Sekunden Ladezeit
 *    - Tablet: ~5-10 Sekunden
 *    - Smartphone: ~10-20 Sekunden (ältere Geräte länger)
 *    Hinweis: Ladebalken wird angezeigt während Daten übertragen werden
 * 
 * 3. CSV-Download (/download)
 *    Funktion: Export aller Messdaten als CSV-Datei
 *    Dateiname: luftfeuchte_14tage.csv
 *    Format:
 *    Zeitpunkt_min,Temperatur_C,IstFeuchte_%,SollFeuchte_%
 *    0,22.50,55.3,60
 *    2,22.52,55.5,60
 *    4,22.51,55.8,60
 *    ...
 *    Spalten:
 *    - Zeitpunkt_min: Minuten seit ESP32-Start
 *    - Temperatur_C: Temperatur in °C (2 Dezimalstellen)
 *    - IstFeuchte_%: Gemessene Luftfeuchte (1 Dezimalstelle)
 *    - SollFeuchte_%: Eingestellte Ziel-Feuchte (Ganzzahl)
 *    Dateigröße: ~500 KB (10.080 Zeilen)
 *    Verwendung: Excel, LibreOffice, Python pandas, etc.
 * 
 * 4. Manuelles Speichern (/save_ring)
 *    Funktion: Ringspeicher sofort in LittleFS sichern
 *    Verwendung: Vor Hardware-Änderungen oder Updates
 *    Bestätigung: Dialog-Box vor Ausführung
 *    Feedback: Erfolgsmeldung nach Abschluss
 * 
 * 
 * CHART.JS INTEGRATION
 * --------------------
 * Bibliothek: Chart.js v4.x (von cdn.jsdelivr.net geladen)
 * Chart-Typ: Line Chart (Liniendiagramm)
 * Features:
 * - Zoom: Mausrad / Pinch (Touch)
 * - Pan: Drag / Swipe
 * - Tooltip: Hover über Datenpunkte
 * - Legend: Klickbar (Kurven ein/ausblenden)
 * 
 * Achsen:
 * - X-Achse: Zeit in Minuten (0-20160)
 * - Y-Achse links: Luftfeuchte 0-100%
 * - Y-Achse rechts: Temperatur 0-100°C
 * 
 * Performance-Optimierungen:
 * - pointRadius: 0 (keine Punkte zeichnen)
 * - borderWidth: 1 (dünne Linien)
 * - tension: 0.1 (leichte Glättung)
 * - animation: false (für schnelleres Rendern)
 * 
 * ============================================================================
 * BEDIENUNGSANLEITUNG
 * ============================================================================
 * 
 * ERSTE INBETRIEBNAHME (Schritt-für-Schritt)
 * -------------------------------------------
 * 
 * 1. Hardware aufbauen und verkabeln
 *    □ Alle Komponenten gemäß Pin-Belegung anschließen
 *    □ DHT22 mit 10kΩ Pull-Up zwischen DATA und VCC
 *    □ Wasserstands-Kontakte in Behälter einbauen
 *    □ Vernebler an separates Netzteil anschließen
 *    □ ESP32 per USB mit PC verbinden, Software aufspielen.
 *      Partitionsschema: "Minimal SPIFFS (1.9MB App with OTA/190KB SPIFFS)"
 * 
 * 2. Erste Prüfung
 *    □ Display leuchtet? → Hintergrundbeleuchtung OK
 *    □ Text sichtbar? → Display-Treiber OK
 *    □ "WiFi: Keine Config"? → Normal bei erstem Start
 *    □ Serial Monitor (115200 baud) öffnen:
 *      - "Setup abgeschlossen!" → Code läuft
 *      - "DHT22 initialisiert" → Sensor gefunden
 *      - "LittleFS erfolgreich gestartet" → Dateisystem OK
 * 
 * 3. Config-Mode aktivieren
 *    □ Encoder-Taster 3 Sekunden drücken (lang)
 *    □ Display zeigt: "CONFIG MODE"
 *    □ Display zeigt: SSID, Passwort, IP
 *    □ Hinweis: "Taste -> Beenden" erscheint
 * 
 * 4. WiFi konfigurieren
 *    □ Mit Handy/Tablet/Laptop WLAN-Suche öffnen
 *    □ Netzwerk "brutschrank" auswählen
 *    □ Passwort eingeben: config123
 *    □ Browser öffnen: http://192.168.4.1
 *    □ Konfigurationsseite erscheint
 * 
 * 5. Einstellungen vornehmen
 *    □ WiFi SSID: Name deines Heim-WLANs eingeben
 *    □ WiFi Passwort: WLAN-Passwort eingeben
 *    □ Sprühstoß-Dauer: z.B. 10 Sekunden
 *    □ Start-Zielluftfeuchte: z.B. 60%
 *    □ [Speichern] Button klicken
 * 
 * 6. Neustart abwarten
 *    □ "Gespeichert! Neustart..." erscheint
 *    □ ESP32 startet neu (~5 Sekunden)
 *    □ Display zeigt nun: "WiFi: [IP-Adresse]"
 *    □ Serial Monitor zeigt: "WiFi verbunden! IP: ..."
 * 
 * 7. Funktionstest
 *    □ Encoder drehen → Ziel-Feuchte ändert sich (rot während Änderung)
 *    □ Wasserbehälter füllen → Warnung verschwindet
 *    □ Warten auf Messung → Nach 2 Minuten erste Werte
 *    □ Bei niedriger Feuchte → Vernebler startet automatisch
 *    □ Web-Interface testen: http://brutschrank.local
 * 
 * TÄGLICHER BETRIEB
 * ------------------
 * Normal-Zustand:
 * - Display zeigt aktuelle Werte
 * - Vernebler läuft automatisch bei Bedarf
 * - Grüner Punkt blinkt (Heartbeat)
 * - WiFi zeigt IP-Adresse
 * 
 * Ziel-Luftfeuchte ändern:
 * - Encoder nach links = niedriger (min. 40%)
 * - Encoder nach rechts = höher (max. 90%)
 * - Wert wird sofort rot angezeigt
 * - Nach 5 Minuten: Automatisch in NVS gespeichert
 * 
 * Wasser nachfüllen:
 * - Bei Warnung "WASSER LEER!"
 * - Behälter auffüllen bis Kontakte bedeckt
 * - Warnung verschwindet automatisch (~10 Sekunden)
 * 
 * Web-Interface nutzen:
 * - Browser öffnen: http://brutschrank.local
 * - Chart ansehen: Verlaufs-Button klicken
 * - Daten exportieren: Download-Button klicken
 * - Einstellungen ändern: Werte anpassen → [Speichern]
 * 
 * 
 * ============================================================================
 * TROUBLESHOOTING - PROBLEME UND LÖSUNGEN
 * ============================================================================
 * 
 * DISPLAY-PROBLEME
 * ----------------
 * Display bleibt schwarz:
 *   Ursache: Verkabelung oder TFT_eSPI-Konfiguration
 *   Lösung:
 *   □ Alle Pins prüfen (CS, RST, DC, SCLK, MOSI)
 *   □ VCC auf 3.3V (NICHT 5V!)
 *   □ LED-Pin auf 3.3V (Hintergrundbeleuchtung)
 *   □ User_Setup.h in TFT_eSPI-Bibliothek überprüfen
 *   □ Richtigen Treiber gewählt? (#define ST7789_2_DRIVER)
 * 
 * Display zeigt wirren Text:
 *   Ursache: Falsche Rotation oder Auflösung
 *   Lösung:
 *   □ tft.setRotation(1) im Code prüfen (Zeile ~360)
 *   □ LVGL_DISPLAY_WIDTH 280, HEIGHT 240 korrekt?
 * 
 * Display flackert:
 *   Ursache: Zu viele GUI-Updates oder Stromversorgung
 *   Lösung:
 *   □ Cache-Mechanismus sollte das verhindern
 *   □ Externes 5V-Netzteil verwenden (nicht nur USB)
 *   □ Kondensator 100µF parallel zu Display-VCC
 * 
 * SENSOR-PROBLEME
 * ---------------
 * DHT22 liefert keine Werte (grau):
 *   Ursache: Verkabelung, Stromversorgung oder defekter Sensor
 *   Lösung:
 *   □ Pin-Belegung prüfen (DATA auf GPIO 26)
 *   □ Sensor an 3.3V (NICHT 5V, kann DHT22 beschädigen!)
 *   □ 10kΩ Pull-Up zwischen DATA und VCC löten
 *   □ Serial Monitor: "Sensor disconnected!" → Sensor defekt?
 *   □ Anderen DHT22 zum Testen verwenden
 * 
 * DHT22 zeigt unrealistische Werte:
 *   Ursache: Sensor-Drift oder Kurzschluss
 *   Lösung:
 *   □ Sensor von Wärmequellen fernhalten
 *   □ Sensor nicht direkt neben Vernebler (Tropfen!)
 *   □ Kalibrierung: Mit Referenz-Hygrometer vergleichen
 *   □ DHT22 nach 1-2 Jahren austauschen (normale Alterung)
 * 
 * Wasserstands-Sensor meldet ständig "LEER":
 *   Ursache: Kontakte nicht im Wasser oder Korrosion
 *   Lösung:
 *   □ Wasserstand prüfen (Kontakte müssen bedeckt sein)
 *   □ Kontakte reinigen (Kalkablagerungen entfernen)
 *   □ Kontakt-Material prüfen (Edelstahl verwenden!)
 *   □ Verkabelung: GPIO 25 und GND korrekt?
 *   □ Test: GPIO 25 direkt mit GND verbinden → Warnung weg?
 * 
 * Wasserstands-Sensor reagiert nicht:
 *   Ursache: Pin falsch oder Pull-Up fehlt
 *   Lösung:
 *   □ Multimeter: Spannung zwischen GPIO 25 und GND messen
 *     Ohne Wasser: ~3.3V (Pull-Up aktiv)
 *     Mit Wasser: ~0V (Kontakt nach GND)
 * 
 * ENCODER-PROBLEME
 * ----------------
 * Encoder reagiert nicht:
 *   Ursache: Verkabelung oder falsche Pin-Belegung
 *   Lösung:
 *   □ CLK auf GPIO 34, DT auf GPIO 35
 *   □ VCC auf GPIO 33 oder fest 3.3V
 *   □ GND verbunden?
 *   □ Serial Monitor: "Encoder initialisiert" erscheint?
 * 
 * Encoder dreht falsch herum:
 *   Ursache: CLK und DT vertauscht
 *   Lösung:
 *   □ CLK und DT-Pins tauschen (Hardware oder Code)
 *   □ Oder im Code: ROTARY_ENCODER_A_PIN mit B_PIN tauschen
 * 
 * Encoder-Werte springen:
 *   Ursache: Falsche STEPS-Einstellung oder defekter Encoder
 *   Lösung:
 *   □ ROTARY_ENCODER_STEPS ändern (2 oder 4 versuchen)
 *   □ Zeile: #define ROTARY_ENCODER_STEPS 2
 *   □ Encoder-Kontakte mit Kontaktspray reinigen
 *   □ Anderen Encoder testen
 * 
 * Taster reagiert nicht:
 *   Ursache: Pin falsch oder Pull-Up fehlt
 *   Lösung:
 *   □ SW auf GPIO 32
 *   □ Code: pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP)
 *   □ Test: Pin 32 direkt auf GND → "Button DOWN" im Serial?
 * 
 * Config-Mode startet nicht:
 *   Ursache: Taster-Timing oder Entprellung
 *   Lösung:
 *   □ Taster LÄNGER als 3 Sekunden gedrückt halten
 *   □ Serial Monitor: "Button DOWN" und "Button UP" erscheinen?
 *   □ Dauer zwischen DOWN und UP > 3000ms?
 *   □ LONG_PRESS_TIME im Code anpassen (Zeile ~170)
 * 
 * VERNEBLER-PROBLEME
 * ------------------
 * Vernebler läuft nicht:
 *   Ursache: Relais, Verkabelung, Wasser oder Einstellungen
 *   Systematische Prüfung:
 *   □ 1. Display: "Vernebler: an" angezeigt?
 *      Nein → Ist-Feuchte zu hoch oder Wasser leer
 *   □ 2. Relais-LED leuchtet?
 *      Nein → GPIO 27 Pin oder Relais-VCC prüfen
 *   □ 3. Relais klickt hörbar?
 *      Nein → Relais defekt oder zu schwacher Strom
 *   □ 4. Wasserbehälter voll?
 *      Membran muss im Wasser sein
 * 
 * Vernebler läuft ständig:
 *   Ursache: Relais-Logik invertiert oder Hardware-Fehler
 *   Lösung:
 *   □ Code-Zeile ändern: digitalWrite(RELAY_PIN, LOW) statt HIGH
 *   □ Oder: Relais NC/NO-Kontakte vertauschen
 * 
 * Vernebler sprüht zu viel/wenig:
 *   Ursache: Falsche Sprühstoß-Dauer
 *   Lösung:
 *   □ Web-Interface: Sprühstoß-Dauer anpassen
 *   □ Raum-Größe beachten (großer Raum = längere Dauer)
 * 
 * WIFI-PROBLEME
 * -------------
 * WiFi verbindet nicht:
 *   Ursache: SSID/Passwort falsch oder Reichweite
 *   Lösung:
 *   □ Display: "WiFi: Verbindungsfehler" → SSID/Passwort falsch
 *   □ Display: "WiFi: Keine Config" → Config-Mode nutzen
 *   □ Nur 2.4 GHz WiFi! (5 GHz funktioniert NICHT)
 *   □ WiFi-Reichweite: ESP32 näher an Router stellen
 *   □ Serial Monitor: Detaillierte Fehlermeldung lesen
 *   □ SSID mit Leerzeichen? → Exakt wie im Router eingeben
 * 
 * WiFi trennt immer wieder:
 *   Ursache: Schlechtes Signal oder Router-Problem
 *   Lösung:
 *   □ WiFi-Signal-Stärke prüfen (Serial: "RSSI: -XX dBm")
 *     Gut: -30 bis -50 dBm
 *     Okay: -50 bis -70 dBm
 *     Schlecht: -70 bis -90 dBm
 *   □ ESP32 umpositionieren (näher an Router)
 *   □ Externe WiFi-Antenne verwenden (falls unterstützt)
 *   □ Router: DHCP-Lease-Time erhöhen
 *   □ Router: Energiesparmodus deaktivieren
 * 
 * mDNS funktioniert nicht (brutschrank.local):
 *   Ursache: Betriebssystem oder Netzwerk
 *   Lösung:
 *   □ Windows: Bonjour-Dienst installieren (iTunes enthält es)
 *   □ Mac/iOS: Sollte von Haus aus funktionieren
 *   □ Android: mDNS-Apps im Play Store
 *   □ Alternative: IP-Adresse vom Display ablesen
 * 
 * Web-Interface lädt nicht:
 *   Ursache: WiFi getrennt oder falsche URL
 *   Lösung:
 *   □ Display: "WiFi: [IP]" angezeigt?
 *   □ Ping-Test: ping brutschrank.local (oder IP)
 *   □ Anderer Browser versuchen
 *   □ HTTPS statt HTTP? → Nur HTTP funktioniert!
 *   □ Router-Firewall: ESP32 blockiert?
 * 
 * SPEICHER-PROBLEME
 * -----------------
 * "Nicht genug RAM für Ringspeicher":
 *   Ursache: Zu viele Messungen oder andere RAM-Fressern
 *   Lösung:
 *   □ RING_SIZE reduzieren (z.B. 10080 → 5040 für 7 Tage)
 *   □ Zeile ändern: const int RING_SIZE = 5040;
 *   □ Andere RAM-intensive Features deaktivieren
 * 
 * Flash overflow (Sketch zu groß):
 *   Ursache: Code + Bibliotheken > 1.31 MB
 *   Lösung:
 *   □ DEBUG deaktivieren (spart ~50 KB):
 *     Zeile auskommentieren: // #define DEBUG
 *   □ Chart-Funktion entfernen (spart ~20 KB)
 *   □ Kleinere Partition-Tabelle wählen:
 *     Tools → Partition Scheme → "Minimal SPIFFS"
 * 
 * LittleFS mount fehlgeschlagen:
 *   Ursache: Dateisystem korrupt oder nicht formatiert
 *   Lösung:
 *   □ Serial Monitor: "Formatiere LittleFS..." erscheint?
 *   □ Falls nicht: Im Code LittleFS.format() einmalig aufrufen
 *   □ Nach Formatierung: Zeile wieder entfernen
 *   □ Code hochladen → Daten gehen verloren (neu anlegen)
 * 
 * Alte Daten nach Update:
 *   Ursache: LittleFS behält Daten bei Re-Flash
 *   Lösung:
 *   □ Gewünscht: Daten bleiben erhalten
 *   □ Ungewünscht: LittleFS formatieren (siehe oben)
 *   □ Oder: /ringdata.bin über Web-Interface löschen
 * 
 * CSV-Download bricht ab:
 *   Ursache: Browser-Timeout oder zu langsame Verbindung
 *   Lösung:
 *   □ Normal bei 10.080 Zeilen (~500 KB)
 *   □ Browser-Timeout erhöhen (Developer Tools)
 *   □ RING_SIZE reduzieren für kleinere CSV
 *   □ Schnellere WiFi-Verbindung (näher an Router)
 * 
 * Chart lädt extrem langsam:
 *   Ursache: Zu viele Datenpunkte für Browser
 *   Lösung:
 *   □ Normal bei 10.080 Punkten auf alten Geräten
 *   □ Geduld: 10-30 Sekunden warten
 *   □ Desktop-PC statt Smartphone verwenden
 *   □ RING_SIZE reduzieren (z.B. auf 1440 für 24h)
 *   □ Oder: Chart-Decimation aktivieren (Code anpassen)
 * 
 * SONSTIGE PROBLEME
 * -----------------
 * Serial Monitor zeigt Müll:
 *   Ursache: Falsche Baudrate
 *   Lösung:
 *   □ Baudrate auf 115200 einstellen (nicht 9600!)
 *   □ Zeile im Code: Serial.begin(115200)
 * 
 * ESP32 startet immer wieder neu (Bootloop):
 *   Ursache: Absturz im Code oder Hardware-Problem
 *   Lösung:
 *   □ Serial Monitor: Reset-Grund ablesen
 *     "Brownout" → Stromversorgung zu schwach
 *     "Panic" → Code-Fehler (Stack Trace lesen)
 *     "Watchdog" → Endlosschleife im Code
 *   □ Externes Netzteil verwenden (min. 1A @ 5V)
 *   □ Vernebler trennen (zieht viel Strom?)
 *   □ Code-Fehler suchen (letzte Änderungen rückgängig)
 * 
 * Encoder zeigt falsche Werte nach Neustart:
 *   Ursache: NVS-Wert und Encoder nicht synchronisiert
 *   Lösung:
 *   □ Normal: Encoder wird in setup() auf NVS-Wert gesetzt
 *   □ Zeile prüfen: rotaryEncoder.setEncoderValue(sollFeuchte)
 *   □ Nach 1-2 Sekunden: Encoder sollte korrekt sein
 * 
 * 
 * 
 * IDEEN FÜR ANWENDUNGEN
 * -----------------------------
 * Brutschrank
 * Terrarium
 * Gewächshaus:
 * Pilzzucht:
 * 
 * ============================================================================
 * LIZENZ UND RECHTLICHES
 * ============================================================================
 * 
 * Autor:   Karl Effinger
 * E-Mail:  kaeff@gmx.de
 * Version: 1.0.0
 * Datum:   2025-12-02
 * 
 * LIZENZ
 * ------
 * Dieses Projekt steht unter freier Lizenz:
 * - Freie Verwendung für private und kommerzielle Zwecke
 * - Modifikation und Weitergabe ausdrücklich erlaubt
 * - Namensnennung erwünscht, aber nicht erforderlich
 * - Keine Copyleft-Verpflichtung (kein GPL)
 * 
 * HAFTUNGSAUSSCHLUSS
 * ------------------
 * Diese Software wird "wie besehen" bereitgestellt:
 * - Keine Gewährleistung für Fehlerfreiheit
 * - Keine Haftung für Schäden (Hardware, Tiere, Pflanzen, etc.)
 * - Verwendung auf eigene Gefahr
 * - Insbesondere bei Tierhaltung: Professionelle Beratung einholen!
 * 
 * 
 * DATENSCHUTZ
 * -----------
 * - WiFi-Passwort wird im Flash gespeichert (unverschlüsselt)
 * - Keine Cloud-Verbindung (alle Daten lokal)
 * - Keine Telemetrie oder Tracking
 * - Web-Interface ohne Login (LAN-Zugriff ausreichend)
 * 
 * OPEN SOURCE VERWENDUNG
 * ----------------------
 * Dieses Projekt nutzt folgende Open-Source-Bibliotheken:
 * - LVGL (MIT License)
 * - TFT_eSPI (FreeBSD License)
 * - DHT sensor library (MIT License)
 * - AiEsp32RotaryEncoder (MIT License)
 * - ESP32 Arduino Core (LGPL 2.1)
 * - Chart.js (MIT License, CDN)
 * 
 * Dank an alle Entwickler dieser großartigen Bibliotheken!
 * 
 * ============================================================================
 * PROJEKTKOSTEN UND ZEITAUFWAND
 * ============================================================================
 * 
 * HARDWARE-KOSTEN (geschätzt, Stand 2025)
 * ----------------------------------------
 * ESP32 DevKit:           ~5-8 EUR
 * TFT 1.69" ST7789:       ~8-12 EUR
 * DHT22 Sensor:           ~4-8 EUR
 * Rotary Encoder:         ~2-3 EUR
 * Relais-Modul:           ~2-4 EUR
 * Ultraschall-Vernebler:  ~8-15 EUR
 * Netzteil(e):            ~10-20 EUR
 * Kabel, Stecker, Platine: ~5-10 EUR
 * Gehäuse:                ~5-15 EUR
 * ─────────────────────────────────
 * Gesamt:                 ~50-95 EUR
 * 
 * 
 * ============================================================================
 * CHANGELOG - VERSIONSHISTORIE
 * ============================================================================
 * 
 * v1.0.0 - 2025-12-02 - Erste vollständige Version
 *   Bekannte Einschränkungen v1.0.0:
 *   - Chart mit 10.080 Punkten kann auf alten Geräten langsam sein
 *   - Keine E-Mail-Benachrichtigung
 *   - Keine OTA-Update-Funktion
 *   - Kein MQTT Support
 */


// ============================================================================
// Debugging
// ============================================================================
#define DEBUG
#ifdef DEBUG
  #define DBEGIN() do { Serial.begin(115200); delay(100); } while(0)
  #define DPRINT(...) Serial.print(__VA_ARGS__)
  #define DPRINTLN(...) Serial.println(__VA_ARGS__)
  #define DPRINTF(...) Serial.printf(__VA_ARGS__)
  #define DFLUSH() Serial.flush()
  #include <esp_system.h>
#else
  #define DBEGIN()
  #define DPRINT(...)
  #define DPRINTLN(...)
  #define DPRINTF(...)
  #define DFLUSH()
#endif

// ============================================================================
// Bibliotheken
// ============================================================================
#include <lvgl.h>                    // v8.3.11 LVGL GUI    https://github.com/lvgl/lvgl
#include <TFT_eSPI.h>                // v2.5.43 TFT-Display https://github.com/Bodmer/TFT_eSPI
#include <DHT.h>                     // v1.4.4 DHT22 Sensor https://github.com/adafruit/DHT-sensor-library
#include <AiEsp32RotaryEncoder.h>    // v1.7 Rotary Encoder https://github.com/igorantolic/ai-esp32-rotary-encoder
#include <WiFi.h>                    // WiFi
#include <WebServer.h>               // Webserver
#include <Update.h>                  // für OverTheAir Updates
#include <ESPmDNS.h>                 // mDNS / Hostname
#include <Preferences.h>             // NVS Speicherung
#include <LittleFS.h>                // Speicher für die Messdaten

// ============================================================================
// Hardware-Konfiguration
// ============================================================================
#define DHT_PIN 26                   // GPIO für DHT22
#define DHT_TYPE DHT22               // DHT22 Sensor-Typ
#define RELAY_PIN 27                 // GPIO für Relais
#define WATER_SENSOR_PIN 25          // Wasserstands-Sensor: Diesen Pin und einen GND-Anschluss als Kontaktflächen unten in den Wasservorratsbehälter einbringen.
#define ROTARY_ENCODER_A_PIN 34      // CLK
#define ROTARY_ENCODER_B_PIN 35      // DT
#define ROTARY_ENCODER_BUTTON_PIN 32 // SW (ohne Pull-Up!)
#define ROTARY_ENCODER_VCC_PIN 33    // VCC
#define DEVICE_HOSTNAME "brutschrank"
#define AP_SSID "brutschrank"
#define AP_PASSWORD "Kristof"

// ============================================================================
// Display-Konfiguration
// ============================================================================
TFT_eSPI tft = TFT_eSPI();

#define LVGL_DISPLAY_WIDTH 280
#define LVGL_DISPLAY_HEIGHT 240
#define LVGL_DISPLAY_BUF_ROWS 40
#define LVGL_DISPLAY_BUF_SIZE (LVGL_DISPLAY_WIDTH * LVGL_DISPLAY_BUF_ROWS)

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LVGL_DISPLAY_BUF_SIZE];
static lv_disp_drv_t disp_drv;

// ============================================================================
// GUI-Objekte
// ============================================================================
lv_obj_t *label_temp;                // Ist-Temperatur
lv_obj_t *label_ist_feuchte;         // Ist-Luftfeuchte
lv_obj_t *label_soll_feuchte;        // Soll-Luftfeuchte
lv_obj_t *label_letzter_spray;       // Letzter Sprühstoß
lv_obj_t *label_vernebler;           // Vernebler-Status
lv_obj_t *label_status;              // WiFi-Status
lv_obj_t *blink_dot;                 // Blink-Indikator
lv_obj_t *label_water_warning;       // Wasserstandswarnung

// GUI Cache für Updates
static float gui_last_temp = -1000.0f;
static float gui_last_ist_feuchte = -1000.0f;
static int gui_last_soll_feuchte = -1;
static int gui_last_spray_minuten = -1;
static bool gui_last_vernebler_on = false;
static bool gui_lastWaterLow = false;
static char gui_last_status[64] = "";
static int gui_last_color_code = -1;
static unsigned long encoderLastChange = 0;

// ============================================================================
// DHT22 Sensor
// ============================================================================
DHT dht(DHT_PIN, DHT_TYPE);

// Timing
unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = 2 * 60 * 1000; // alle 2 Minuten messen und ggf. reagieren

// Messwerte
float istTemp = 0.0;
float istFeuchte = 0.0;
bool sensorValid = false;

// ============================================================================
// Rotary Encoder
// ============================================================================
#define ROTARY_ENCODER_STEPS 2
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
  ROTARY_ENCODER_A_PIN,
  ROTARY_ENCODER_B_PIN,
  ROTARY_ENCODER_BUTTON_PIN,
  ROTARY_ENCODER_VCC_PIN,
  ROTARY_ENCODER_STEPS
);

bool configModeRequested = false;
volatile bool buttonStateChanged = false;
volatile unsigned long lastButtonChange = 0;
const unsigned long DEBOUNCE_DELAY = 50;

unsigned long buttonPressStartTime = 0;
bool buttonWasPressed = false;
const unsigned long LONG_PRESS_TIME = 3000;

// ============================================================================
// Systemzustände
// ============================================================================
int sollFeuchte = 60;                // Ziel-Luftfeuchte (40-90%)
int lastSavedSollFeuchte = 60;      // Für NVS-Änderungserkennung
const int hysterese = 1;             // ±1%

int spruehstossDauer = 10;           // Sekunden (Web-Interface änderbar)
bool verneblerOn = false;
unsigned long verneblerStartTime = 0;
unsigned long lastSprayTime = 0;     // Zeitpunkt letzter Sprühstoß
bool waterLevelLow = false;

// ============================================================================
// Ringspeicher (120 Messungen = 2 Stunden)
// ============================================================================
struct MessungCompact {
  uint16_t timestamp;      // 2 Bytes (Minuten seit Start: 0-65535)
  uint16_t temperatur;     // 2 Bytes (Temp × 100: 0-10000 = 0-100°C mit 0.01°C Auflösung)
  uint8_t istFeuchte;      // 1 Byte (Feuchte × 2.55: 0-255 = 0-100% mit ~0.4% Auflösung)
  uint8_t sollFeuchte;     // 1 Byte (0-100%)
};

const int RING_SIZE = 10080;  // 14 Tage × 720 Messungen/Tag
MessungCompact *ringspeicher = nullptr;
int ringIndex = 0;
bool ringFull = false;

const unsigned long RING_SAVE_INTERVAL = 30 * 60 * 1000UL; // 30 Minuten
const char* RING_FILENAME = "/ringdata.bin";

// ============================================================================
// WiFi und Webserver
// ============================================================================
WiFiClient espClient;
WebServer server(80);
Preferences preferences;

String wifi_ssid = "";
String wifi_password = "";

// ============================================================================
// Funktions-Deklarationen
// ============================================================================
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
void create_gui();
void update_gui();
void measureDHT22();
void addToRingspeicher();
void loadConfig();
void saveConfig();
void saveRingspeicherToLittleFS();
void loadRingspeicherFromLittleFS();
void enterConfigMode();
void handleRoot();
void handleSave();
void handleChart();
void handleDownload();
void handleSaveRing();
void handleUpdatePage();
void handleUpdateUpload();
void handleUpdateDone();
void connectWiFi();
void IRAM_ATTR readEncoderISR();
void IRAM_ATTR buttonISR();

// ============================================================================
// ISRs
// ============================================================================
void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonChange > DEBOUNCE_DELAY) {
    buttonStateChanged = true;
    lastButtonChange = now;
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  DBEGIN();
  
  #ifdef DEBUG
    esp_reset_reason_t reason = esp_reset_reason();
    DPRINT("Reset-Grund: ");
    switch (reason) {
      case ESP_RST_POWERON: DPRINTLN("Power-On"); break;
      case ESP_RST_SW: DPRINTLN("Software Reset"); break;
      case ESP_RST_BROWNOUT: DPRINTLN("Brownout"); break;
      default: DPRINTLN("Andere"); break;
    }
  #endif
  
  // Ringspeicher dynamisch allokieren
  DPRINTLN("Allokiere Ringspeicher...");
  ringspeicher = (MessungCompact*)malloc(RING_SIZE * sizeof(MessungCompact));
  
  if (ringspeicher == nullptr) {
    DPRINTLN("FEHLER: Nicht genug RAM für Ringspeicher!");
    DPRINTLN("Reduziere RING_SIZE!");
    while(1) { delay(1000); } // Stoppt hier
  }
  
  DPRINTF("Ringspeicher allokiert: %u Bytes\n", RING_SIZE * sizeof(MessungCompact));
  
  // Initialisieren
  for(int i = 0; i < RING_SIZE; i++) {
    ringspeicher[i].timestamp = 0;
    ringspeicher[i].temperatur = 0;
    ringspeicher[i].istFeuchte = 0;
    ringspeicher[i].sollFeuchte = 0;
  }


  // Konfiguration laden
  loadConfig();

  // LittleFS initialisieren
  if (!LittleFS.begin(true)) {  // true = format if mount fails
    DPRINTLN("LittleFS Mount fehlgeschlagen!");
    DPRINTLN("Formatiere LittleFS...");
    if (LittleFS.format()) {
      DPRINTLN("LittleFS formatiert");
      if (!LittleFS.begin()) {
        DPRINTLN("FEHLER: LittleFS Start nach Format fehlgeschlagen!");
      }
    } else {
      DPRINTLN("FEHLER: LittleFS Formatierung fehlgeschlagen!");
    }
  } else {
    DPRINTLN("LittleFS erfolgreich gestartet");
    
    // Dateisystem-Info ausgeben
    #ifdef DEBUG
      size_t totalBytes = LittleFS.totalBytes();
      size_t usedBytes = LittleFS.usedBytes();
      DPRINTF("LittleFS: %u KB total, %u KB verwendet, %u KB frei\n",
              totalBytes/1024, usedBytes/1024, (totalBytes-usedBytes)/1024);
    #endif
  }
  
  // Display initialisieren
  tft.begin();
  tft.setRotation(1);
  
  // LVGL initialisieren
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LVGL_DISPLAY_BUF_SIZE);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LVGL_DISPLAY_WIDTH;
  disp_drv.ver_res = LVGL_DISPLAY_HEIGHT;
  disp_drv.flush_cb = my_flush_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  
  create_gui();
  lv_refr_now(NULL);
  delay(50);
  
  // DHT22 initialisieren
  dht.begin();
  DPRINTLN("DHT22 initialisiert");
  
  // Rotary Encoder initialisieren
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP); // Software Pull-Up!
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_BUTTON_PIN), buttonISR, CHANGE);
  
  rotaryEncoder.setBoundaries(40, 90, false);
  rotaryEncoder.setAcceleration(50);
  rotaryEncoder.setEncoderValue(sollFeuchte);
  
  DPRINTLN("Rotary Encoder initialisiert");
  
  // Relais initialisieren
  pinMode(RELAY_PIN, OUTPUT);
  verneblerOn = false;
  digitalWrite(RELAY_PIN, LOW);

  // Wasserstands-Sensor
  pinMode(WATER_SENSOR_PIN, INPUT_PULLUP); // Pull-Up aktiv
  DPRINTLN("Wasserstands-Sensor initialisiert");
  
  // gespeicherte Ringspeicher-Daten aus LittleFS laden
  loadRingspeicherFromLittleFS();
  
  // WiFi verbinden
  if (wifi_ssid.length() > 0) {
    connectWiFi();
  } else {
    DPRINTLN("Keine WiFi-Konfiguration");
    if (label_status) {
      lv_label_set_text(label_status, "Keine WiFi Config");
    }
  }
  
  DPRINTLN("\n=== Setup abgeschlossen ===");
  DPRINTLN("Führe erste Messung durch...");
  measureDHT22();
  if (sensorValid) {
    addToRingspeicher();
  }
  lastMeasurement = millis();

  DPRINTLN("Encoder: Luftfeuchte einstellen");
  DPRINTLN("Langer Tastendruck: Config-Mode");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // Config-Modus
  if (configModeRequested) {
    enterConfigMode();
    ESP.restart();
  }

  // LVGL Zeitbasis
  static unsigned long lv_last_tick = 0;
  unsigned long now = millis();
  if (lv_last_tick == 0) lv_last_tick = now;
  if (now - lv_last_tick >= 5) {
    lv_tick_inc(5);
    lv_last_tick += 5;
  }
  
  lv_task_handler();
  delay(5);

  // Overflow-Schutz (nach 49 Tagen)
  if (verneblerStartTime > 0 && millis() < verneblerStartTime) {
    DPRINTLN("millis() Overflow - Vernebler-Timer zurückgesetzt");
    verneblerStartTime = millis();
  }
  if (lastSprayTime > 0 && millis() < lastSprayTime) {
    DPRINTLN("millis() Overflow - Spray-Timer zurückgesetzt");
    lastSprayTime = millis();
  }
  
  // DHT22 Messung, Wasserstandsmessung, ggf. Vernebler anschalten
  if (now - lastMeasurement >= measurementInterval) {
    measureDHT22();
    checkWaterLevel();
    if (!verneblerOn &&                                    // Läuft nicht bereits
        sensorValid &&                                     // Sensor funktioniert
        !waterLevelLow &&                                  // Wasser vorhanden (wichtig!)
        istFeuchte < (sollFeuchte - hysterese)) {          // Zu trocken
      verneblerOn = true;
      verneblerStartTime = now;
      digitalWrite(RELAY_PIN, HIGH);
      DPRINTF("Sprühstoß gestartet (Ist: %.1f%%, Soll: %d%%)\n", istFeuchte, sollFeuchte);
    }
    addToRingspeicher();
    lastMeasurement = now;
  }

  // nach der Sprühstoßdauer den Vernebler wieder ausschalten
  if (verneblerOn) {
    if (now - verneblerStartTime >= (unsigned long)spruehstossDauer * 1000UL) {
      verneblerOn = false;
      digitalWrite(RELAY_PIN, LOW);
      lastSprayTime = now;
      DPRINTLN("Sprühstoß beendet");
    }
  }
  
  // Rotary Encoder auslesen
  if (rotaryEncoder.encoderChanged()) {
    int newValue = rotaryEncoder.readEncoder();
    if (newValue != sollFeuchte) {
      sollFeuchte = newValue;
      DPRINTF("Neue Soll-Feuchte: %d%%\n", sollFeuchte);
      
      // Sofort GUI aktualisieren
      char buf[32];
      snprintf(buf, sizeof(buf), "Ziel: %d%%", sollFeuchte);
      lv_label_set_text(label_soll_feuchte, buf);
      lv_obj_set_style_text_color(label_soll_feuchte, lv_color_hex(0xFF0000), 0);
      gui_last_soll_feuchte = sollFeuchte;
      lv_refr_now(NULL);
      encoderLastChange = millis();
    }
  }
  
  // Button-Erkennung
  if (buttonStateChanged) {
    buttonStateChanged = false;
    bool isPressed = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == LOW);
    
    if (isPressed && !buttonWasPressed) {
      buttonWasPressed = true;
      buttonPressStartTime = millis();
      DPRINTLN("Button DOWN");
    }
    else if (!isPressed && buttonWasPressed) {
      unsigned long pressDuration = millis() - buttonPressStartTime;
      buttonWasPressed = false;
      DPRINTF("Button UP (%lu ms)\n", pressDuration);
      
      if (pressDuration > LONG_PRESS_TIME) {
        configModeRequested = true;
        DPRINTLN("==> Config-Mode");
      }
    }
  }

  // Webserver bedienen
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
  
  // Auto-Save Config (alle 5 Minuten)
  static unsigned long lastSave = 0;
  if (millis() - lastSave >= 5 * 60 * 1000UL) {
    saveConfig();
    lastSave = millis();
  }
  
  // Auto-Save Ringspeicher (alle 30 Minuten)
  static unsigned long lastRingSave = 0;
  if (millis() - lastRingSave >= RING_SAVE_INTERVAL) {
    saveRingspeicherToLittleFS();
    lastRingSave = millis();
  }

  // GUI aktualisieren (alle 200ms)
  static unsigned long lastGuiUpdate = 0;
  if (millis() - lastGuiUpdate >= 200) {
    update_gui();
    lastGuiUpdate = millis();
  }
}

// ============================================================================
// LVGL Flush Callback
// ============================================================================
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(drv);
}

// ============================================================================
// GUI erstellen
// ============================================================================
void create_gui() {
  lv_obj_t *scr = lv_scr_act();
  
  // Ist-Temperatur
  label_temp = lv_label_create(scr);
  lv_label_set_text(label_temp, "Temp: --.-°C");
  lv_obj_set_pos(label_temp, 10, 20);
  lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_18, 0);
  
  // Ist-Luftfeuchte
  label_ist_feuchte = lv_label_create(scr);
  lv_label_set_text(label_ist_feuchte, "Ist: --%");
  lv_obj_set_pos(label_ist_feuchte, 10, 50);
  lv_obj_set_style_text_font(label_ist_feuchte, &lv_font_montserrat_18, 0);
  
  // Soll-Luftfeuchte
  label_soll_feuchte = lv_label_create(scr);
  lv_label_set_text(label_soll_feuchte, "Ziel: --%");
  lv_obj_set_pos(label_soll_feuchte, 150, 50);
  lv_obj_set_style_text_font(label_soll_feuchte, &lv_font_montserrat_18, 0);
  
  // Letzter Sprühstoß
  label_letzter_spray = lv_label_create(scr);
  lv_label_set_text(label_letzter_spray, "Spray: ---");
  lv_obj_set_pos(label_letzter_spray, 10, 80);
  lv_obj_set_style_text_font(label_letzter_spray, &lv_font_montserrat_16, 0);
  
  // Blink-Dot
  blink_dot = lv_obj_create(scr);
  lv_obj_set_size(blink_dot, 10, 10);
  lv_obj_set_style_radius(blink_dot, 2, 0);
  lv_obj_set_style_bg_color(blink_dot, lv_color_hex(0x00AA00), 0);
  lv_obj_set_pos(blink_dot, 150, 85);
  lv_obj_clear_flag(blink_dot, LV_OBJ_FLAG_HIDDEN);
  
  // Vernebler-Status
  label_vernebler = lv_label_create(scr);
  lv_label_set_text(label_vernebler, "Vernebler: aus");
  lv_obj_set_pos(label_vernebler, 10, 150);
  lv_obj_set_style_text_font(label_vernebler, &lv_font_montserrat_16, 0);
  
  // WiFi-Status
  label_status = lv_label_create(scr);
  lv_label_set_text(label_status, "WiFi: ---");
  lv_obj_set_pos(label_status, 10, 180);
  lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);

  // Wasser-Warnung
  label_water_warning = lv_label_create(scr);
  lv_label_set_text(label_water_warning, LV_SYMBOL_CHARGE " WASSER LEER " LV_SYMBOL_CHARGE);
  lv_obj_set_pos(label_water_warning, 65, 210);
  lv_obj_set_style_text_font(label_water_warning, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(label_water_warning, lv_color_hex(0xFF0000), 0); // Rot
  lv_obj_set_style_bg_color(label_water_warning, lv_color_hex(0xFFFF00), 0); // Gelber Hintergrund
  lv_obj_set_style_bg_opa(label_water_warning, LV_OPA_COVER, 0);
  lv_obj_add_flag(label_water_warning, LV_OBJ_FLAG_HIDDEN); // Initial versteckt
}

// ============================================================================
// GUI aktualisieren
// ============================================================================
void update_gui() {
  char buf[80];
  unsigned long now = millis();

  if (encoderLastChange > 0 && (now - encoderLastChange >= 2000)) {
    // 2 Sekunden sind vorbei → Farbe zurück auf schwarz
    lv_obj_set_style_text_color(label_soll_feuchte, lv_color_hex(0x000000), 0);
    encoderLastChange = 0;
    lv_refr_now(NULL);
  }
  
  // Temperatur und Luftfeuchte
  static unsigned long last_temp_update = 0;
  if (now - last_temp_update >= 500) {
    last_temp_update = now;
    
    if (istTemp != gui_last_temp) {
      snprintf(buf, sizeof(buf), "Temp: %.1f°C", istTemp);
      lv_label_set_text(label_temp, buf);
      gui_last_temp = istTemp;
    }
    
    if (istFeuchte != gui_last_ist_feuchte) {
      snprintf(buf, sizeof(buf), "Ist: %.0f%%", istFeuchte);
      lv_label_set_text(label_ist_feuchte, buf);
      gui_last_ist_feuchte = istFeuchte;
    }
    
    if (sollFeuchte != gui_last_soll_feuchte) {
      snprintf(buf, sizeof(buf), "Ziel: %d%%", sollFeuchte);
      lv_label_set_text(label_soll_feuchte, buf);
      gui_last_soll_feuchte = sollFeuchte;
    }
    
    // Farbe für Ist-Feuchte
    int colorCode;
    if (!sensorValid) colorCode = 3; // grau
    else if (istFeuchte < (sollFeuchte - hysterese)) colorCode = 1; // rot (zu niedrig)
    else if (istFeuchte > (sollFeuchte + hysterese)) colorCode = 2; // invertiert rot (zu hoch)
    else colorCode = 0; // grün
    
    if (colorCode != gui_last_color_code) {
      switch (colorCode) {
        case 0: // grün
          lv_obj_set_style_text_color(label_ist_feuchte, lv_color_hex(0x00AA00), 0);
          lv_obj_set_style_bg_opa(label_ist_feuchte, LV_OPA_TRANSP, 0);
          break;
        case 1: // rot
          lv_obj_set_style_text_color(label_ist_feuchte, lv_color_hex(0xFF0000), 0);
          lv_obj_set_style_bg_opa(label_ist_feuchte, LV_OPA_TRANSP, 0);
          break;
        case 2: // invertiert rot
          lv_obj_set_style_bg_color(label_ist_feuchte, lv_color_hex(0xFF0000), 0);
          lv_obj_set_style_bg_opa(label_ist_feuchte, LV_OPA_COVER, 0);
          lv_obj_set_style_text_color(label_ist_feuchte, lv_color_hex(0xFFFFFF), 0);
          break;
        default: // grau
          lv_obj_set_style_text_color(label_ist_feuchte, lv_color_hex(0x888888), 0);
          lv_obj_set_style_bg_opa(label_ist_feuchte, LV_OPA_TRANSP, 0);
          break;
      }
      gui_last_color_code = colorCode;
    }
  }
  
  // Letzter Spray (jede Sekunde)
  static unsigned long last_spray_update = 0;
  if (now - last_spray_update >= 1000) {
    last_spray_update = now;
    
    int spray_minuten;
    if (lastSprayTime == 0) {
      spray_minuten = -1; // noch nie
    } else {
      spray_minuten = (millis() - lastSprayTime) / 60000UL;
    }
    
    if (spray_minuten != gui_last_spray_minuten) {
      if (spray_minuten < 0) {
        snprintf(buf, sizeof(buf), "Spray: ---");
      } else {
        snprintf(buf, sizeof(buf), "Spray: %d Min", spray_minuten);
      }
      lv_label_set_text(label_letzter_spray, buf);
      gui_last_spray_minuten = spray_minuten;
    }
    
    // Wasser-Warnung blinken
    if (waterLevelLow) {
      static bool warnBlink = false;
      warnBlink = !warnBlink;
      if (warnBlink) {
        lv_obj_clear_flag(label_water_warning, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(label_water_warning, LV_OBJ_FLAG_HIDDEN);
      }
    }

    // Blink-Indikator
    static bool blink_state = true;
    blink_state = !blink_state;
    blink_state ? lv_obj_clear_flag(blink_dot, LV_OBJ_FLAG_HIDDEN) 
                : lv_obj_add_flag(blink_dot, LV_OBJ_FLAG_HIDDEN);
  }
  
  // Vernebler-Status
  if (verneblerOn != gui_last_vernebler_on) {
    snprintf(buf, sizeof(buf), "Vernebler: %s", verneblerOn ? "AN" : "aus");
    lv_label_set_text(label_vernebler, buf);
    gui_last_vernebler_on = verneblerOn;
  }
  
  // WiFi-Status
  const char *new_status;
  static char status_buf[64];
  
  if (WiFi.status() == WL_CONNECTED) {
    // IP-Adresse anzeigen
    snprintf(status_buf, sizeof(status_buf), "WiFi: %s", 
             WiFi.localIP().toString().c_str());
    new_status = status_buf;
  } else if (wifi_ssid.length() == 0) {
    new_status = "WiFi: Keine Config";
  } else {
    // Offline-Modus
    new_status = "WiFi: Offline";
  }
  
  if (strcmp(new_status, gui_last_status) != 0) {
    strncpy(gui_last_status, new_status, sizeof(gui_last_status)-1);
    gui_last_status[sizeof(gui_last_status)-1] = '\0';
    lv_label_set_text(label_status, new_status);
  }
}

// ============================================================================
// DHT22 Messung
// ============================================================================
void measureDHT22() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    DPRINTLN("DHT22 Lesefehler!");
    sensorValid = false;
    return;
  }
  
  istTemp = t;
  istFeuchte = h;
  sensorValid = true;
  
  DPRINTF("DHT22: %.1f°C, %.1f%%\n", t, h);
}

// ============================================================================
// Ringspeicher
// ============================================================================
void addToRingspeicher() {
  if (!sensorValid) return;
  
  ringspeicher[ringIndex].timestamp = millis() / 60000UL; // Minuten seit Start
  ringspeicher[ringIndex].temperatur = encodeTemperatur(istTemp);
  ringspeicher[ringIndex].istFeuchte = encodeFeuchte(istFeuchte);
  ringspeicher[ringIndex].sollFeuchte = sollFeuchte;
  
  ringIndex++;
  if (ringIndex >= RING_SIZE) {
    ringIndex = 0;
    ringFull = true;
  }
  
  DPRINTF("Ring[%d]: %lu min, %.2f°C, %.1f%%, Soll: %d%%\n",
          ringIndex-1, 
          (unsigned long)ringspeicher[(ringIndex-1+RING_SIZE)%RING_SIZE].timestamp,
          decodeTemperatur(ringspeicher[(ringIndex-1+RING_SIZE)%RING_SIZE].temperatur),
          decodeFeuchte(ringspeicher[(ringIndex-1+RING_SIZE)%RING_SIZE].istFeuchte),
          ringspeicher[(ringIndex-1+RING_SIZE)%RING_SIZE].sollFeuchte);
}

// ============================================================================
// Ringspeicher in LittleFS speichern
// ============================================================================
void saveRingspeicherToLittleFS() {
  DPRINTLN("Speichere Ringspeicher nach LittleFS...");
  
  File file = LittleFS.open(RING_FILENAME, "w");
  if (!file) {
    DPRINTLN("FEHLER: Kann Datei nicht zum Schreiben öffnen!");
    return;
  }
  
  // Metadaten speichern (Version für zukünftige Kompatibilität)
  uint8_t version = 1;
  file.write(&version, sizeof(version));
  
  // Index und Full-Flag speichern
  file.write((uint8_t*)&ringIndex, sizeof(ringIndex));
  file.write((uint8_t*)&ringFull, sizeof(ringFull));
  
  // Komplettes Array speichern
  size_t written = file.write((uint8_t*)ringspeicher, sizeof(ringspeicher));
  file.close();
  
  if (written == sizeof(ringspeicher)) {
    DPRINTF("Ringspeicher gespeichert: %u Bytes (Index: %d, Full: %d)\n",
            written, ringIndex, ringFull);
  } else {
    DPRINTF("FEHLER: Nur %u von %u Bytes geschrieben!\n",
            written, sizeof(ringspeicher));
  }
}

// ============================================================================
// Ringspeicher aus LittleFS laden
// ============================================================================
void loadRingspeicherFromLittleFS() {
  DPRINTLN("Lade Ringspeicher aus LittleFS...");
  
  if (ringspeicher == nullptr) {
    DPRINTLN("FEHLER: Ringspeicher nicht allokiert!");
    return;
  }
  
  if (!LittleFS.exists(RING_FILENAME)) {
    DPRINTLN("Keine gespeicherten Daten gefunden (erster Start)");
    return;
  }
  
  File file = LittleFS.open(RING_FILENAME, "r");
  if (!file) {
    DPRINTLN("FEHLER: Kann Datei nicht zum Lesen öffnen!");
    return;
  }
  
  // Version prüfen
  uint8_t version;
  file.read(&version, sizeof(version));
  if (version != 1) {
    DPRINTF("WARNUNG: Unbekannte Datenversion %d, ignoriere Daten\n", version);
    file.close();
    return;
  }
  
  // Metadaten laden
  file.read((uint8_t*)&ringIndex, sizeof(ringIndex));
  file.read((uint8_t*)&ringFull, sizeof(ringFull));
  
  // Plausibilitätsprüfung
  if (ringIndex >= RING_SIZE) {
    DPRINTF("FEHLER: Ungültiger ringIndex %d (max: %d)\n", ringIndex, RING_SIZE-1);
    ringIndex = 0;
    ringFull = false;
    file.close();
    return;
  }
  
  // Daten laden
  size_t bytesRead = file.read((uint8_t*)ringspeicher, sizeof(ringspeicher));
  file.close();
  
  if (bytesRead == sizeof(ringspeicher)) {
    int count = ringFull ? RING_SIZE : ringIndex;
    DPRINTF("Ringspeicher geladen: %u Bytes (%d Messungen)\n", bytesRead, count);
    
    // Erste und letzte Messung anzeigen
    if (count > 0) {
      int firstIdx = ringFull ? ringIndex : 0;
      int lastIdx = ringFull ? ((ringIndex - 1 + RING_SIZE) % RING_SIZE) : (ringIndex - 1);
      
      DPRINTF("  Erste: %lu min, %.2f°C, %.1f%%\n",
              (unsigned long)ringspeicher[firstIdx].timestamp,
              decodeTemperatur(ringspeicher[firstIdx].temperatur),
              decodeFeuchte(ringspeicher[firstIdx].istFeuchte));
      
      DPRINTF("  Letzte: %lu min, %.2f°C, %.1f%%\n",
              (unsigned long)ringspeicher[lastIdx].timestamp,
              decodeTemperatur(ringspeicher[lastIdx].temperatur),
              decodeFeuchte(ringspeicher[lastIdx].istFeuchte));
    }
  } else {
    DPRINTF("FEHLER: Nur %u von %u Bytes gelesen!\n", bytesRead, sizeof(ringspeicher));
    // Bei Fehler: Array zurücksetzen
    ringIndex = 0;
    ringFull = false;
  }
}

// ============================================================================
// Wasserstand prüfen
// ============================================================================
void checkWaterLevel() {
  // Sensor lesen (LOW = Wasser vorhanden, HIGH = kein Wasser)
  // Grund: Pull-Up aktiv, Wasser schließt Kontakt nach GND
  bool sensorState = digitalRead(WATER_SENSOR_PIN);
  
  if (sensorState == HIGH) {
    // Kein Wasser erkannt
    if (!waterLevelLow) {
      waterLevelLow = true;
      DPRINTLN("⚠ WARNUNG: Wasserstand niedrig!");
    }
  } else {
    // Wasser vorhanden
    if (waterLevelLow) {
      waterLevelLow = false;
      DPRINTLN("✓ Wasserstand OK");
      
      // Warnung ausblenden
      if (label_water_warning) {
        lv_obj_add_flag(label_water_warning, LV_OBJ_FLAG_HIDDEN);
        lv_refr_now(NULL);
      }
    }
  }
}

// ============================================================================
// Konfiguration laden
// ============================================================================
void loadConfig() {
  preferences.begin("luftbefeuchter", false);
  
  wifi_ssid = preferences.getString("wifi_ssid", "");
  wifi_password = preferences.getString("wifi_pass", "");
  
  sollFeuchte = preferences.getInt("soll_feuchte", 60);
  lastSavedSollFeuchte = sollFeuchte;
  
  spruehstossDauer = preferences.getInt("spray_dauer", 10);
  
  preferences.end();
  
  DPRINTLN("Konfiguration geladen:");
  DPRINTLN("WiFi SSID: " + wifi_ssid);
  DPRINTF("Soll-Feuchte: %d%%\n", sollFeuchte);
  DPRINTF("Sprühstoß-Dauer: %ds\n", spruehstossDauer);
}

// ============================================================================
// Konfiguration speichern
// ============================================================================
void saveConfig() {
  preferences.begin("luftbefeuchter", false);
  
  if (sollFeuchte != lastSavedSollFeuchte) {
    preferences.putInt("soll_feuchte", sollFeuchte);
    lastSavedSollFeuchte = sollFeuchte;
    DPRINTF("Soll-Feuchte gespeichert: %d%%\n", sollFeuchte);
  }
  
  preferences.end();
}

// ============================================================================
// Config-Modus
// ============================================================================
void enterConfigMode() {
  DPRINTLN("Starte Config-Modus...");

  // Ringspeicher-Daten sichern
  saveRingspeicherToLittleFS();
  DPRINTLN("Ringspeicher vor Config-Mode gesichert");
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(40, 10);
  tft.println("CONFIG MODE");
  
  tft.setCursor(10, 40);
  tft.println("SSID: " + String(AP_SSID));
  tft.setCursor(10, 65);
  tft.println("Pass: config123");
  tft.setCursor(10, 90);
  tft.println("URL:  192.168.4.1");
  
  tft.drawLine(0, 115, 280, 115, TFT_WHITE);
  
  tft.setCursor(10, 135);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.println("Taste -> Beenden");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  delay(500);
  
  IPAddress IP = WiFi.softAPIP();
  tft.setCursor(10, 165);
  tft.setTextSize(1);
  tft.print("AP gestartet: ");
  tft.println(IP);
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
  tft.setCursor(10, 180);
  tft.println("Webserver aktiv");
  DPRINTLN("Webserver gestartet");
  
  bool buttonPressed = false;
  
  while (!buttonPressed) {
    server.handleClient();
    
    if (rotaryEncoder.isEncoderButtonDown()) {
      while (rotaryEncoder.isEncoderButtonDown()) {
        delay(10);
      }
      buttonPressed = true;
      
      tft.fillScreen(TFT_GREEN);
      tft.setTextColor(TFT_BLACK, TFT_GREEN);
      tft.setTextSize(3);
      tft.setCursor(10, 100);
      tft.println("Beende");
      tft.setCursor(10, 130);
      tft.println("Config");
      
      DPRINTLN("Config-Mode beendet");
      delay(1000);
      break;
    }
    
    delay(10);
  }
}

// ============================================================================
// Webserver: Root
// ============================================================================
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Luftbefeuchter Config</title>"
    "<style>"
    "body{font-family:Arial;margin:20px;background:#f0f0f0;}"
    ".c{max-width:600px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;}"
    "h1{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px;}"
    "input,select{width:100%;padding:10px;margin:8px 0;box-sizing:border-box;"
    "border:1px solid #ddd;border-radius:4px;}"
    "button{width:100%;padding:12px;margin-top:20px;background:#007bff;"
    "color:#fff;border:none;border-radius:4px;font-size:16px;cursor:pointer;}"
    "button:hover{background:#0056b3;}"
    ".btn-secondary{background:#28a745;margin-top:10px;}"
    ".btn-secondary:hover{background:#218838;}"
    ".btn-warning{background:#ffc107;color:#333;margin-top:10px;}"  // ← NEU
    ".btn-warning:hover{background:#e0a800;}"  // ← NEU
    "label{display:block;margin-top:10px;color:#555;font-weight:bold;}"
    "</style></head><body><div class='c'>"
    "<h1>Luftbefeuchter Config</h1>"
    "<form action='/save' method='POST'>"
    "<h2>WiFi</h2>"
    "<label>SSID:</label><input type='text' name='wifi_ssid' value='");
  html += wifi_ssid;
  html += F("' required>"
    "<label>Passwort:</label><input type='password' name='wifi_pass' value='");
  html += wifi_password;
  html += F("'>"
    "<h2>Einstellungen</h2>"
    "<label>Sprühstoß-Dauer (Sekunden):</label>"
    "<input type='number' name='spray_dauer' min='1' max='60' value='");
  html += String(spruehstossDauer);
  html += F("'>"
    "<label>Start-Zielluftfeuchte (%):</label>"
    "<input type='number' name='start_soll' min='40' max='90' value='");
  html += String(sollFeuchte);
  html += F("'>"
    "<button type='submit'>Speichern</button>"
    "</form>"
    "<button class='btn-secondary' onclick=\"location.href='/chart'\">📊 Verlaufs-Chart</button>"
    "<button class='btn-secondary' onclick=\"location.href='/download'\">💾 Daten Download (CSV)</button>"
    "<button class='btn-secondary' onclick=\"if(confirm('Ringspeicher jetzt speichern?')){location.href='/save_ring'}\">💾 Daten jetzt sichern</button>"
    "<button class='btn-warning' onclick=\"location.href='/update'\">🔄 Firmware Update (OTA)</button>"  // ← NEU
    "</div></body></html>");
  
  server.send(200, F("text/html"), html);
}

// ============================================================================
// Webserver: Save
// ============================================================================
void handleSave() {
  tft.fillRect(0, 210, 280, 30, TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setCursor(10, 215);
  tft.println("Speichere...");
  
  if (server.hasArg("wifi_ssid")) wifi_ssid = server.arg("wifi_ssid");
  if (server.hasArg("wifi_pass")) wifi_password = server.arg("wifi_pass");
  if (server.hasArg("spray_dauer")) spruehstossDauer = server.arg("spray_dauer").toInt();
  if (server.hasArg("start_soll")) sollFeuchte = server.arg("start_soll").toInt();
  
  preferences.begin("luftbefeuchter", false);
  preferences.putString("wifi_ssid", wifi_ssid);
  preferences.putString("wifi_pass", wifi_password);
  preferences.putInt("spray_dauer", spruehstossDauer);
  preferences.putInt("soll_feuchte", sollFeuchte);
  preferences.end();
  
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<style>body{font-family:Arial;text-align:center;margin:50px;}");
  html += F(".s{background:#fff;padding:40px;border-radius:10px;}</style>");
  html += F("</head><body><div class='s'>");
  html += F("<h1>✓ Gespeichert!</h1><p>Neustart...</p>");
  html += F("</div></body></html>");
  
  server.send(200, F("text/html"), html);
  
  DPRINTLN("Config gespeichert, Neustart...");
  delay(2000);
  ESP.restart();
}

// ============================================================================
// Webserver: Chart zeichnen (nutzt cdn.jsdelivr.net/npm/chart.js )
// ============================================================================
void handleChart() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Luftfeuchte-Verlauf</title>"
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "<script>"
    "window.chartLoaded=false;"
    "window.addEventListener('load',function(){"
    "setTimeout(function(){"
    "if(typeof Chart==='undefined'){"
    "document.getElementById('loading').innerHTML="
    "'❌ <b>Chart.js konnte nicht geladen werden!</b><br><br>'+"
    "'<div style=\"text-align:left;max-width:400px;margin:0 auto;\">'+"
    "'<b>Mögliche Ursachen:</b><ul>'+"
    "'<li>Kein Internet-Zugang</li>'+"
    "'<li>CDN (cdn.jsdelivr.net) nicht erreichbar</li>'+"
    "'<li>Firewall blockiert externe Scripts</li>'+"
    "'</ul>'+"
    "'<b>Alternative:</b><br>'+"
    "'Nutzen Sie den CSV-Download und öffnen Sie die Daten in Excel/LibreOffice.</div>';"
    "document.getElementById('chart').style.display='none';"
    "var btn=document.createElement('button');"
    "btn.textContent='💾 CSV herunterladen';"
    "btn.style.cssText='padding:10px 20px;margin:20px;background:#28a745;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:16px';"
    "btn.onclick=function(){location.href='/download';};"
    "document.getElementById('loading').appendChild(btn);"
    "}else{"
    "window.chartLoaded=true;"
    "initChart();"
    "}"
    "},500);"
    "});"
    "</script>"
    "<style>"
    "body{font-family:Arial;margin:20px;background:#f0f0f0;}"
    ".c{max-width:1200px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;}"
    ".info{background:#fff3cd;padding:10px;margin:10px 0;border-radius:5px;border-left:4px solid #ffc107;}"
    "button{padding:10px 20px;margin:5px;background:#007bff;color:#fff;border:none;border-radius:4px;cursor:pointer;}"
    "button:hover{background:#0056b3;}"
    "#loading{text-align:center;padding:20px;font-size:18px;}"
    "</style></head><body><div class='c'>"
    "<h1>📊 Luftfeuchte-Verlauf (14 Tage)</h1>"
    "<div class='info'>⚠️ <b>Hinweis:</b> Chart enthält 10.080 Datenpunkte. "
    "Das Laden kann auf Mobilgeräten einige Sekunden dauern.</div>"
    "<div id='loading'>⏳ Lade Daten...</div>"
    "<canvas id='chart' style='display:none;'></canvas>"
    "<br><button onclick=\"location.href='/'\">← Zurück</button>"
    "<script>"
    "function initChart(){"
    "const labels=[");
  
  // Daten vorbereiten
  int count = ringFull ? RING_SIZE : ringIndex;
  int start = ringFull ? ringIndex : 0;
  
  // Labels
  for(int i = 0; i < count; i++) {
    int idx = (start + i) % RING_SIZE;
    if (i > 0) html += ",";
    html += String(ringspeicher[idx].timestamp);
  }
  
  html += F("];"
    "const temp=[");
  
  // Temperatur
  for(int i = 0; i < count; i++) {
    int idx = (start + i) % RING_SIZE;
    if (i > 0) html += ",";
    html += String(decodeTemperatur(ringspeicher[idx].temperatur), 2);
  }
  
  html += F("];"
    "const ist=[");
  
  // Ist-Feuchte
  for(int i = 0; i < count; i++) {
    int idx = (start + i) % RING_SIZE;
    if (i > 0) html += ",";
    html += String(decodeFeuchte(ringspeicher[idx].istFeuchte), 1);
  }
  
  html += F("];"
    "const soll=[");
  
  // Soll-Feuchte
  for(int i = 0; i < count; i++) {
    int idx = (start + i) % RING_SIZE;
    if (i > 0) html += ",";
    html += String(ringspeicher[idx].sollFeuchte);
  }
  
  // Chart erstellen
  html += F("];"
    "document.getElementById('loading').style.display='none';"
    "document.getElementById('chart').style.display='block';"
    "new Chart(document.getElementById('chart'),{"
    "type:'line',"
    "data:{"
    "labels:labels,"
    "datasets:["
    "{label:'Ist-Feuchte (%)',data:ist,borderColor:'rgb(0,170,0)',borderWidth:1,pointRadius:0,tension:0.1},"
    "{label:'Soll-Feuchte (%)',data:soll,borderColor:'rgb(255,0,0)',borderDash:[5,5],borderWidth:1,pointRadius:0,tension:0.1},"
    "{label:'Temperatur (°C)',data:temp,borderColor:'rgb(0,0,255)',borderWidth:1,pointRadius:0,yAxisID:'y1',tension:0.1}"
    "]},"
    "options:{"
    "responsive:true,"
    "maintainAspectRatio:false,"
    "plugins:{"
    "decimation:{enabled:false},"
    "legend:{display:true}"
    "},"
    "scales:{"
    "x:{display:true,title:{display:true,text:'Zeit (Minuten)'}},"
    "y:{beginAtZero:false,title:{display:true,text:'Luftfeuchte (%)'}},"
    "y1:{position:'right',beginAtZero:false,title:{display:true,text:'Temperatur (°C)'}}"
    "}"
    "}"
    "});"
    "}"
    "</script>"
    "</div></body></html>");
  
  server.send(200, F("text/html"), html);
}

// ============================================================================
// Webserver: CSV Download
// ============================================================================
void handleDownload() {
  String csv = "Zeitpunkt_min,Temperatur_C,IstFeuchte_%,SollFeuchte_%\n";
  
  int count = ringFull ? RING_SIZE : ringIndex;
  int start = ringFull ? ringIndex : 0;
  
  for(int i = 0; i < count; i++) {
    int idx = (start + i) % RING_SIZE;
    
    csv += String(ringspeicher[idx].timestamp) + ",";
    csv += String(decodeTemperatur(ringspeicher[idx].temperatur), 2) + ",";
    csv += String(decodeFeuchte(ringspeicher[idx].istFeuchte), 1) + ",";
    csv += String(ringspeicher[idx].sollFeuchte) + "\n";
  }
  
  server.sendHeader("Content-Disposition", "attachment; filename=luftfeuchte_daten.csv");
  server.send(200, "text/csv", csv);
}

// ============================================================================
// Webserver: Ringspeicher speichern erzwingen
// ============================================================================
void handleSaveRing() {
  saveRingspeicherToLittleFS();
  
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  html += F("<style>body{font-family:Arial;text-align:center;margin:50px;}");
  html += F(".s{background:#fff;padding:40px;border-radius:10px;}</style>");
  html += F("</head><body><div class='s'>");
  html += F("<h1>✓ Gespeichert!</h1>");
  html += F("<p>Ringspeicher (60 KB) wurde gesichert.</p>");
  html += F("<button onclick=\"location.href='/'\">← Zurück</button>");
  html += F("</div></body></html>");
  
  server.send(200, F("text/html"), html);
}

// ============================================================================
// OTA Update Seite
// ============================================================================
void handleUpdatePage() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>OTA Firmware Update</title>"
    "<style>"
    "body{font-family:Arial;margin:20px;background:#f0f0f0;text-align:center;}"
    ".c{max-width:600px;margin:0 auto;background:#fff;padding:30px;border-radius:10px;}"
    "h1{color:#333;border-bottom:2px solid #ffc107;padding-bottom:10px;}"
    ".warning{background:#fff3cd;padding:15px;margin:20px 0;border-radius:5px;"
    "border-left:4px solid #ffc107;text-align:left;}"
    "input[type=file]{margin:20px 0;padding:10px;border:2px dashed #ddd;"
    "border-radius:4px;width:100%;box-sizing:border-box;}"
    "button{padding:15px 30px;margin:10px;background:#ffc107;color:#333;"
    "border:none;border-radius:4px;font-size:18px;cursor:pointer;font-weight:bold;}"
    "button:hover{background:#e0a800;}"
    ".btn-back{background:#6c757d;color:#fff;}"
    ".btn-back:hover{background:#5a6268;}"
    "#progress-container{display:none;margin:20px 0;}"
    "#progress-bar{width:100%;height:30px;background:#e9ecef;border-radius:15px;overflow:hidden;}"
    "#progress-fill{height:100%;background:#28a745;width:0%;transition:width 0.3s;"
    "display:flex;align-items:center;justify-content:center;color:#fff;font-weight:bold;}"
    "#status{margin:20px 0;font-size:18px;font-weight:bold;}"
    ".success{color:#28a745;}"
    ".error{color:#dc3545;}"
    "</style></head><body><div class='c'>"
    "<h1>🔄 Firmware Update (OTA)</h1>"
    "<div class='warning'>"
    "⚠️ <b>WICHTIG:</b><ul style='text-align:left;'>"
    "<li>Update dauert ca. 30-60 Sekunden</li>"
    "<li>Während des Updates: NICHT ausschalten!</li>"
    "<li>WiFi muss stabil sein</li>"
    "<li>Nach Update: Gerät startet automatisch neu</li>"
    "<li>Nur .bin Dateien hochladen!</li>"
    "</ul></div>"
    "<form id='upload-form' method='POST' action='/update' enctype='multipart/form-data'>"
    "<input type='file' name='update' id='file-input' accept='.bin' required>"
    "<br><button type='submit' id='upload-btn'>📤 Update starten</button>"
    "</form>"
    "<button class='btn-back' onclick=\"location.href='/'\">← Zurück</button>"
    "<div id='progress-container'>"
    "<div id='status'>Uploading...</div>"
    "<div id='progress-bar'><div id='progress-fill'>0%</div></div>"
    "</div>"
    "<script>"
    "document.getElementById('upload-form').onsubmit=function(e){"
    "e.preventDefault();"
    "var file=document.getElementById('file-input').files[0];"
    "if(!file){alert('Bitte Datei auswählen!');return;}"
    "if(!file.name.endsWith('.bin')){alert('Nur .bin Dateien erlaubt!');return;}"
    "document.getElementById('upload-btn').disabled=true;"
    "document.getElementById('progress-container').style.display='block';"
    "var formData=new FormData();"
    "formData.append('update',file);"
    "var xhr=new XMLHttpRequest();"
    "xhr.upload.addEventListener('progress',function(e){"
    "if(e.lengthComputable){"
    "var percent=Math.round((e.loaded/e.total)*100);"
    "document.getElementById('progress-fill').style.width=percent+'%';"
    "document.getElementById('progress-fill').textContent=percent+'%';"
    "document.getElementById('status').textContent='Uploading: '+percent+'%';"
    "}"
    "});"
    "xhr.addEventListener('load',function(){"
    "if(xhr.status==200){"
    "document.getElementById('status').textContent='✅ Update erfolgreich! Neustart in 5 Sekunden...';"
    "document.getElementById('status').className='success';"
    "document.getElementById('progress-fill').style.background='#28a745';"
    "setTimeout(function(){location.href='/';},5000);"
    "}else{"
    "document.getElementById('status').textContent='❌ Update fehlgeschlagen! ('+xhr.status+')';"
    "document.getElementById('status').className='error';"
    "document.getElementById('progress-fill').style.background='#dc3545';"
    "document.getElementById('upload-btn').disabled=false;"
    "}"
    "});"
    "xhr.addEventListener('error',function(){"
    "document.getElementById('status').textContent='❌ Verbindungsfehler!';"
    "document.getElementById('status').className='error';"
    "document.getElementById('upload-btn').disabled=false;"
    "});"
    "xhr.open('POST','/update');"
    "xhr.send(formData);"
    "};"
    "</script>"
    "</div></body></html>");
  
  server.send(200, F("text/html"), html);
}

// ============================================================================
// OTA Update Handler
// ============================================================================
void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    // Update starten
    String filename = upload.filename;
    
    // Display-Meldung
    if (label_status) {
      lv_label_set_text(label_status, "OTA Update...");
      lv_refr_now(NULL);
    }
    
    // LittleFS schließen (wichtig!)
    LittleFS.end();
    
    // Update beginnen
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Daten schreiben
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
    
    // Optional: Fortschritt auf Display
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 1000) {  // Alle 1s
      lastDisplayUpdate = millis();
      if (label_status) {
        char buf[32];
        unsigned int percent = (Update.progress() * 100) / Update.size();
        snprintf(buf, sizeof(buf), "OTA: %u%%", percent);
        lv_label_set_text(label_status, buf);
        lv_refr_now(NULL);
      }
    }
    
  } else if (upload.status == UPLOAD_FILE_END) {
    // Update beenden
    if (Update.end(true)) {
      // Erfolg!
      if (label_status) {
        lv_label_set_text(label_status, "Update OK!");
        lv_refr_now(NULL);
      }
    } else {
      // Fehler
      Update.printError(Serial);
      if (label_status) {
        lv_label_set_text(label_status, "Update FEHLER!");
        lv_refr_now(NULL);
      }
    }
  }
}

// ============================================================================
// OTA Update Abschluss
// ============================================================================
void handleUpdateDone() {
  server.sendHeader("Connection", "close");
  
  if (Update.hasError()) {
    server.send(500, F("text/plain"), F("Update FAILED!"));
  } else {
    server.send(200, F("text/plain"), F("Update OK! Rebooting..."));
    
    // 2 Sekunden warten, dann neu starten
    delay(2000);
    ESP.restart();
  }
}

// ============================================================================
// WiFi verbinden
// ============================================================================
void connectWiFi() {
  DPRINTLN("Verbinde WiFi: " + wifi_ssid);
  
  if (label_status) {
    lv_label_set_text(label_status, "WiFi verbindet...");
    lv_refr_now(NULL);
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DEVICE_HOSTNAME);  // Hostname setzen
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DPRINTLN("WiFi verbunden!");
    DPRINT("IP: ");
    DPRINTLN(WiFi.localIP());
    DPRINTF("Hostname: %s.local\n", DEVICE_HOSTNAME);

    // mDNS starten
    if (!MDNS.begin(DEVICE_HOSTNAME)) {  // ← NEU
      DPRINTLN("mDNS FEHLER!");
    } else {
      DPRINTLN("mDNS gestartet");
    }
    
    // Webserver starten
    server.on("/", HTTP_GET, handleRoot);         // Startseite
    server.on("/save", HTTP_POST, handleSave);    // zum Speichern der Parameter (WLAN und Sprühdauer)
    server.on("/chart", HTTP_GET, handleChart);   // Anzeigen des Charts aus den Ringspeicherdaten
    server.on("/download", HTTP_GET, handleDownload);      // zum Download der Daten aus dem Ringspeicher 
    server.on("/save_ring", HTTP_GET, handleSaveRing);     // Speichern der aktuellen Daten in den Ringspeicher
    server.on("/update", HTTP_GET, handleUpdatePage);      // Upload-Seite für OTA-Update
    server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);  // Upload-Handler
    server.begin();
    
    if (label_status) {
      lv_label_set_text(label_status, "WiFi OK");
      lv_refr_now(NULL);
    }
  } else {
    DPRINTLN("\n=== WiFi Verbindung fehlgeschlagen ===");
    DPRINTLN("Gerät arbeitet im Offline-Modus");
    
    if (label_status) {
      lv_label_set_text(label_status, "WiFi: Verbindungsfehler");  // ← Präziser
      lv_refr_now(NULL);
    }
  }
}

// ============================================================================
// Hilfsfunktionen für die Datenspeicherung im Ringspeicher
// ============================================================================
// Temperatur in komprimiertes Format konvertieren (0.01°C Auflösung)
uint16_t encodeTemperatur(float temp) {
  if (temp < 0) temp = 0;
  if (temp > 100) temp = 100;
  return (uint16_t)(temp * 100);
}

// Komprimierte Temperatur zurück in float
float decodeTemperatur(uint16_t encoded) {
  return (float)encoded / 100.0f;
}

// Luftfeuchte in komprimiertes Format (0.4% Auflösung)
uint8_t encodeFeuchte(float feuchte) {
  if (feuchte < 0) feuchte = 0;
  if (feuchte > 100) feuchte = 100;
  return (uint8_t)(feuchte * 2.55f);
}

// Komprimierte Feuchte zurück in float
float decodeFeuchte(uint8_t encoded) {
  return (float)encoded / 2.55f;
}