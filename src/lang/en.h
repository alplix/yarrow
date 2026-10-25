/* User interface text: English. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "CAUTION"
#define S_DANGER    "DANGER"
#define S_EVACUATE  "EVACUATE"
#define S_LOW       "LOW"
#define S_MEDIUM    "MEDIUM"
#define S_HIGH      "HIGH"

/* titles */
#define S_T_RADIATION "Radiation"
#define S_T_LIVE      "Live"
#define S_T_HISTORY   "History"
#define S_T_EFIELD    "Electric field"
#define S_T_HFIELD    "Magnetic field"
#define S_T_BATTERY   "Battery & device"
#define S_T_GUIDE     "Emergency guide"
#define S_T_SETTINGS  "Settings"
#define S_T_EVENTS    "Events"
#define S_T_BUDGET    "Dose budget"
#define S_T_SAMPLE    "Sample"
#define S_T_LOG       "Log"
#define S_T_STORAGE   "Storage"
#define S_T_THRESH    "Alarm levels"
#define S_T_TUBE      "Tube status"
#define S_T_SELFTEST  "Self test"
#define S_T_USB       "PC link"
#define S_T_MODE      "Measuring mode"
#define S_T_SCAN      "Field finder"
#define S_T_MENU      "Menu"
#define S_T_THEMES    "Themes"
#define S_T_ABOUT     "About"
#define S_T_CLOCK     "Time & date"
#define S_T_CALIB     "Calibration"
#define S_CLOCK_DATE  "Time & date"
#define S_DAY    "Day"
#define S_MONTH  "Month"
#define S_YEAR   "Year"
#define S_HOUR   "Hour"
#define S_MINUTE "Minute"
#define S_NEXT   "next"
#define S_SAVE   "save"
#define S_CAL_SENS   "Sensitivity"
#define S_CAL_DEAD   "Dead time"
#define S_CAL_E      "Electric factor"
#define S_CAL_H      "Magnetic factor"
#define S_CAL_NOTE   "Adjust the device by comparing it with a known source."

/* labels */
#define S_TODAY     "Today"
#define S_WEEK      "Week"
#define S_TOTAL     "Total"
#define S_MIN       "Min"
#define S_AVG       "Avg"
#define S_MAX       "Max"
#define S_PEAK      "Peak"
#define S_AVERAGE   "Average"
#define S_NOW       "now"
#define S_REC_ON    "recording"
#define S_BAT_LEFT  "battery"
#define S_LAST_10   "last 10 min"
#define S_NORMAL_x  "of normal"
#define S_TIMES     "times"
#define S_THRESHOLD "level"
#define S_SCALE     "scale"
#define S_DETAILS   "details"
#define S_OPEN      "open"
#define S_BACK      "back"
#define S_SELECT    "select"
#define S_MUTE      "mute"
#define S_GUIDE     "guide"
#define S_CHANGE    "change"
#define S_RESET     "reset"
#define S_FINISH    "finish"
#define S_CANCEL    "cancel"
#define S_ON        "on"
#define S_OFF       "off"
#define S_YES       "yes"
#define S_NO        "no"
#define S_DONE      "done"

/* battery screen */
#define S_REMAINING "Time left"
#define S_HEALTH    "Battery health"
#define S_CYCLES    "Charge cycles"
#define S_RUNTIME   "Total run time"
#define S_TUBE_LIFE "Tube life"
#define S_HOURS     "h"
#define S_MINUTES   "min"
#define S_CHARGING  "charging"
#define S_FULL_BAT  "battery full"
#define S_PRESS_ANY "press any key"
#define S_CHARGED   "full"
#define S_NO_USB    "no USB"

/* alarm */
#define S_STEP1 "Go inside"
#define S_STEP2 "Close doors and windows"
#define S_STEP3 "Listen to the news"

/* settings */
#define S_THEME      "Theme"
#define S_BRIGHTNESS "Brightness"
#define S_SOUND      "Alarm sound"
#define S_VIBRATION  "Alarm vibration"
#define S_LANGUAGE   "Language"
#define S_ATT_TH     "Caution level"
#define S_CLICK      "Count click"
#define S_COUNT_VIBE "Count vibration"
#define S_USB_START  "Start on USB power"
#define S_LED_COLOR  "LED colour"
#define LED_COLOR_NAMES {"Theme", "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White"}
#define S_LED_PULSE  "Count LED"
#define S_LED_CHARGE "Charge LED"
#define S_TIME_ZONE  "Time zone"
#define S_TH_ATTENTION "Caution"
#define S_TH_DANGER    "Danger"
#define S_TH_EVACUATE  "Evacuate"
#define S_SOUND_LIGHT  "sound · vibration · light"
#define S_TURKISH    "Turkish"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Dark", "Light", "Paper", "Amber", "Sakura", "Night", "Terminal", "Ocean", "Forest", "Contrast", "Lavender", "Mint", "Sunset", "Charcoal", "Sand", "Ice", "Cherry", "Violet", "Candy", "Mono", "Crimson"}
#define S_THEME_DARK   "Dark"
#define S_THEME_LIGHT  "Light"
#define S_THEME_PAPER  "Paper"
#define S_THEME_AMBER  "Amber"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profile"
#define S_P_DETAIL   "Detailed"
#define S_P_DENSE    "Dense"
#define S_P_BALANCED "Medium"
#define S_P_LONG     "Long"
#define S_P_ARCHIVE  "Archive"
#define S_DAYS       "days"
#define S_MONTHS     "months"
#define S_YEARS      "years"
#define S_OLDEST     "Oldest record"
#define S_WHEN_FULL  "When full"
#define S_OVERWRITE  "overwrite"
#define S_FULL_PCT   "full"
#define S_FREE       "free"
#define S_RECORDED   "days of records"
#define S_ALARM_LOG  "Alarm records"
#define S_AUTO_NOTE  "Alarm moments are also saved every second."
#define S_USB_DOWNLOAD "USB: download records"

/* sample test, budget, scan */
#define S_BACKGROUND "Background"
#define S_SAMPLE     "Sample"
#define S_NET_DIFF   "Net difference"
#define S_RESULT     "Result"
#define S_HIGHER     "Clearly above background"
#define S_SAME       "Same as background"
#define S_MEASURE_TIME "time"
#define S_GOAL       "goal"
#define S_MISSION    "Budget"
#define S_START      "Started"
#define S_DURATION   "Duration"
#define S_LEFT       "Left"
#define S_USED_PCT   "used"
#define S_ON_EXCEED  "over limit: alarm"
#define S_NEW_BUDGET "new budget"
#define S_FIND_SRC   "The closer you get to the source, the higher the tone and the taller the bars."
#define S_APPROACH   "getting closer"
#define S_SOUND_ON   "sound on"
#define S_SCAN_SOUND "finder sound"
#define S_MODE_NAV   "mode"
#define S_EXIT       "exit"

/* tube, self test, usb */
#define S_HV         "High voltage"
#define S_HV_FACTORY "factory value"
#define S_TOTAL_PULSES "Total pulses"
#define S_BG_RATE    "Background"
#define S_DEADTIME   "Dead time correction"
#define S_HV_PROTECTED "HV protected"
#define S_CHK_HV      "High voltage"
#define S_CHK_TUBE    "Tube pulses"
#define S_CHK_BAT     "Battery reading"
#define S_CHK_E       "Electric sensor"
#define S_CHK_H       "Magnetic sensor"
#define S_CHK_CLOCK   "Clock"
#define S_CHK_MEM     "Log memory"
#define S_USB_STATE   "LINKED"
#define S_STREAM      "Live stream"
#define S_CHARGE_A    "Charge"
#define S_CLOCK_SYNC  "Clock"
#define S_SYNCED      "synced"
#define S_RECORDS     "Records"
#define S_TRANSFER    "Transfer"
#define S_ALL_ON_PC   "all records on the PC"
#define S_DOWNLOAD_NOW "download now"
#define S_AUTO_ON     "automatic: on"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "records"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"What to do in a nuclear emergency.", "List of past high-dose events.", \
    "Shows how much of your chosen dose is used up.", "Separate dose records for a trip or a walk.", \
    "Checks whether an object gives off radiation.", "Finds wires and field sources by sound and bars.", \
    "Recording time and how full the memory is.", "Set the warning and alarm levels.", \
    "Tube condition, dead time, high voltage.", "Checks the parts of the device one by one.", \
    "Connect to a computer and move the records.", "Pick the colour theme of the screen.", \
    "Brightness, sound, vibration and clock.", "Set the sensitivity and field factors by hand.", \
    "Sets the sensitivity with a known source.", "Version and project links."}
#define SETTINGS_DESC {"Pick the colour theme of the screen.", "A dimmer screen makes the battery last longer.", \
    "Should alarms and warnings make a sound.", "Should the device vibrate on an alarm.", \
    "A click on every tube count, like the classic Geiger counters.", "The motor gives a short buzz on every count.", \
    "The LED flashes on every count. Switching it off saves a little power.", "The LED lights while the battery charges.", \
    "Colour of the count and charge LED. Theme follows the accent colour of the screen.", \
    "Should the device switch itself on when USB power arrives. If not, hold the power key to start it.", \
    "Which time zone the clock shows.", "Set the time and date by hand.", \
    "When idle, shows a large clock and the radiation level instead of switching the screen off.", \
    "Left plugged in: screen dark after 30 s, USB restarts itself if it hangs."}
/* sessions */
#define S_T_SESSIONS   "Sessions"
#define S_SESS_NEW     "Start a new session"
#define S_SESS_RUNNING "Running session"
#define S_SESS_NONE    "No saved sessions yet."
#define S_SESS_PICK    "Choose the session type"
#define S_SESS_HINT    "During a session the dose, time and highest rate are recorded. It keeps going when you switch the device off."
#define SESS_KINDS_NAMES  {"Trip / flight", "Walk", "Indoor survey", "Outdoors", "Around a sample", "General"}
#define S_SESS_DOSE    "Dose received"
#define S_SESS_TIME    "Duration"
#define S_SESS_AVG     "Average"
#define S_SESS_MAX     "Highest"
#define S_SESS_NOW     "Now"
#define S_SESS_ASK     "Press OK again to finish"
#define S_FINISH       "finish"
#define S_BEGIN        "start"
#define S_APPLY        "apply"
#define S_DISCARD      "discard"
#define S_BROWSE       "browse"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "days of natural dose"
#define S_NATURAL_LESS "less than a day of natural dose"
/* calibration wizard */
#define S_CW_T0        "Reference dose rate"
#define S_CW_T1        "Measuring"
#define S_CW_T2        "Result"
#define S_CW_HELP0     "Put Yarrow next to a source whose dose rate you know: the value another device shows, or a certified source. Enter that value."
#define S_CW_COUNTS    "counts"
#define S_CW_HINT1     "About 3 % error at 1000 counts. A longer measurement is more accurate."
#define S_CW_SAT       "The tube is too busy: move the source away."
#define S_CW_NEEDMORE  "At least 100 counts are needed to finish."
#define S_CW_OLD       "Current"
#define S_CW_NEW       "New"
#define S_CW_DIFF      "Change"
#define S_CW_UNC       "Uncertainty"
#define S_CW_RANGE     "Value out of range: measure again."
#define S_M_GUIDE    "Emergency guide"
#define S_M_EVENTS   "Event log"
#define S_M_BUDGET   "Dose budget"
#define S_M_SAMPLE   "Sample test"
#define S_M_SCAN     "Field & wire finder"
#define S_M_SESSIONS "Sessions"
#define S_M_CALWIZ   "Calibration wizard"
#define S_M_LOG      "Log & storage"
#define S_M_THRESH   "Alarm levels"
#define S_M_TUBE     "Tube status"
#define S_M_SELFTEST "Self test"
#define S_M_USB      "PC link"
#define S_M_SETTINGS "Settings"
#define S_M_THEMES   "Themes"
#define S_M_ABOUT    "About"
#define S_M_CALIB    "Calibration"
#define S_OPEN_SOURCE "Open source · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Stay inside", \
     {"Go into the nearest building, a basement or an inner room if you can.", \
      "Close doors, windows and ventilation.", \
      "Switch off air conditioning and fans.", \
      "Listen to official announcements on the radio or phone.", \
      "Do not go outside until the authorities say so.", 0, 0}}, \
    {"If you were outside", \
     {"Take off your outer clothes and seal them in a bag. This removes most of the contamination.", \
      "Shower with soap and water, wash your hair.", \
      "Rinse eyes and wounds with clean water.", \
      "Do not scrub or irritate the skin.", 0, 0, 0}}, \
    {"Water and food", \
     {"Food and drink in sealed packaging is safe.", \
      "Do not eat food left open outside or garden produce.", \
      "Wait for the authorities to speak about tap water.", \
      "Wash your hands before eating.", 0, 0, 0}}, \
    {"Potassium iodide", \
     {"Take it only when the authorities tell you to.", \
      "It protects the thyroid only against radioactive iodine.", \
      "For pregnancy and children, ask a doctor or pharmacist about the dose.", \
      "Do not start taking it on your own.", 0, 0, 0}}, \
    {"Get information", \
     {"Listen to radio (FM/AM) and phone announcements.", \
      "Emergency number: 112 (EU), 911 (US).", \
      "Power may fail: keep a battery radio and a charged phone ready.", 0, 0, 0, 0}}, \
    {"Dose table", \
     {"Natural background: about 0.1 µSv/h, 2.4 mSv a year.", \
      "Public limit: 1 mSv a year.", \
      "Chest X-ray: 0.1 mSv.", \
      "CT scan: 2–10 mSv.", \
      "Radiation worker: 20 mSv a year.", \
      "1000 mSv or more at once: radiation sickness.", 0}}, \
    {"Alarm levels", \
     {"Caution: a rise above background.", \
      "Danger: move indoors and read the guide.", \
      "Evacuate: follow the official instructions.", \
      "You can change the levels in Settings.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "stage "
#define S_HELD_BACK    "Held back"
#define S_RETRY        "Retry"
#define S_SAVER        "Screensaver"
#define S_STATION      "Station mode"
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "Last 7 days"
#define S_PEAK_WORD    "peak "
#define S_MILLION      " million"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 h"}
#define L_WEEKDAYS     {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " h"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}
#define L_DATE_MON_FIRST 1
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "offline"
