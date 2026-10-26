/* User interface text: Polish. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "OSTRZEŻENIE"
#define S_DANGER    "RYZYKO"
#define S_EVACUATE  "EWAKUUJ"
#define S_LOW       "NISKI"
#define S_MEDIUM    "ŚREDNI"
#define S_HIGH      "WYSOKI"

/* titles */
#define S_T_RADIATION "Promieniowanie"
#define S_T_LIVE      "ŻYWY"
#define S_T_HISTORY   "Historia"
#define S_T_EFIELD    "Pole elektryczne"
#define S_T_HFIELD    "Pole magnetyczne"
#define S_T_BATTERY   "Bateria i sprzęt"
#define S_T_GUIDE     "Poradnik awaryjny"
#define S_T_SETTINGS  "Ustawienia"
#define S_T_EVENTS    "Zdarzenia"
#define S_T_BUDGET    "Budżet dawki"
#define S_T_SAMPLE    "Próbka"
#define S_T_LOG       "Log"
#define S_T_STORAGE   "Pamięć"
#define S_T_THRESH    "Poziomy alarmu"
#define S_T_TUBE      "Stan rurki"
#define S_T_SELFTEST  "Samotest"
#define S_T_USB       "Połączenie PC"
#define S_T_MODE      "Tryb pomiaru"
#define S_T_SCAN      "Lokalizator pola"
#define S_T_MENU      "Menu"
#define S_T_THEMES    "Motywy"
#define S_T_ABOUT     "O nas"
#define S_T_CLOCK     "Data i czas"
#define S_T_CALIB     "Kalibracja"
#define S_CLOCK_DATE  "Data i czas"
#define S_DAY    "Dzień"
#define S_MONTH  "Miesiąc"
#define S_YEAR   "Rok"
#define S_HOUR   "Godzina"
#define S_MINUTE "Minuta"
#define S_NEXT   "Dalej"
#define S_SAVE   "Zapisz"
#define S_CAL_SENS   "Czułość"
#define S_CAL_DEAD   "Czas martwy"
#define S_CAL_E      "Współczynnik"
#define S_CAL_H      "Współczynnik magnetyczny"
#define S_CAL_NOTE   "Dostosuj urządzenie, porównując je z znanym źródłem."

/* labels */
#define S_TODAY     "Dziś"
#define S_WEEK      "Tydzień"
#define S_TOTAL     "Całkowity"
#define S_MIN       "Min"
#define S_AVG       "Śr"
#define S_MAX       "Max"
#define S_PEAK      "Szczyt"
#define S_AVERAGE   "Średnia"
#define S_NOW       "Teraz"
#define S_REC_ON    "Rejestracja"
#define S_BAT_LEFT  "Bateria"
#define S_LAST_10   "Ostatnie 10 min"
#define S_NORMAL_x  "normalne"
#define S_TIMES     "razy"
#define S_THRESHOLD "poziom"
#define S_SCALE     "skala"
#define S_DETAILS   "szczegóły"
#define S_OPEN      "otwórz"
#define S_BACK      "wstecz"
#define S_SELECT    "wybierz"
#define S_MUTE      "wycisz"
#define S_GUIDE     "pomoc"
#define S_CHANGE    "zmień"
#define S_RESET     "reset"
#define S_FINISH    "skończ"
#define S_CANCEL    "anuluj"
#define S_ON        "włącz"
#define S_OFF       "wyłącz"
#define S_YES       "tak"
#define S_NO        "nie"
#define S_DONE      "gotowe"

/* battery screen */
#define S_REMAINING "Pozostały czas"
#define S_HEALTH    "Stan baterii"
#define S_CYCLES    "Cykle ładowania"
#define S_RUNTIME   "Całkowity czas pracy"
#define S_TUBE_LIFE "Żywot rurki"
#define S_HOURS     "h"
#define S_MINUTES   "min"
#define S_CHARGING  "ładowanie"
#define S_FULL_BAT  "bateria pełna"
#define S_PRESS_ANY "naciśnij dowolny klawisz"
#define S_CHARGED   "Pełny"
#define S_NO_USB    "Brak USB"

/* alarm */
#define S_STEP1 "Wejdź do środka"
#define S_STEP2 "Zamknij drzwi i okna"
#define S_STEP3 "Słuchaj wiadomości"

/* settings */
#define S_THEME      "Motyw"
#define S_BRIGHTNESS "Jasność"
#define S_SOUND      "Dźwięk alarmu"
#define S_VIBRATION  "Wibracja alarmu"
#define S_LANGUAGE   "Język"
#define S_ATT_TH     "Poziom zagrożenia"
#define S_CLICK      "Licznik kliknięć"
#define S_COUNT_VIBE "Licznik wibracji"
#define S_USB_START  "Start z zasilania USB"
#define S_LED_COLOR  "Kolor LED"
#define LED_COLOR_NAMES {"Temat", "Czerwony", "Zielony", "Niebieski", "Żółty", "Cyjan", "Magenta", "Biały"}
#define S_LED_PULSE  "Licznik LED"
#define S_LED_CHARGE "Ładowanie LED"
#define S_TIME_ZONE  "Strefa czasu"
#define S_TH_ATTENTION "Uwaga"
#define S_TH_DANGER    "Zagrożenie"
#define S_TH_EVACUATE  "Ewakuować"
#define S_SOUND_LIGHT  "Dźwięk · wibracja · światło"
#define S_TURKISH    "Turecki"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Ciemny", "Jasny", "Papier", "Bursztyn", "Sakura", "Noc", "Terminal", "Ocean", "Las", "Kontrast", "Lawenda", "Mięta", "Zachód", "Węgiel", "Piasek", "Lód", "Wiśnia", "Fiolet", "Cukierki", "Mono", "Karmazyn"}
#define S_THEME_DARK   "Ciemny"
#define S_THEME_LIGHT  "Jasny"
#define S_THEME_PAPER  "Papier"
#define S_THEME_AMBER  "Bursztyn"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profil"
#define S_P_DETAIL   "Szczegółowe"
#define S_P_DENSE    "Gęsty"
#define S_P_BALANCED "Średni"
#define S_P_LONG     "Długi"
#define S_P_ARCHIVE  "Archiwum"
#define S_DAYS       "dni"
#define S_MONTHS     "miesiące"
#define S_YEARS      "lata"
#define S_OLDEST     "Najstarszy zapis"
#define S_WHEN_FULL  "Gdy pełne"
#define S_OVERWRITE  "Nadpisz"
#define S_FULL_PCT   "Pełne"
#define S_FREE       "wolny"
#define S_RECORDED   "dni zapisów"
#define S_ALARM_LOG  "Logi alarmów"
#define S_AUTO_NOTE  "Momenty alarmowe zapisywane są też co sekundę."
#define S_USB_DOWNLOAD "USB: pobierz zapisy"

/* sample test, budget, scan */
#define S_BACKGROUND "Tło"
#define S_SAMPLE     "Próbka"
#define S_NET_DIFF   "Różnica netto"
#define S_RESULT     "Wynik"
#define S_HIGHER     "Wyraźnie powyżej tła"
#define S_SAME       "Tak jak tło"
#define S_MEASURE_TIME "czas"
#define S_GOAL       "cel"
#define S_MISSION    "Budżet"
#define S_START      "Rozpoczęto"
#define S_DURATION   "Czas trwania"
#define S_LEFT       "Pozostało"
#define S_USED_PCT   "Zużyte"
#define S_ON_EXCEED  "Alarm: limit"
#define S_NEW_BUDGET "Budżet"
#define S_FIND_SRC   "Im bliżej źródła, tym wyższy ton i wyższe paski."
#define S_APPROACH   "Im bliżej"
#define S_SOUND_ON   "Dźwięk włącz."
#define S_SCAN_SOUND "Dźwięk lokalizatora"
#define S_MODE_NAV   "Tryb"
#define S_EXIT       "Wyj."

/* tube, self test, usb */
#define S_HV         "Wysokie napięcie"
#define S_HV_FACTORY "Wartość fabryczna"
#define S_TOTAL_PULSES "Łączne impulsy"
#define S_BG_RATE    "Tło"
#define S_DEADTIME   "Korekta czasu martwego"
#define S_HV_PROTECTED "Ochrona HV"
#define S_CHK_HV      "Wysokie napięcie"
#define S_CHK_TUBE    "Impulsy rurki"
#define S_CHK_BAT     "Odczyt baterii"
#define S_CHK_E       "Czujnik elektryczny"
#define S_CHK_H       "Czujnik magnetyczny"
#define S_CHK_CLOCK   "Zegar"
#define S_CHK_MEM     "Pamięć logu"
#define S_USB_STATE   "POŁĄCZONY"
#define S_STREAM      "Transmisja na żywo"
#define S_CHARGE_A    "Ładowanie"
#define S_CLOCK_SYNC  "Zegar"
#define S_SYNCED      "Zsynk."
#define S_RECORDS     "Zapisy"
#define S_TRANSFER    "Transfer"
#define S_ALL_ON_PC   "Wszystkie zapisy na PC"
#define S_DOWNLOAD_NOW "Pobierz teraz"
#define S_AUTO_ON     "Auto: Włączony"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Zapisy"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Co robić w sytuacji awaryjnej jądrowej.", "Lista przeszłych zdarzeń wysokiej dawki.", \
    "Pokazuje, ile z wybranej dawki zostało zużyte.", "Oddzielne zapisy dawki dla wycieczki lub spaceru.", \
    "Sprawdza, czy obiekt emituje promieniowanie.", "Lokalizuje przewody i źródła pola za pomocą dźwięku i pasków.", \
    "Czas nagrywania i stan pamięci.", "Ustaw poziomy ostrzeżenia i alarmu.", \
    "Stan rurki, czas martwy, napięcie wysokie.", "Sprawdza części urządzenia po kolei.", \
    "Połącz z komputerem i prześlij zapisy.", "Wybierz motyw kolorystyczny ekranu.", \
    "Jasność, dźwięk, wibracje i zegar.", "Ustaw czułość i współczynniki pola ręcznie.", \
    "Ustawia czułość znanym źródłem.", "Wersja i linki do projektów.", \
    "Mierzy czas martwy rurki przy użyciu dwóch źródeł."}
#define SETTINGS_DESC {"Wybierz motyw kolorystyczny ekranu.", "Przyciemniony ekran wydłuża czas pracy baterii.", \
    "Czy alarmy i ostrzeżenia mają wydawać dźwięk.", "Czy urządzenie ma wibrować podczas alarmu.", \
    "Klik przy każdym odliczeniu rurki, jak w klasycznych licznikach Geigera.", "Silnik wydaje krótki dźwięk przy każdym odliczeniu.", \
    "LED miga przy każdym odliczeniu. Wyłączenie oszczędza energię.", "LED świeci podczas ładowania baterii.", \
    "Kolor LED licznika i ładowania. Temat podąża za kolorem akcentu ekranu.", \
    "Czy urządzenie ma się włączać po podłączeniu zasilania USB. Jeśli nie, przytrzymaj klawisz zasilania.", \
    "Jaka strefa czasowa jest wyświetlana na zegarze.", "Ustaw czas i datę ręcznie.", \
    "Gdy urządzenie bezczynne, pokazuje duży zegar i poziom promieniowania zamiast wyłączać ekran.", \
    "Stale podłączone: ekran gaśnie po 30 s, USB restart po zawieszeniu."}
/* sessions */
#define S_T_SESSIONS   "Sesje"
#define S_SESS_NEW     "Rozpocznij nową sesję"
#define S_SESS_RUNNING "Trwająca sesja"
#define S_SESS_NONE    "Brak zapisanych sesji."
#define S_SESS_PICK    "Wybierz typ sesji"
#define S_SESS_HINT    "Podczas sesji rejestrowane są dawka, czas i najwyższe natężenie. Kontynuuje to działanie po wyłączeniu urządzenia."
#define SESS_KINDS_NAMES  {"Wycieczka / Lot", "Spacer", "Pomiar wewnątrz", "Na zewnątrz", "Wokół próbki", "Ogólne"}
#define S_SESS_DOSE    "Dawka przyjęta"
#define S_SESS_TIME    "Czas trwania"
#define S_SESS_AVG     "Średnia"
#define S_SESS_MAX     "Najwyższa"
#define S_SESS_NOW     "Teraz"
#define S_SESS_ASK     "Naciśnij OK, by zakończyć"
#define S_FINISH       "Zakończ"
#define S_BEGIN        "Rozpocznij"
#define S_APPLY        "Zastosuj"
#define S_DISCARD      "Odrzuć"
#define S_BROWSE       "Przeglądaj"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Dni naturalnej dawki"
#define S_NATURAL_LESS "Mniej niż jeden dzień naturalnej dawki"
/* calibration wizard */
#define S_CW_T0        "Referencyjna dawka"
#define S_CW_T1        "Pomiar"
#define S_CW_T2        "Wynik"
#define S_CW_HELP0     "Umieść Yarrow obok źródła o znanym tempie dawki: wartości wyświetlanej przez inny sprzęt lub certyfikowanego źródła. Wprowadź tę wartość."
#define S_CW_COUNTS    "Liczby"
#define S_CW_HINT1     "Błąd ok. 3% przy 1000 liczbach. Dłuższy pomiar jest dokładniejszy."
#define S_CW_SAT       "Rurka jest zbyt zajęta: oddal źródło."
#define S_CW_NEEDMORE  "Potrzebne jest co najmniej 100 liczby do zakończenia."
#define S_CW_OLD       "Aktualny"
#define S_CW_NEW       "Nowy"
#define S_CW_DIFF      "Zmień"
#define S_CW_UNC       "Niepewność"
#define S_CW_RANGE     "Wartość poza zakresem: zmierz ponownie."
#define S_M_GUIDE    "Poradnik awaryjny"
#define S_M_EVENTS   "Rejestr zdarzeń"
#define S_M_BUDGET   "Budżet dawki"
#define S_M_SAMPLE   "Test próbki"
#define S_M_SCAN     "Lokalizator pola i kabla"
#define S_M_SESSIONS "Sesje"
#define S_M_CALWIZ   "Asystent kalibracji"
#define S_M_LOG      "Log i pamięć"
#define S_M_THRESH   "Poziomy alarmu"
#define S_M_TUBE     "Stan rurki"
#define S_M_SELFTEST "Samotest"
#define S_M_USB      "Połączenie PC"
#define S_M_SETTINGS "Ustawienia"
#define S_M_THEMES   "Tematy"
#define S_M_ABOUT    "Informacje"
#define S_M_CALIB    "Kalibracja"
#define S_OPEN_SOURCE "Otwarty kod źródłowy · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Pozostań w środku", \
     {"Wejdź do najbliższego budynku, najlepiej do piwnicy.", \
      "Zamknij drzwi, okna i wentylację.", \
      "Wyłącz klimatyzację i wentylatory.", \
      "Słuchaj komunikatów w radiu lub telefonie.", \
      "Nie wychodź, dopóki władze nie zezwolą.", 0, 0}}, \
    {"Jeśli byłeś na zewnątrz", \
     {"Zdejmij ubrania zewnętrzne i zapakuj je w worek. To usunie większość zanieczyszczeń.", \
      "Prysznic z mydłem i wodą, umyj włosy.", \
      "Przemyj oczy i rany czystą wodą.", \
      "Nie szoruj ani nie podrażniaj skóry.", 0, 0, 0}}, \
    {"Woda i żywność", \
     {"Żywność i napoje w szczelnych opakowaniach są bezpieczne.", \
      "Nie jedz otwartej żywności na zewnątrz ani warzyw ogrodowych.", \
      "Poczekaj na informację władz o wodzie z kranu.", \
      "Umyj ręce przed jedzeniem.", 0, 0, 0}}, \
    {"Jodek potasu", \
     {"Przyjmij tylko, gdy polecą to władze.", \
      "Chroni tarczycę tylko przed radioaktywnym jodem.", \
      "W przypadku ciąży i dzieci skonsultuj dawkę z lekarzem lub farmaceutą.", \
      "Nie zaczynaj przyjmować samodzielnie.", 0, 0, 0}}, \
    {"Informacje", \
     {"Słuchaj radia (FM/AM) i komunikatów telefonicznych.", \
      "Numer alarmowy: 112.", \
      "Może wystąpić awaria zasilania: miej pod ręką radio na baterie i naładowany telefon.", 0, 0, 0, 0}}, \
    {"Tabela dawek", \
     {"Naturalne tło: ok. 0.1 µSv/h, 2.4 mSv rocznie.", \
      "Limit dla ludności: 1 mSv rocznie.", \
      "RTG klatki piersiowej: 0.1 mSv.", \
      "TK: 2–10 mSv.", \
      "Pracownik narażony: 20 mSv rocznie.", \
      "1000 mSv lub więcej naraz: choroba promieniowania.", 0}}, \
    {"Poziomy alarmowe", \
     {"Ostrzeżenie: wzrost powyżej tła.", \
      "Niebezpieczeństwo: przejdź do środka i przeczytaj przewodnik.", \
      "Ewakuacja: postępuj zgodnie z oficjalnymi instrukcjami.", \
      "Możesz zmienić poziomy w Ustawieniach.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Etap "
#define S_HELD_BACK    "Wstrzymano"
#define S_RETRY        "Ponów"
#define S_SAVER        "Wygaszacz ekranu"
#define S_STATION      "Tryb stacji"
#define S_H_TITLE       "Zdrowie"
#define S_H_STARTS     "Starty"
#define S_H_THIS       "Ten start"
#define S_H_WDT        "Resety Watchdog'a"
#define S_H_CRASHES    "Awarie"
#define S_H_USB        "Uruchomienia USB"
#define S_H_RUNNING    "Działanie"
#define S_H_STARTUP    "Uruchamianie"
#define S_H_STAGE      "etap"
#define S_H_LASTCRASH  "Ostatnia awaria o"
#define S_M_DEADTIME   "Wizjer czasu martwego"
#define S_DT_A         "Tylko źródło A"
#define S_DT_AB        "Źródła A i B razem"
#define S_DT_B         "Tylko źródło B"
#define S_DT_NONE      "Brak żadnego źródła"
#define S_DT_INTRO     "Wymagane są dwa źródła o podobnej sile, każde dające min. 500 liczników/s, oraz rurka w miejscu. Cztery pomiary po 2 minuty: A, A+B, B i brak."
#define S_DT_WARN      "Bez dwóch takich źródeł wynik nie jest wiarygodny, co sygnalizuje asystent."
#define S_DT_PLACE     "Umieść źródło lub źródła wymienione powyżej przed rurką, a następnie uruchom. Nie ruszaj niczego podczas pomiaru."
#define S_DT_NORES     "Brak użytecznego wyniku."
#define S_DT_NORES_D   "Źródła były zbyt słabe lub jeden się poruszył. Różnica między licznikami nie była wyraźnie większa niż szum."
#define H_REASONS {"nieznane", "włączanie", "pin resetu", "watchdog", "watchdog okna", "oprogramowanie", "niski prąd", "awaria"}
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "Ostatnie 7 dni"
#define S_PEAK_WORD    "Szczyt "
#define S_MILLION      " milion"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 godz"}
#define L_WEEKDAYS     {"Pn", "Wt", "Śr", "Cz", "Pt", "So", "Nd"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " godz"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paź", "lis", "gru"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Offline"
