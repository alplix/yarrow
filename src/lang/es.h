/* User interface text: Spanish. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "PRECAUCIÓN"
#define S_DANGER    "PELIGRO"
#define S_EVACUATE  "EVACUAR"
#define S_LOW       "BAJO"
#define S_MEDIUM    "MEDIO"
#define S_HIGH      "ALTO"

/* titles */
#define S_T_RADIATION "Radiación"
#define S_T_LIVE      "En vivo"
#define S_T_HISTORY   "Historial"
#define S_T_EFIELD    "Campo eléctrico"
#define S_T_HFIELD    "Campo magnético"
#define S_T_BATTERY   "Batería"
#define S_T_GUIDE     "Guía de emergencia"
#define S_T_SETTINGS  "Configuración"
#define S_T_EVENTS    "Eventos"
#define S_T_BUDGET    "Dosis límite"
#define S_T_SAMPLE    "Muestra"
#define S_T_LOG       "Reg."
#define S_T_STORAGE   "Almacenamiento"
#define S_T_THRESH    "Niveles de alarma"
#define S_T_TUBE      "Estado del tubo"
#define S_T_SELFTEST  "Autocomprobación"
#define S_T_USB       "Enlace PC"
#define S_T_MODE      "Modo de medición"
#define S_T_SCAN      "Buscar campo"
#define S_T_MENU      "Menú"
#define S_T_THEMES    "Temas"
#define S_T_ABOUT     "Info"
#define S_T_CLOCK     "Hora y fecha"
#define S_T_CALIB     "Calibración"
#define S_CLOCK_DATE  "Hora y fecha"
#define S_DAY    "Día"
#define S_MONTH  "Mes"
#define S_YEAR   "Año"
#define S_HOUR   "Hora"
#define S_MINUTE "Minuto"
#define S_NEXT   "Siguiente"
#define S_SAVE   "Guardar"
#define S_CAL_SENS   "Sensibilidad"
#define S_CAL_DEAD   "Tiempo muerto"
#define S_CAL_E      "Factor eléctrico"
#define S_CAL_H      "Factor magnético"
#define S_CAL_NOTE   "Ajuste el dispositivo comparándolo con una fuente conocida."

/* labels */
#define S_TODAY     "Hoy"
#define S_WEEK      "Semana"
#define S_TOTAL     "Total"
#define S_MIN       "Mín."
#define S_AVG       "Prom."
#define S_MAX       "Máx."
#define S_PEAK      "Pico"
#define S_AVERAGE   "Promedio"
#define S_NOW       "Ahora"
#define S_REC_ON    "Grabando"
#define S_BAT_LEFT  "Batería"
#define S_LAST_10   "Últimos 10 min"
#define S_NORMAL_x  "normal"
#define S_TIMES     "veces"
#define S_THRESHOLD "nivel"
#define S_SCALE     "escala"
#define S_DETAILS   "detalles"
#define S_OPEN      "abrir"
#define S_BACK      "atrás"
#define S_SELECT    "Selec."
#define S_MUTE      "silenciar"
#define S_GUIDE     "guía"
#define S_CHANGE    "cambiar"
#define S_RESET     "reiniciar"
#define S_FINISH    "finalizar"
#define S_CANCEL    "cancelar"
#define S_ON        "Enc."
#define S_OFF       "Apag."
#define S_YES       "Sí"
#define S_NO        "No"
#define S_DONE      "Listo"

/* battery screen */
#define S_REMAINING "Tiempo rem."
#define S_HEALTH    "Salud de la batería"
#define S_CYCLES    "Ciclos de carga"
#define S_RUNTIME   "Tiempo total"
#define S_TUBE_LIFE "Vida del tubo"
#define S_HOURS     "h"
#define S_MINUTES   "min"
#define S_CHARGING  "Cargando"
#define S_FULL_BAT  "Batería llena"
#define S_PRESS_ANY "Presione tecla"
#define S_CHARGED   "Completo"
#define S_NO_USB    "Sin USB"

/* alarm */
#define S_STEP1 "Entrar"
#define S_STEP2 "Cierre puertas y ventanas"
#define S_STEP3 "Escuchar noticias"

/* settings */
#define S_THEME      "Tema"
#define S_BRIGHTNESS "Brillo"
#define S_SOUND      "Sonido alarma"
#define S_VIBRATION  "Vibración alarma"
#define S_LANGUAGE   "Idioma"
#define S_ATT_TH     "Nivel de alerta"
#define S_CLICK      "Conteo clic"
#define S_COUNT_VIBE "Vibración de conteo"
#define S_USB_START  "Arrancar con USB"
#define S_LED_COLOR  "Color LED"
#define LED_COLOR_NAMES {"Tema", "Rojo", "Verde", "Azul", "Amarillo", "Cian", "Magenta", "Blanco"}
#define S_LED_PULSE  "Conteo LED"
#define S_LED_CHARGE "Carga LED"
#define S_TIME_ZONE  "Zona horaria"
#define S_TH_ATTENTION "Precaución"
#define S_TH_DANGER    "Peligro"
#define S_TH_EVACUATE  "Evacuar"
#define S_SOUND_LIGHT  "Sonido · vibración · luz"
#define S_TURKISH    "Turco"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Oscuro", "Claro", "Papel", "Ámbar", "Sakura", "Noche", "Terminal", "Océano", "Bosque", "Contraste", "Lavanda", "Menta", "Atardecer", "Carbón", "Arena", "Hielo", "Cereza", "Violeta", "Dulce", "Mono", "Carmesí"}
#define S_THEME_DARK   "Oscuro"
#define S_THEME_LIGHT  "Claro"
#define S_THEME_PAPER  "Papel"
#define S_THEME_AMBER  "Ámbar"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Perfil"
#define S_P_DETAIL   "Detalle"
#define S_P_DENSE    "Denso"
#define S_P_BALANCED "Medio"
#define S_P_LONG     "Largo"
#define S_P_ARCHIVE  "Archivo"
#define S_DAYS       "días"
#define S_MONTHS     "meses"
#define S_YEARS      "años"
#define S_OLDEST     "Registro más viejo"
#define S_WHEN_FULL  "Lle"
#define S_OVERWRITE  "Sobrescr"
#define S_FULL_PCT   "Lle"
#define S_FREE       "libre"
#define S_RECORDED   "días de registros"
#define S_ALARM_LOG  "Reg. alarma"
#define S_AUTO_NOTE  "Los momentos de alarma también se guardan cada segundo."
#define S_USB_DOWNLOAD "USB: Descargar reg."

/* sample test, budget, scan */
#define S_BACKGROUND "fondo"
#define S_SAMPLE     "muestra"
#define S_NET_DIFF   "diferencia neta"
#define S_RESULT     "resultado"
#define S_HIGHER     "Muy visible"
#define S_SAME       "Igual que el fondo"
#define S_MEASURE_TIME "tiempo de medición"
#define S_GOAL       "meta"
#define S_MISSION    "límite"
#define S_START      "Iniciado"
#define S_DURATION   "Duración"
#define S_LEFT       "Restante"
#define S_USED_PCT   "Usado"
#define S_ON_EXCEED  "Excede: alarma"
#define S_NEW_BUDGET "Nuevo límite"
#define S_FIND_SRC   "Más cerca de la fuente: tono más agudo y barras más altas."
#define S_APPROACH   "Acercándose"
#define S_SOUND_ON   "Sonido encendido"
#define S_SCAN_SOUND "Sonido buscador"
#define S_MODE_NAV   "Modo"
#define S_EXIT       "Salir"

/* tube, self test, usb */
#define S_HV         "Alto voltaje"
#define S_HV_FACTORY "Valor de fábrica"
#define S_TOTAL_PULSES "Pulso total"
#define S_BG_RATE    "Fondo"
#define S_DEADTIME   "Corrección de tiempo muerto"
#define S_HV_PROTECTED "Prot. HV"
#define S_CHK_HV      "Alto voltaje"
#define S_CHK_TUBE    "Pulsos de tubo"
#define S_CHK_BAT     "Lectura batería"
#define S_CHK_E       "Sensor eléctrico"
#define S_CHK_H       "Sensor magnético"
#define S_CHK_CLOCK   "Reloj"
#define S_CHK_MEM     "Memoria log"
#define S_USB_STATE   "ENLACE"
#define S_STREAM      "Flujo en vivo"
#define S_CHARGE_A    "Carga"
#define S_CLOCK_SYNC  "Reloj"
#define S_SYNCED      "Sincronizado"
#define S_RECORDS     "Registros"
#define S_TRANSFER    "Transferencia"
#define S_ALL_ON_PC   "Todos los registros en el PC"
#define S_DOWNLOAD_NOW "Desc."
#define S_AUTO_ON     "Automático: encendido"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Registros"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Qué hacer en una emergencia nuclear.", "Lista de eventos de alta dosis pasados.", \
    "Muestra cuánto de la dosis elegida se ha usado.", "Registros de dosis separados para un viaje o paseo.", \
    "Comprueba si un objeto emite radiación.", "Localiza cables y fuentes de campo por sonido y barras.", \
    "Tiempo de grabación y capacidad de memoria.", "Configurar niveles de advertencia y alarma.", \
    "Condición del tubo, tiempo muerto, alto voltaje.", "Comprueba las partes del dispositivo una por una.", \
    "Conectar a PC y transferir registros.", "Seleccionar el tema de color de la pantalla.", \
    "Brillo, sonido, vibración y reloj.", "Ajustar sensibilidad y factores de campo manualmente.", \
    "Ajusta la sensibilidad con una fuente conocida.", "Versión y enlaces de proyecto."}
#define SETTINGS_DESC {"Seleccionar el tema de color de la pantalla.", "Una pantalla atenuada alarga la duración de la batería.", \
    "¿Deben sonar alarmas y advertencias?", "¿Debe vibrar el dispositivo en alarma?", \
    "Un clic por cada cuenta de tubo, como los contadores Geiger clásicos.", "El motor emite un zumbido corto en cada cuenta.", \
    "El LED parpadea en cada cuenta. Apagarlo ahorra algo de energía.", "El LED se enciende mientras la batería se carga.", \
    "Color del LED de conteo y carga. El tema sigue el color de acento de la pantalla.", \
    "¿Debe encenderse el dispositivo al recibir energía USB? Si no, mantenga pulsada la tecla de encendido.", \
    "Zona horaria que muestra el reloj.", "Configurar la hora y fecha manualmente."}
/* sessions */
#define S_T_SESSIONS   "Sesiones"
#define S_SESS_NEW     "Iniciar nueva sesión"
#define S_SESS_RUNNING "Sesión en curso"
#define S_SESS_NONE    "Aún no hay sesiones guardadas."
#define S_SESS_PICK    "Elegir tipo de sesión"
#define S_SESS_HINT    "Durante una sesión se registran dosis, tiempo y tasa máxima. Continúa cuando apagas el dispositivo."
#define SESS_KINDS_NAMES  {"Viaje / Vuelo", "Caminata", "Inspección interior", "Exterior", "Alrededor de una muestra", "General"}
#define S_SESS_DOSE    "Dosis recibida"
#define S_SESS_TIME    "Duración"
#define S_SESS_AVG     "Promedio"
#define S_SESS_MAX     "Máximo"
#define S_SESS_NOW     "Ahora"
#define S_SESS_ASK     "Presione OK para finalizar"
#define S_FINISH       "Finalizar"
#define S_BEGIN        "Iniciar"
#define S_APPLY        "Aplicar"
#define S_DISCARD      "Descartar"
#define S_BROWSE       "Explorar"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Días de dosis natural"
#define S_NATURAL_LESS "Menos de un día de dosis natural"
/* calibration wizard */
#define S_CW_T0        "Tasa de dosis de referencia"
#define S_CW_T1        "Midiendo"
#define S_CW_T2        "Resultado"
#define S_CW_HELP0     "Coloque Yarrow junto a una fuente cuyo caudal de dosis conoce: el valor que muestra otro dispositivo o una fuente certificada. Introduzca ese valor."
#define S_CW_COUNTS    "Recuentos"
#define S_CW_HINT1     "Error de aprox. 3 % a 1000 recuentos. Una medición más larga es más precisa."
#define S_CW_SAT       "El tubo está muy ocupado: aleje la fuente."
#define S_CW_NEEDMORE  "Se necesitan al menos 100 recuentos para terminar."
#define S_CW_OLD       "Actual"
#define S_CW_NEW       "Nuevo"
#define S_CW_DIFF      "Cambiar"
#define S_CW_UNC       "Incertidumbre"
#define S_CW_RANGE     "Valor fuera de rango: medir de nuevo."
#define S_M_GUIDE    "Guía de emergencia"
#define S_M_EVENTS   "Log de eventos"
#define S_M_BUDGET   "Límite de dosis"
#define S_M_SAMPLE   "Prueba de muestra"
#define S_M_SCAN     "Buscador de campo y cable"
#define S_M_SESSIONS "Sesiones"
#define S_M_CALWIZ   "Asistente de calibración"
#define S_M_LOG      "Registro/Almacenamiento"
#define S_M_THRESH   "Niveles de alarma"
#define S_M_TUBE     "Estado del tubo"
#define S_M_SELFTEST "Autoprueba"
#define S_M_USB      "Enlace PC"
#define S_M_SETTINGS "Ajustes"
#define S_M_THEMES   "Temas"
#define S_M_ABOUT    "Sobre"
#define S_M_CALIB    "Calibración"
#define S_OPEN_SOURCE "Código abierto · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Quédese dentro", \
     {"Vaya al edificio más cercano, sótano o habitación interior si puede.", \
      "Cierre puertas, ventanas y ventilación.", \
      "Apague el aire acondicionado y los ventiladores.", \
      "Escuche anuncios oficiales en radio o teléfono.", \
      "No salga hasta nuevo aviso.", 0, 0}}, \
    {"Si estuvo fuera", \
     {"Quítese la ropa exterior y ciérrela en una bolsa. Esto elimina la mayor parte de la contaminación.", \
      "Dúchese con agua y jabón, lávese el pelo.", \
      "Enjuague ojos y heridas con agua limpia.", \
      "No frote ni irrite la piel.", 0, 0, 0}}, \
    {"Agua y alimentos", \
     {"Los alimentos y bebidas en envases sellados son seguros.", \
      "No consuma alimentos dejados al aire libre ni productos de huerto.", \
      "Espere a que las autoridades informen sobre el agua del grifo.", \
      "Lave sus manos antes de comer.", 0, 0, 0}}, \
    {"Yoduro de potasio", \
     {"Tómelo solo cuando las autoridades se lo indiquen.", \
      "Protege la tiroides solo contra el yodo radiactivo.", \
      "Para embarazadas y niños, consulte a un médico o farmacéutico sobre la dosis.", \
      "No empiece a tomarlo por su cuenta.", 0, 0, 0}}, \
    {"Obtener información", \
     {"Escuche radio (FM/AM) y anuncios telefónicos.", \
      "Número de emergencias: 112.", \
      "La energía puede fallar: tenga radio a pilas y móvil cargado.", 0, 0, 0, 0}}, \
    {"Tabla de dosis", \
     {"Fondo natural: aprox. 0.1 µSv/h, 2.4 mSv/año.", \
      "Límite público: 1 mSv/año.", \
      "Rayos X de tórax: 0.1 mSv.", \
      "TC: 2–10 mSv.", \
      "Trabajador en radiación: 20 mSv/año.", \
      "1000 mSv o más a la vez: enfermedad por radiación.", 0}}, \
    {"Niveles de alarma", \
     {"Precaución: aumento sobre el fondo.", \
      "Peligro: muévase al interior y lea la guía.", \
      "Evacuar: siga las instrucciones oficiales.", \
      "Puede cambiar los niveles en Configuración.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Etapa "
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "Últimos 7 días"
#define S_PEAK_WORD    "Pico "
#define S_MILLION      " Millón"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 h"}
#define L_WEEKDAYS     {"Lu", "Ma", "Mi", "Ju", "Vi", "Sá", "Do"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " h"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Desconectado"
