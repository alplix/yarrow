/* User interface text: German. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

/* units */
#define S_USVH   "µSv/h"
#define S_MSVH   "mSv/h"
#define S_NSVH   "nSv/h"
#define S_USV    "µSv"
#define S_MSV    "mSv"
#define S_NSV    "nSv"
#define S_CPM    "CPM"
#define S_VM     "V/m"
#define S_UT     "µT"

/* alarm levels */
#define S_NORMAL    "NORMAL"
#define S_ATTENTION "VORSICHT"
#define S_DANGER    "GEFAHR"
#define S_EVACUATE  "EVAKUIEREN"
#define S_LOW       "NIEDRIG"
#define S_MEDIUM    "MITTEL"
#define S_HIGH      "HOCH"

/* titles */
#define S_T_RADIATION "Strahlung"
#define S_T_LIVE      "Live"
#define S_T_HISTORY   "Verlauf"
#define S_T_EFIELD    "Elektrisches Feld"
#define S_T_HFIELD    "Magnetisches Feld"
#define S_T_BATTERY   "Akku & Gerät"
#define S_T_GUIDE     "Notfallanleitung"
#define S_T_SETTINGS  "Einst."
#define S_T_EVENTS    "Ereign."
#define S_T_BUDGET    "Dosisbudget"
#define S_T_SAMPLE    "Probe"
#define S_T_LOG       "Lg"
#define S_T_STORAGE   "Speicher"
#define S_T_THRESH    "Alarmstufen"
#define S_T_TUBE      "Röhrenstatus"
#define S_T_SELFTEST  "Selbtest"
#define S_T_USB       "PC-Link"
#define S_T_MODE      "Messmodus"
#define S_T_SCAN      "Feldfinder"
#define S_T_MENU      "Menü"
#define S_T_THEMES    "Themen"
#define S_T_ABOUT     "Über"
#define S_T_CLOCK     "Zeit & Datum"
#define S_T_CALIB     "Kalibrierung"
#define S_CLOCK_DATE  "Zeit & Datum"
#define S_DAY    "Tag"
#define S_MONTH  "Monat"
#define S_YEAR   "Jahr"
#define S_HOUR   "Stunde"
#define S_MINUTE "Minute"
#define S_NEXT   "Weiter"
#define S_SAVE   "Speich"
#define S_CAL_SENS   "Empfindlichkeit"
#define S_CAL_DEAD   "Totzeit"
#define S_CAL_E      "Elektrischer Faktor"
#define S_CAL_H      "Magnetfaktor"
#define S_CAL_NOTE   "Gerät mit bekannter Quelle kalibrieren."

/* labels */
#define S_TODAY     "Heute"
#define S_WEEK      "Woche"
#define S_TOTAL     "Gesamt"
#define S_MIN       "Min"
#define S_AVG       "Mittel"
#define S_MAX       "Max"
#define S_PEAK      "Spitze"
#define S_AVERAGE   "Mittelwert"
#define S_NOW       "Jetzt"
#define S_REC_ON    "Aufzeichnung"
#define S_BAT_LEFT  "Batterie"
#define S_LAST_10   "letzte 10 Min"
#define S_NORMAL_x  "normal"
#define S_TIMES     "mal"
#define S_THRESHOLD "Pegel"
#define S_SCALE     "Skala"
#define S_DETAILS   "Details"
#define S_OPEN      "Öffnen"
#define S_BACK      "Zurück"
#define S_SELECT    "Auswählen"
#define S_MUTE      "Stumm"
#define S_GUIDE     "Anleitung"
#define S_CHANGE    "Ändern"
#define S_RESET     "Reset"
#define S_FINISH    "Beenden"
#define S_CANCEL    "Abbrechen"
#define S_ON        "an"
#define S_OFF       "aus"
#define S_YES       "ja"
#define S_NO        "nein"
#define S_DONE      "fertig"

/* battery screen */
#define S_REMAINING "Restzeit"
#define S_HEALTH    "Akku-Zustand"
#define S_CYCLES    "Ladezyklen"
#define S_RUNTIME   "Gesamtbetriebszeit"
#define S_TUBE_LIFE "Röhrenlebensdauer"
#define S_HOURS     "h"
#define S_MINUTES   "Min"
#define S_CHARGING  "Aufladen"
#define S_FULL_BAT  "Akku voll"
#define S_PRESS_ANY "Taste drücken"
#define S_CHARGED   "voll"
#define S_NO_USB    "kein USB"

/* alarm */
#define S_STEP1 "Hinein"
#define S_STEP2 "Türen und Fenster schließen"
#define S_STEP3 "Nachrichten hören"

/* settings */
#define S_THEME      "Thema"
#define S_BRIGHTNESS "Helligkeit"
#define S_SOUND      "Alarmton"
#define S_VIBRATION  "Alarmvibration"
#define S_LANGUAGE   "Sprache"
#define S_ATT_TH     "Warnstufe"
#define S_CLICK      "Klick zählen"
#define S_COUNT_VIBE "Vibration zählen"
#define S_USB_START  "Start über USB-Strom"
#define S_LED_COLOR  "LED Farbe"
#define LED_COLOR_NAMES {"Thema", "Rot", "Grün", "Blau", "Gelb", "Cyan", "Magenta", "Weiß"}
#define S_LED_PULSE  "Zähl-LED"
#define S_LED_CHARGE "Lad-LED"
#define S_TIME_ZONE  "Zeitzone"
#define S_TH_ATTENTION "Vorsicht"
#define S_TH_DANGER    "Gefahr"
#define S_TH_EVACUATE  "Evakuieren"
#define S_SOUND_LIGHT  "Ton · Vibration · Licht"
#define S_TURKISH    "Türkisch"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Dunkel", "Hell", "Papier", "Bernstein", "Sakura", "Nacht", "Terminal", "Ozean", "Wald", "Kontrast", "Lavendel", "Minze", "Abendrot", "Holzkohle", "Sand", "Eis", "Kirsche", "Violett", "Süß", "Mono", "Karmin"}
#define S_THEME_DARK   "Dunkel"
#define S_THEME_LIGHT  "Hell"
#define S_THEME_PAPER  "Papier"
#define S_THEME_AMBER  "Amber"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profil"
#define S_P_DETAIL   "Detailliert"
#define S_P_DENSE    "Dicht"
#define S_P_BALANCED "Mittel"
#define S_P_LONG     "Lang"
#define S_P_ARCHIVE  "Archiv"
#define S_DAYS       "Tage"
#define S_MONTHS     "Monate"
#define S_YEARS      "Jahre"
#define S_OLDEST     "Ältestes Datum"
#define S_WHEN_FULL  "Vo"
#define S_OVERWRITE  "Überschreiben"
#define S_FULL_PCT   "Vo"
#define S_FREE       "frei"
#define S_RECORDED   "Tage gespeichert"
#define S_ALARM_LOG  "Alarmprot."
#define S_AUTO_NOTE  "Alarmmomente werden auch jede Sekunde gespeichert."
#define S_USB_DOWNLOAD "USB: Protokolle laden"

/* sample test, budget, scan */
#define S_BACKGROUND "Hintergrund"
#define S_SAMPLE     "Probe"
#define S_NET_DIFF   "Netto-Differenz"
#define S_RESULT     "Ergebnis"
#define S_HIGHER     "Deutlich über Hintergrund"
#define S_SAME       "Gleich Hintergrund"
#define S_MEASURE_TIME "Messzeit"
#define S_GOAL       "Ziel"
#define S_MISSION    "Budget"
#define S_START      "Gestartet"
#define S_DURATION   "Dauer"
#define S_LEFT       "Rest"
#define S_USED_PCT   "verwendet"
#define S_ON_EXCEED  "Über Grenzwert: Alarm"
#define S_NEW_BUDGET "Neu B."
#define S_FIND_SRC   "Näher an der Quelle: höherer Ton, höhere Balken."
#define S_APPROACH   "Näher kommen"
#define S_SOUND_ON   "Ton an"
#define S_SCAN_SOUND "Suchton"
#define S_MODE_NAV   "Modus"
#define S_EXIT       "Beenden"

/* tube, self test, usb */
#define S_HV         "Hohe Spannung"
#define S_HV_FACTORY "Werkseinstellung"
#define S_TOTAL_PULSES "Gesamtpulse"
#define S_BG_RATE    "Hintergrund"
#define S_DEADTIME   "Totzeitkorrektur"
#define S_HV_PROTECTED "HV geschützt"
#define S_CHK_HV      "Hochspannung"
#define S_CHK_TUBE    "Röhrenpulse"
#define S_CHK_BAT     "Batterie-Wert"
#define S_CHK_E       "Elektrischer Sensor"
#define S_CHK_H       "Magnetischer Sensor"
#define S_CHK_CLOCK   "Uhrzeit"
#define S_CHK_MEM     "Speicher"
#define S_USB_STATE   "VERBUNDEN"
#define S_STREAM      "Live-Stream"
#define S_CHARGE_A    "Ladung"
#define S_CLOCK_SYNC  "Uhr"
#define S_SYNCED      "synchronisiert"
#define S_RECORDS     "Aufzeichnungen"
#define S_TRANSFER    "Übertragen"
#define S_ALL_ON_PC   "Alle Aufzeichnungen auf dem PC"
#define S_DOWNLOAD_NOW "Jetzt laden"
#define S_AUTO_ON     "Auto: An"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Aufzeichnungen"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Was im Falle eines nuklearen Notfalls zu tun ist.", "Liste vergangener Hochdosisereignisse.", \
    "Zeigt, wie viel von der gewählten Dosis verbraucht wurde.", "Separate Dosisaufzeichnungen für Ausflüge oder Spaziergänge.", \
    "Prüft, ob ein Objekt Strahlung abgibt.", "Findet Kabel und Feldsonden durch Ton und Balken.", \
    "Aufzeichnungszeit und Speicherfüllstand.", "Warn- und Alarmstufen einstellen.", \
    "Röhrenzustand, Totzeit, Hochspannung.", "Überprüft die Gerätekomponenten einzeln.", \
    "Mit PC verbinden und Aufzeichnungen übertragen.", "Farbschema des Bildschirms wählen.", \
    "Helligkeit, Ton, Vibration und Uhr.", "Empfindlichkeits- und Feldkoeffizienten manuell einstellen.", \
    "Empfindlichkeit mit bekannter Quelle einstellen.", "Version und Projektverknüpfungen.", \
    "Misst die Totzeit der Röhre mit zwei Quellen."}
#define SETTINGS_DESC {"Farbschema des Bildschirms wählen.", "Ein gedimmter Bildschirm verlängert die Akkulaufzeit.", \
    "Soll Alarm und Warnung einen Ton erzeugen.", "Soll das Gerät bei Alarm vibrieren.", \
    "Ein Klick pro Zählrohr, wie bei klassischen Geigerzählern.", "Der Motor piept bei jeder Zählung kurz.", \
    "Die LED blinkt bei jeder Zählung. Ausschalten spart etwas Strom.", "Die LED leuchtet, während der Akku geladen wird.", \
    "Farbe der Zähl- und Lad-LED. Das Thema folgt der Akzentfarbe des Bildschirms.", \
    "Soll das Gerät bei USB-Strom einschalten? Wenn nicht, halten Sie den Ein-/Aus-Taster gedrückt.", \
    "Welche Zeitzone die Uhr anzeigt.", "Zeit und Datum manuell einstellen.", \
    "Zeigt im Leerlauf eine große Uhr und den Strahlungspegel, statt den Bildschirm auszuschalten.", \
    "Dauerbetrieb: Display nach 30 s dunkel, USB-Neustart bei Hängen."}
/* sessions */
#define S_T_SESSIONS   "Sitzungen"
#define S_SESS_NEW     "Neue Sitzung starten"
#define S_SESS_RUNNING "Laufende Sitzung"
#define S_SESS_NONE    "Noch keine gespeicherten Sitzungen."
#define S_SESS_PICK    "Sitzungstyp wählen"
#define S_SESS_HINT    "Während einer Sitzung werden Dosis, Zeit und höchste Rate aufgezeichnet. Es läuft weiter, wenn Sie das Gerät ausschalten."
#define SESS_KINDS_NAMES  {"Trip / Flug", "Gehen", "Innenmessung", "Außen", "Um Probe herum", "Allgemein"}
#define S_SESS_DOSE    "Dosis erhalten"
#define S_SESS_TIME    "Dauer"
#define S_SESS_AVG     "Durchschnitt"
#define S_SESS_MAX     "Höchstwert"
#define S_SESS_NOW     "Jetzt"
#define S_SESS_ASK     "OK erneut drücken zum Beenden"
#define S_FINISH       "Beenden"
#define S_BEGIN        "Start"
#define S_APPLY        "anwenden"
#define S_DISCARD      "verwerfen"
#define S_BROWSE       "blättern"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Tage natürlicher Dosis"
#define S_NATURAL_LESS "Weniger als 1 Tag natürlicher Dosis"
/* calibration wizard */
#define S_CW_T0        "Referenzdosisrate"
#define S_CW_T1        "Messung"
#define S_CW_T2        "Ergebnis"
#define S_CW_HELP0     "Platzieren Sie Yarrow neben eine Quelle mit bekannter Dosisrate: der Wert eines anderen Geräts oder einer zertifizierten Quelle. Geben Sie diesen Wert ein."
#define S_CW_COUNTS    "Zählung"
#define S_CW_HINT1     "Etwa 3 % Fehler bei 1000 Zählungen. Eine längere Messung ist genauer."
#define S_CW_SAT       "Röhre zu stark belastet: Quelle entfernen."
#define S_CW_NEEDMORE  "Mindestens 100 Zählungen nötig."
#define S_CW_OLD       "Strom"
#define S_CW_NEW       "Neu"
#define S_CW_DIFF      "Ändern"
#define S_CW_UNC       "Unsicherheit"
#define S_CW_RANGE     "Wert außerhalb des Bereichs: erneut messen."
#define S_M_GUIDE    "Notfallanleitung"
#define S_M_EVENTS   "Ereignisprotokoll"
#define S_M_BUDGET   "Dosisbudget"
#define S_M_SAMPLE   "Probenprüfung"
#define S_M_SCAN     "Feld- & Drahtfinder"
#define S_M_SESSIONS "Sitzungen"
#define S_M_CALWIZ   "Kalibrierassistent"
#define S_M_LOG      "Protokoll & Speicher"
#define S_M_THRESH   "Alarmstufen"
#define S_M_TUBE     "Röhrenstatus"
#define S_M_SELFTEST "Selbtest"
#define S_M_USB      "PC Link"
#define S_M_SETTINGS "Einstellungen"
#define S_M_THEMES   "Themen"
#define S_M_ABOUT    "Über"
#define S_M_CALIB    "Kalibrierung"
#define S_OPEN_SOURCE "Open Source · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Bleiben Sie drinnen", \
     {"Gehen Sie in das nächste Gebäude, Keller oder Innenraum, wenn möglich.", \
      "Türen, Fenster und Lüftung schließen.", \
      "Klimaanlage und Lüfter ausschalten.", \
      "Hören Sie offizielle Durchsagen im Radio oder am Telefon.", \
      "Verlassen Sie das Haus nicht, bis die Behörden es erlauben.", 0, 0}}, \
    {"Wenn Sie draußen waren", \
     {"Äußere Kleidung ausziehen und in einem Beutel versiegeln. Das entfernt die meisten Kontaminationen.", \
      "Mit Seife und Wasser duschen, Haare waschen.", \
      "Augen und Wunden mit sauberem Wasser spülen.", \
      "Haut nicht reiben oder reizen.", 0, 0, 0}}, \
    {"Wasser und Nahrung", \
     {"Lebensmittel und Getränke in versiegelter Verpackung sind sicher.", \
      "Draußen offen liegende Lebensmittel und Gartenprodukte nicht essen.", \
      "Warten Sie auf die Behörden bezüglich Leitungswasser.", \
      "Hände vor dem Essen waschen.", 0, 0, 0}}, \
    {"Kaliumiodid", \
     {"Nur einnehmen, wenn es die Behörden anordnen.", \
      "Es schützt die Schilddrüse nur vor radioaktivem Iod.", \
      "Bei Schwangerschaft und Kindern: Dosierung beim Arzt/Apotheker erfragen.", \
      "Nicht eigenmächtig einnehmen.", 0, 0, 0}}, \
    {"Informationen", \
     {"Radio (FM/AM) und Telefonansagen hören.", \
      "Notruf: 112.", \
      "Stromausfall möglich: Batterie-Radio und geladenes Handy bereithalten.", 0, 0, 0, 0}}, \
    {"Dosis-Tabelle", \
     {"Natürlicher Hintergrund: ca. 0.1 µSv/h, 2.4 mSv pro Jahr.", \
      "Öffentlicher Grenzwert: 1 mSv/Jahr.", \
      "Brust-Röntgen: 0.1 mSv.", \
      "CT-Scan: 2–10 mSv.", \
      "Strahlenschutzpersonal: 20 mSv/Jahr.", \
      "1000 mSv oder mehr auf einmal: Strahlenkrankheit.", 0}}, \
    {"Alarmstufen", \
     {"Vorsicht: Anstieg über Hintergrund.", \
      "Gefahr: Innen gehen und Anleitung lesen.", \
      "Evakuieren: Offizielle Anweisungen befolgen.", \
      "Die Werte können in den Einstellungen geändert werden.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Stufe "
#define S_HELD_BACK    "Zurückgehalten"
#define S_RETRY        "Nochmal"
#define S_SAVER        "Bildschirmschoner"
#define S_STATION      "Stationsbetrieb"
#define S_H_TITLE       "Zustand"
#define S_H_STARTS     "Starts"
#define S_H_THIS       "Dieser Start"
#define S_H_WDT        "Watchdog-Resets"
#define S_H_CRASHES    "Abstürze"
#define S_H_USB        "USB-Neustarts"
#define S_H_RUNNING    "Läuft"
#define S_H_STARTUP    "Start-up"
#define S_H_STAGE      "Stufe"
#define S_H_LASTCRASH  "Letzter Absturz um"
#define S_M_DEADTIME   "Dead-Time-Assistent"
#define S_DT_A         "Quelle A allein"
#define S_DT_AB        "Quellen A und B zusammen"
#define S_DT_B         "Quelle B allein"
#define S_DT_NONE      "Keine Quelle vorhanden"
#define S_DT_INTRO     "Benötigt zwei Quellen gleicher Stärke, je mindestens 500 Zählung/Sekunde, und eine stationäre Röhre. Vier Messungen à 2 Minuten: A, A+B, B und keine Quelle."
#define S_DT_WARN      "Ohne zwei solche Quellen ist das Ergebnis nicht zuverlässig. Der Assistent weist darauf hin."
#define S_DT_PLACE     "Platzieren Sie die oben genannte Quelle/Quellen vor die Röhre und starten Sie. Bewegen Sie nichts während der Zählung."
#define S_DT_NORES     "Kein nutzbarer Wert."
#define S_DT_NORES_D   "Die Quellen waren zu schwach oder eine wurde bewegt. Der Zähldifferenz war nicht deutlich größer als das Rauschen."
#define H_REASONS {"unbekannt", "einschalten", "reset-Pin", "watchdog", "fenster-watchdog", "software", "niedrige leistung", "absturz"}
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "Letzte 7 Tage"
#define S_PEAK_WORD    "Peak "
#define S_MILLION      " Millionen"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 Std"}
#define L_WEEKDAYS     {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " Std"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Offline"
