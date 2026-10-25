/* User interface text: Russian. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_NORMAL    "НОРМА"
#define S_ATTENTION "ОСТОРОЖНО"
#define S_DANGER    "ОПАСНОСТЬ"
#define S_EVACUATE  "ЭВАКУАЦИЯ"
#define S_LOW       "НИЗ"
#define S_MEDIUM    "СРЕДНИЙ"
#define S_HIGH      "ВЫСОКИЙ"

/* titles */
#define S_T_RADIATION "Радиация"
#define S_T_LIVE      "Живой"
#define S_T_HISTORY   "История"
#define S_T_EFIELD    "Электрическое поле"
#define S_T_HFIELD    "Магнитное поле"
#define S_T_BATTERY   "Батарея и приб."
#define S_T_GUIDE     "Руководство по ЧС"
#define S_T_SETTINGS  "Настройки"
#define S_T_EVENTS    "События"
#define S_T_BUDGET    "Бюджет дозы"
#define S_T_SAMPLE    "Образец"
#define S_T_LOG       "Журн."
#define S_T_STORAGE   "Память"
#define S_T_THRESH    "Уровни тревоги"
#define S_T_TUBE      "Статус трубки"
#define S_T_SELFTEST  "Самотест"
#define S_T_USB       "Связь с ПК"
#define S_T_MODE      "Режим измерения"
#define S_T_SCAN      "Поиск поля"
#define S_T_MENU      "Меню"
#define S_T_THEMES    "Темы"
#define S_T_ABOUT     "О программе"
#define S_T_CLOCK     "Время и дата"
#define S_T_CALIB     "Калибровка"
#define S_CLOCK_DATE  "Дата/Время"
#define S_DAY    "День"
#define S_MONTH  "Месяц"
#define S_YEAR   "Год"
#define S_HOUR   "Час"
#define S_MINUTE "Минута"
#define S_NEXT   "Далее"
#define S_SAVE   "Сохранить"
#define S_CAL_SENS   "Чувствительность"
#define S_CAL_DEAD   "Время задержки"
#define S_CAL_E      "Электрический фактор"
#define S_CAL_H      "Магнитный коэффициент"
#define S_CAL_NOTE   "Настройте прибор, сравнив его с известным источником."

/* labels */
#define S_TODAY     "Сегодня"
#define S_WEEK      "Неделя"
#define S_TOTAL     "Всего"
#define S_MIN       "Мин"
#define S_AVG       "Ср."
#define S_MAX       "Макс"
#define S_PEAK      "Пик"
#define S_AVERAGE   "Среднее"
#define S_NOW       "Сейчас"
#define S_REC_ON    "Запись"
#define S_BAT_LEFT  "Батарея"
#define S_LAST_10   "Последние 10 мин"
#define S_NORMAL_x  "нормальный"
#define S_TIMES     "раз"
#define S_THRESHOLD "уровень"
#define S_SCALE     "шкала"
#define S_DETAILS   "детали"
#define S_OPEN      "Открыть"
#define S_BACK      "назад"
#define S_SELECT    "Выб."
#define S_MUTE      "Беззвуч."
#define S_GUIDE     "Гид"
#define S_CHANGE    "Изменить"
#define S_RESET     "Сброс"
#define S_FINISH    "Завершить"
#define S_CANCEL    "Отмена"
#define S_ON        "Вкл"
#define S_OFF       "Выкл"
#define S_YES       "Да"
#define S_NO        "Нет"
#define S_DONE      "Готово"

/* battery screen */
#define S_REMAINING "Остаток времени"
#define S_HEALTH    "Состояние батареи"
#define S_CYCLES    "Циклы заряда"
#define S_RUNTIME   "Общее время работы"
#define S_TUBE_LIFE "Срок службы трубки"
#define S_HOURS     "h"
#define S_MINUTES   "мин"
#define S_CHARGING  "Зарядка"
#define S_FULL_BAT  "Батарея полная"
#define S_PRESS_ANY "Нажмите любую клавишу"
#define S_CHARGED   "полный"
#define S_NO_USB    "нет USB"

/* alarm */
#define S_STEP1 "зайдите внутрь"
#define S_STEP2 "закройте двери и окна"
#define S_STEP3 "слушайте новости"

/* settings */
#define S_THEME      "тема"
#define S_BRIGHTNESS "яркость"
#define S_SOUND      "звук тревоги"
#define S_VIBRATION  "вибрация тревоги"
#define S_LANGUAGE   "язык"
#define S_ATT_TH     "уровень опасности"
#define S_CLICK      "счет кликов"
#define S_COUNT_VIBE "счет вибраций"
#define S_USB_START  "запуск от USB питания"
#define S_LED_COLOR  "Цвет LED"
#define LED_COLOR_NAMES {"Тема", "Красный", "Зеленый", "Синий", "Желтый", "Циан", "Маджента", "Белый"}
#define S_LED_PULSE  "Счёт LED"
#define S_LED_CHARGE "Заряд LED"
#define S_TIME_ZONE  "Часовой пояс"
#define S_TH_ATTENTION "Осторожно"
#define S_TH_DANGER    "Опасно"
#define S_TH_EVACUATE  "Эвакуация"
#define S_SOUND_LIGHT  "Звук · вибрация · свет"
#define S_TURKISH    "Турецкий"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Темный", "Светлый", "Бумага", "Янтарный", "Сакура", "Ночь", "Терминал", "Океан", "Лес", "Контраст", "Лавандовый", "Мята", "Закат", "Угольный", "Песок", "Лед", "Вишня", "Фиолетовый", "Конфеты", "Моно", "Багряный"}
#define S_THEME_DARK   "Темный"
#define S_THEME_LIGHT  "Светлый"
#define S_THEME_PAPER  "Бумага"
#define S_THEME_AMBER  "Янтарный"
#define S_THEME_SAKURA "Сакура"

/* log and storage */
#define S_PROFILE    "Профиль"
#define S_P_DETAIL   "Подробный"
#define S_P_DENSE    "Плотный"
#define S_P_BALANCED "Средн."
#define S_P_LONG     "Длинный"
#define S_P_ARCHIVE  "Архив"
#define S_DAYS       "Дни"
#define S_MONTHS     "Месяцы"
#define S_YEARS      "Годы"
#define S_OLDEST     "Старейшая запись"
#define S_WHEN_FULL  "Заполн."
#define S_OVERWRITE  "Перезап"
#define S_FULL_PCT   "Полный"
#define S_FREE       "Свободно"
#define S_RECORDED   "Дней записей"
#define S_ALARM_LOG  "Тревоги"
#define S_AUTO_NOTE  "Моменты тревоги также сохраняются каждую секунду."
#define S_USB_DOWNLOAD "USB: Загрузка"

/* sample test, budget, scan */
#define S_BACKGROUND "Фон"
#define S_SAMPLE     "Образец"
#define S_NET_DIFF   "Чистая разница"
#define S_RESULT     "Результат"
#define S_HIGHER     "Значительно выше фона"
#define S_SAME       "Как фон"
#define S_MEASURE_TIME "Время измерения"
#define S_GOAL       "Цель"
#define S_MISSION    "Бюджет"
#define S_START      "Начато"
#define S_DURATION   "Продолжительность"
#define S_LEFT       "Остаток"
#define S_USED_PCT   "Использовано"
#define S_ON_EXCEED  "Лимит превышен"
#define S_NEW_BUDGET "Лимит"
#define S_FIND_SRC   "Чем ближе к источнику, тем выше тон и выше столбцы."
#define S_APPROACH   "Приближение"
#define S_SOUND_ON   "Звук включен"
#define S_SCAN_SOUND "Поисковый звук"
#define S_MODE_NAV   "Режим"
#define S_EXIT       "Выход"

/* tube, self test, usb */
#define S_HV         "Высокое напряжение"
#define S_HV_FACTORY "Заводское значение"
#define S_TOTAL_PULSES "Общие импульсы"
#define S_BG_RATE    "Фон"
#define S_DEADTIME   "Коррекция мертвого времени"
#define S_HV_PROTECTED "Защита ВЧ"
#define S_CHK_HV      "ВН"
#define S_CHK_TUBE    "Импульсы трубки"
#define S_CHK_BAT     "Показание батареи"
#define S_CHK_E       "Электросенсор"
#define S_CHK_H       "Магнитный датчик"
#define S_CHK_CLOCK   "Часы"
#define S_CHK_MEM     "Память лога"
#define S_USB_STATE   "СВЯЗАН"
#define S_STREAM      "Живой поток"
#define S_CHARGE_A    "Заряд"
#define S_CLOCK_SYNC  "Часы"
#define S_SYNCED      "Синхр."
#define S_RECORDS     "Записи"
#define S_TRANSFER    "Перенос"
#define S_ALL_ON_PC   "Все записи на ПК"
#define S_DOWNLOAD_NOW "Скачать сейчас"
#define S_AUTO_ON     "Авто: Вкл"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Записи"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Что делать при ядерной аварии.", "Список прошлых событий с высокой дозой.", \
    "Показывает, сколько из выбранной дозы использовано.", "Отдельные записи дозы для поездки или прогулки.", \
    "Проверяет, излучает ли объект радиацию.", "Находит провода и источники поля по звуку и шкале.", \
    "Время записи и заполненность памяти.", "Установить уровни предупреждения и тревоги.", \
    "Состояние трубки, время задержки, высокое напряжение.", "Проверяет компоненты устройства по очереди.", \
    "Подключите к компьютеру и перенесите записи.", "Выберите цветовую тему экрана.", \
    "Яркость, звук, вибрация и часы.", "Установить чувствительность и полевые факторы вручную.", \
    "Устанавливает чувствительность с известным источником.", "Версия и ссылки на проекты."}
#define SETTINGS_DESC {"Выберите цветовую тему экрана.", "Приглушенный экран продлевает срок службы батареи.", \
    "Должны ли звучать тревоги и предупреждения.", "Должно ли устройство вибрировать при тревоге.", \
    "Клик при каждом импульсе счетчика, как у классических счетчиков Гейгера.", "Двигатель издает короткий звук при каждом импульсе.", \
    "Светодиод мигает при каждом импульсе. Отключение экономит энергию.", "Светодиод горит во время зарядки батареи.", \
    "Цвет светодиодов счета и зарядки. Тема следует акцентному цвету экрана.", \
    "Включать ли устройство при подаче питания USB. Если нет, нажмите и удерживайте кнопку питания.", \
    "Выбор часового пояса.", "Установить время и дату вручную."}
/* sessions */
#define S_T_SESSIONS   "Сессии"
#define S_SESS_NEW     "Начать новую сессию"
#define S_SESS_RUNNING "Активная сессия"
#define S_SESS_NONE    "Сессии не сохранены."
#define S_SESS_PICK    "Выберите тип сессии"
#define S_SESS_HINT    "Во время сессии записываются доза, время и максимальная скорость. Запись продолжается после выключения устройства."
#define SESS_KINDS_NAMES  {"Полет / Поездка", "Прогулка", "Внутренний осмотр", "На улице", "Вокруг образца", "Общий"}
#define S_SESS_DOSE    "Полученная доза"
#define S_SESS_TIME    "Продолжительность"
#define S_SESS_AVG     "Среднее"
#define S_SESS_MAX     "Макс."
#define S_SESS_NOW     "Сейчас"
#define S_SESS_ASK     "Нажмите OK для завершения"
#define S_FINISH       "Завершить"
#define S_BEGIN        "Начать"
#define S_APPLY        "Применить"
#define S_DISCARD      "Отбросить"
#define S_BROWSE       "Просмотр"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Дней естественной дозы"
#define S_NATURAL_LESS "Менее одного дня естественной дозы"
/* calibration wizard */
#define S_CW_T0        "Референтная скорость дозы"
#define S_CW_T1        "Измерение"
#define S_CW_T2        "Результат"
#define S_CW_HELP0     "Поместите Яровь рядом с источником, доза которого известна (значение другого прибора или сертифицированный источник). Введите это значение."
#define S_CW_COUNTS    "Счётчики"
#define S_CW_HINT1     "Ошибка около 3% при 1000 счётах. Более длительное измерение точнее."
#define S_CW_SAT       "Трубка перегружена: удалите источник."
#define S_CW_NEEDMORE  "Для завершения требуется минимум 100 счётов."
#define S_CW_OLD       "Ток"
#define S_CW_NEW       "Новый"
#define S_CW_DIFF      "Изменить"
#define S_CW_UNC       "Погрешность"
#define S_CW_RANGE     "Значение вне диапазона: измерить снова."
#define S_M_GUIDE    "Аварийный гид"
#define S_M_EVENTS   "Журнал событий"
#define S_M_BUDGET   "Бюджет дозы"
#define S_M_SAMPLE   "Тест образца"
#define S_M_SCAN     "Поиск поля и провода"
#define S_M_SESSIONS "Сессии"
#define S_M_CALWIZ   "Мастер калибровки"
#define S_M_LOG      "Журнал и хранение"
#define S_M_THRESH   "Уровни тревоги"
#define S_M_TUBE     "Статус трубки"
#define S_M_SELFTEST "Самотест"
#define S_M_USB      "ПК-связь"
#define S_M_SETTINGS "Настройки"
#define S_M_THEMES   "Темы"
#define S_M_ABOUT    "О программе"
#define S_M_CALIB    "Калибровка"
#define S_OPEN_SOURCE "Открытый исходный код · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Оставайтесь дома", \
     {"Зайдите в ближайшее здание, подвал или внутреннюю комнату, если возможно.", \
      "Закройте двери, окна и вентиляцию.", \
      "Выключите кондиционер и вентиляторы.", \
      "Слушайте объявления (радио/тел.)", \
      "Не выходите на улицу, пока не объявят власти.", 0, 0}}, \
    {"Если вы были на улице", \
     {"Снимите верхнюю одежду и упакуйте ее в пакет. Это удалит большую часть загрязнения.", \
      "Примите душ с мылом и водой, вымойте волосы.", \
      "Промойте глаза и раны чистой водой.", \
      "Не трите и не раздражайте кожу.", 0, 0, 0}}, \
    {"Вода и еда", \
     {"Пища и напитки в герметичной упаковке безопасны.", \
      "Не ешьте открытую еду на улице или садовые продукты.", \
      "Дождитесь заявления властей о водопроводной воде.", \
      "Вымойте руки перед едой.", 0, 0, 0}}, \
    {"Йодид калия", \
     {"Принимайте только по указанию властей.", \
      "Он защищает щитовидную железу только от радиоактивного йода.", \
      "При беременности и для детей проконсультируйтесь с врачом или фармацевтом о дозе.", \
      "Не начинайте принимать его самостоятельно.", 0, 0, 0}}, \
    {"Получить информацию", \
     {"Слушайте радио (FM/AM) и объявления по телефону.", \
      "Номер экстренных служб: 112.", \
      "Может отключиться питание: держите под рукой радио на батарейках и заряженный телефон.", 0, 0, 0, 0}}, \
    {"Таблица доз", \
     {"Естественный фон: около 0.1 µSv/h, 2.4 mSv в год.", \
      "Предел для населения: 1 mSv в год.", \
      "Рентген грудной клетки: 0.1 mSv.", \
      "КТ-сканирование: 2–10 mSv.", \
      "Радиационный работник: 20 mSv в год.", \
      "1000 mSv или более за раз: лучевая болезнь.", 0}}, \
    {"Уровни тревоги", \
     {"Осторожно: повышение выше фона.", \
      "Опасно: зайдите в помещение и прочитайте руководство.", \
      "Эвакуируйтесь: следуйте официальным инструкциям.", \
      "Уровни можно изменить в Настройках.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Стадия "
#define S_HELD_BACK    "Отключено"
#define S_RETRY        "Повтор"
#define S_AXIS_10MIN   "-10 мин"
#define S_AXIS_60S     "-60 с"
#define S_T_LAST7      "Последние 7 дней"
#define S_PEAK_WORD    "Пик "
#define S_MILLION      " Миллион"
#define L_PROFILE_STEPS {"1 с", "10 с", "1 мин", "5 мин", "1 ч"}
#define L_WEEKDAYS     {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " ч"
#define L_MIN_UNIT     " мин"
#define L_MONTHS       {"янв", "фев", "мар", "апр", "мая", "июн", "июл", "авг", "сен", "окт", "ноя", "дек"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Офлайн"
