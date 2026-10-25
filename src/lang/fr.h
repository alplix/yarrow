/* User interface text: French. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "ATTENTION"
#define S_DANGER    "DANGER"
#define S_EVACUATE  "ÉVACUER"
#define S_LOW       "FAIBLE"
#define S_MEDIUM    "MOYEN"
#define S_HIGH      "ÉLEVÉ"

/* titles */
#define S_T_RADIATION "Radiation"
#define S_T_LIVE      "Direct"
#define S_T_HISTORY   "Historique"
#define S_T_EFIELD    "Champ électrique"
#define S_T_HFIELD    "Champ magnétique"
#define S_T_BATTERY   "Batterie & appareil"
#define S_T_GUIDE     "Guide d'urgence"
#define S_T_SETTINGS  "Paramètres"
#define S_T_EVENTS    "Événements"
#define S_T_BUDGET    "Budget de dose"
#define S_T_SAMPLE    "Échantillon"
#define S_T_LOG       "Jrn"
#define S_T_STORAGE   "Stockage"
#define S_T_THRESH    "Niveaux d'alarme"
#define S_T_TUBE      "État du tube"
#define S_T_SELFTEST  "Auto-test"
#define S_T_USB       "Liaison PC"
#define S_T_MODE      "Mode de mesure"
#define S_T_SCAN      "Localisateur"
#define S_T_MENU      "Menu"
#define S_T_THEMES    "Thèmes"
#define S_T_ABOUT     "À propos"
#define S_T_CLOCK     "Heure et date"
#define S_T_CALIB     "Étalonnage"
#define S_CLOCK_DATE  "Date/Heure"
#define S_DAY    "Jour"
#define S_MONTH  "Mois"
#define S_YEAR   "Année"
#define S_HOUR   "Heure"
#define S_MINUTE "Minute"
#define S_NEXT   "Suivant"
#define S_SAVE   "Sauvegarder"
#define S_CAL_SENS   "Sensibilité"
#define S_CAL_DEAD   "Temps mort"
#define S_CAL_E      "Facteur électrique"
#define S_CAL_H      "Facteur magnétique"
#define S_CAL_NOTE   "Ajuster l'appareil avec une source connue."

/* labels */
#define S_TODAY     "Aujourd'hui"
#define S_WEEK      "Semaine"
#define S_TOTAL     "Total"
#define S_MIN       "Min"
#define S_AVG       "Moy"
#define S_MAX       "Max"
#define S_PEAK      "Pic"
#define S_AVERAGE   "Moyenne"
#define S_NOW       "Maintenant"
#define S_REC_ON    "Enregistrement"
#define S_BAT_LEFT  "Batterie"
#define S_LAST_10   "Dernières 10 min"
#define S_NORMAL_x  "normal"
#define S_TIMES     "fois"
#define S_THRESHOLD "niveau"
#define S_SCALE     "échelle"
#define S_DETAILS   "détails"
#define S_OPEN      "Ouvrir"
#define S_BACK      "Retour"
#define S_SELECT    "Sélec."
#define S_MUTE      "Muet"
#define S_GUIDE     "Guide"
#define S_CHANGE    "Changer"
#define S_RESET     "Reset"
#define S_FINISH    "Terminer"
#define S_CANCEL    "Annuler"
#define S_ON        "ON"
#define S_OFF       "OFF"
#define S_YES       "Oui"
#define S_NO        "Non"
#define S_DONE      "Terminé"

/* battery screen */
#define S_REMAINING "Temps restant"
#define S_HEALTH    "Santé batterie"
#define S_CYCLES    "Cycles de charge"
#define S_RUNTIME   "Temps total"
#define S_TUBE_LIFE "Durée du tube"
#define S_HOURS     "h"
#define S_MINUTES   "min"
#define S_CHARGING  "Charge"
#define S_FULL_BAT  "Batterie pleine"
#define S_PRESS_ANY "Appuyez sur une touche"
#define S_CHARGED   "Plein"
#define S_NO_USB    "Pas de USB"

/* alarm */
#define S_STEP1 "Se réfugier"
#define S_STEP2 "Fermez portes et fenêtres"
#define S_STEP3 "Écoutez les nouvelles"

/* settings */
#define S_THEME      "Thème"
#define S_BRIGHTNESS "Luminosité"
#define S_SOUND      "Son alarme"
#define S_VIBRATION  "Vibration alarme"
#define S_LANGUAGE   "Langue"
#define S_ATT_TH     "Niveau d'alerte"
#define S_CLICK      "Compteur clic"
#define S_COUNT_VIBE "Compteur vibration"
#define S_USB_START  "Démarrage sur USB"
#define S_LED_COLOR  "Couleur LED"
#define LED_COLOR_NAMES {"Thème", "Rouge", "Vert", "Bleu", "Jaune", "Cyan", "Magenta", "Blanc"}
#define S_LED_PULSE  "Compte LED"
#define S_LED_CHARGE "Charge LED"
#define S_TIME_ZONE  "Fuseau horaire"
#define S_TH_ATTENTION "Attention"
#define S_TH_DANGER    "Danger"
#define S_TH_EVACUATE  "Évacuer"
#define S_SOUND_LIGHT  "son · vibration · lumière"
#define S_TURKISH    "Turc"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Sombre", "Clair", "Papier", "Ambre", "Sakura", "Nuit", "Terminal", "Océan", "Forêt", "Contraste", "Lavande", "Menthe", "Crépuscule", "Charbon", "Sable", "Glace", "Cerise", "Violet", "Bonbon", "Mono", "Cramoisi"}
#define S_THEME_DARK   "Sombre"
#define S_THEME_LIGHT  "Clair"
#define S_THEME_PAPER  "Papier"
#define S_THEME_AMBER  "Ambre"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profil"
#define S_P_DETAIL   "Détaillé"
#define S_P_DENSE    "Dense"
#define S_P_BALANCED "Moyen"
#define S_P_LONG     "Long"
#define S_P_ARCHIVE  "Archives"
#define S_DAYS       "jours"
#define S_MONTHS     "mois"
#define S_YEARS      "ans"
#define S_OLDEST     "Plus ancien"
#define S_WHEN_FULL  "Plein"
#define S_OVERWRITE  "Écras"
#define S_FULL_PCT   "Plein"
#define S_FREE       "libre"
#define S_RECORDED   "Jours d'Enr."
#define S_ALARM_LOG  "Enr. d'alarme"
#define S_AUTO_NOTE  "Les moments d'alarme sont aussi enregistrés chaque seconde."
#define S_USB_DOWNLOAD "Enregistrements USB"

/* sample test, budget, scan */
#define S_BACKGROUND "Arrière-plan"
#define S_SAMPLE     "Échantillon"
#define S_NET_DIFF   "Différence nette"
#define S_RESULT     "Résultat"
#define S_HIGHER     "Clairement au-dessus du fond"
#define S_SAME       "Identique au fond"
#define S_MEASURE_TIME "Temps"
#define S_GOAL       "Objectif"
#define S_MISSION    "Budget"
#define S_START      "Démarré"
#define S_DURATION   "Durée"
#define S_LEFT       "Restant"
#define S_USED_PCT   "Utilisé"
#define S_ON_EXCEED  "Limite dépassée"
#define S_NEW_BUDGET "Nouveau B."
#define S_FIND_SRC   "Plus proche de la source, plus le son est fort."
#define S_APPROACH   "Approche de la source"
#define S_SOUND_ON   "Son activé"
#define S_SCAN_SOUND "Son de recherche"
#define S_MODE_NAV   "Mode"
#define S_EXIT       "Quitter"

/* tube, self test, usb */
#define S_HV         "Haute tension"
#define S_HV_FACTORY "Valeur d'usine"
#define S_TOTAL_PULSES "Total impulsions"
#define S_BG_RATE    "Fond"
#define S_DEADTIME   "Correction du temps mort"
#define S_HV_PROTECTED "Protégé HV"
#define S_CHK_HV      "Haute tension"
#define S_CHK_TUBE    "Impulsions tube"
#define S_CHK_BAT     "Lecture batterie"
#define S_CHK_E       "Capteur électrique"
#define S_CHK_H       "Capteur magnétique"
#define S_CHK_CLOCK   "Horloge"
#define S_CHK_MEM     "Mémoire log"
#define S_USB_STATE   "LIÉ"
#define S_STREAM      "Flux direct"
#define S_CHARGE_A    "Charge"
#define S_CLOCK_SYNC  "Horloge"
#define S_SYNCED      "Synchronisé"
#define S_RECORDS     "Enregistrements"
#define S_TRANSFER    "Transfert"
#define S_ALL_ON_PC   "Tous les enregistrements sur PC"
#define S_DOWNLOAD_NOW "Télécharger"
#define S_AUTO_ON     "Auto : ON"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Enregistrements"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Que faire en cas d'urgence nucléaire.", "Liste des événements à forte dose passés.", \
    "Indique la dose choisie utilisée.", "Enregistrements de dose séparés par voyage ou marche.", \
    "Vérifie si un objet émet des radiations.", "Localise fils et sources de champ par son et barres.", \
    "Temps d'enregistrement et niveau de mémoire.", "Définir les niveaux d'alerte et d'avertissement.", \
    "État du tube, temps mort, haute tension.", "Vérifie les composants de l'appareil un par un.", \
    "Connecter à un ordinateur et transférer les enregistrements.", "Choisir le thème de couleur de l'écran.", \
    "Luminosité, son, vibration et horloge.", "Régler la sensibilité et les facteurs de champ manuellement.", \
    "Définit la sensibilité avec une source connue.", "Version et liens de projet."}
#define SETTINGS_DESC {"Choisir le thème de couleur de l'écran.", "Un écran atténué prolonge la durée de vie de la batterie.", \
    "Les alarmes et avertissements doivent-ils sonner ?", "L'appareil doit-il vibrer en cas d'alarme ?", \
    "Un clic pour chaque compte de tube, comme les Geiger classiques.", "Le moteur émet un court bourdonnement à chaque compte.", \
    "La LED clignote à chaque compte. La désactiver économise de l'énergie.", "La LED s'allume pendant la charge de la batterie.", \
    "Couleur de la LED de comptage et de charge. Le thème suit la couleur d'accent de l'écran.", \
    "L'appareil doit-il s'allumer avec l'alimentation USB ? Sinon, maintenez la touche d'alimentation.", \
    "Zone horaire affichée par l'horloge.", "Régler l'heure et la date manuellement."}
/* sessions */
#define S_T_SESSIONS   "Sessions"
#define S_SESS_NEW     "Démarrer une nouvelle session"
#define S_SESS_RUNNING "Session en cours"
#define S_SESS_NONE    "Aucune session sauvegardée pour l'instant."
#define S_SESS_PICK    "Choisir le type de session"
#define S_SESS_HINT    "Pendant une session, la dose, le temps et le taux le plus élevé sont enregistrés. Cela continue même si vous éteignez l'appareil."
#define SESS_KINDS_NAMES  {"Trajet / vol", "Marche", "Relevé intérieur", "Extérieur", "Autour d'échantillon", "Général"}
#define S_SESS_DOSE    "Dose reçue"
#define S_SESS_TIME    "Durée"
#define S_SESS_AVG     "Moyenne"
#define S_SESS_MAX     "Maximum"
#define S_SESS_NOW     "Actuel"
#define S_SESS_ASK     "Appuyez sur OK pour finir"
#define S_FINISH       "Finir"
#define S_BEGIN        "Démarrer"
#define S_APPLY        "Appliquer"
#define S_DISCARD      "Annuler"
#define S_BROWSE       "Parcourir"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Jours de dose naturelle"
#define S_NATURAL_LESS "Moins d'un jour de dose naturelle"
/* calibration wizard */
#define S_CW_T0        "Taux de dose de référence"
#define S_CW_T1        "Mesure"
#define S_CW_T2        "Résultat"
#define S_CW_HELP0     "Placez Yarrow près d'une source dont vous connaissez le taux de dose : la valeur affichée par un autre appareil ou une source certifiée. Entrez cette valeur."
#define S_CW_COUNTS    "Comptes"
#define S_CW_HINT1     "Erreur d'environ 3 % à 1000 comptes. Une mesure plus longue est plus précise."
#define S_CW_SAT       "Le tube est trop sollicité : éloignez la source."
#define S_CW_NEEDMORE  "Au moins 100 comptes sont nécessaires pour terminer."
#define S_CW_OLD       "Courant"
#define S_CW_NEW       "Nouveau"
#define S_CW_DIFF      "Changer"
#define S_CW_UNC       "Incertitude"
#define S_CW_RANGE     "Valeur hors plage : mesurer à nouveau."
#define S_M_GUIDE    "Guide d'urgence"
#define S_M_EVENTS   "Journal d'événements"
#define S_M_BUDGET   "Budget de dose"
#define S_M_SAMPLE   "Test échantillon"
#define S_M_SCAN     "Localisateur de champ/fil"
#define S_M_SESSIONS "Sessions"
#define S_M_CALWIZ   "Assistant de calibration"
#define S_M_LOG      "Logs et stockage"
#define S_M_THRESH   "Niveaux d'alarme"
#define S_M_TUBE     "État du tube"
#define S_M_SELFTEST "Auto-test"
#define S_M_USB      "Lien PC"
#define S_M_SETTINGS "Paramètres"
#define S_M_THEMES   "Thèmes"
#define S_M_ABOUT    "À propos"
#define S_M_CALIB    "Calibration"
#define S_OPEN_SOURCE "Open source · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Restez à l'intérieur", \
     {"Allez dans le bâtiment le plus proche, un sous-sol ou une pièce intérieure si possible.", \
      "Fermez les portes, fenêtres et la ventilation.", \
      "Coupez la climatisation et les ventilateurs.", \
      "Écoutez les annonces officielles à la radio ou au téléphone.", \
      "Ne sortez pas tant que les autorités ne l'ont pas dit.", 0, 0}}, \
    {"Si vous étiez dehors", \
     {"Enlevez vos vêtements extérieurs et scellez-les dans un sac. Cela élimine la plupart de la contamination.", \
      "Douchez-vous avec du savon et de l'eau, lavez vos cheveux.", \
      "Rincez les yeux et les plaies avec de l'eau propre.", \
      "Ne frottez pas ou n'irritez pas la peau.", 0, 0, 0}}, \
    {"Eau et nourriture", \
     {"La nourriture et les boissons emballées sont sûres.", \
      "Ne mangez pas de nourriture laissée à l'extérieur ou de produits du jardin.", \
      "Attendez que les autorités donnent des informations sur l'eau du robinet.", \
      "Lavez-vous les mains avant de manger.", 0, 0, 0}}, \
    {"Iodure de potassium", \
     {"Prenez-le uniquement si les autorités vous le demandent.", \
      "Il protège la thyroïde uniquement contre l'iode radioactif.", \
      "Pour les femmes enceintes et les enfants, demandez la dose à un médecin ou pharmacien.", \
      "Ne commencez pas à le prendre par vous-même.", 0, 0, 0}}, \
    {"Infos", \
     {"Écoutez la radio (FM/AM) et les annonces téléphoniques.", \
      "Numéro d'urgence : 112.", \
      "Panne de courant possible : gardez une radio à piles et un téléphone chargé.", 0, 0, 0, 0}}, \
    {"Doses", \
     {"Fond naturel : environ 0.1 µSv/h, 2.4 mSv par an.", \
      "Limite publique : 1 mSv par an.", \
      "Radio thorax : 0.1 mSv.", \
      "TDM : 2–10 mSv.", \
      "Travailleur exposé : 20 mSv par an.", \
      "1000 mSv ou plus en une fois : maladie radiationnelle.", 0}}, \
    {"Niveaux d'alarme", \
     {"Attention : augmentation par rapport au fond.", \
      "Danger : rentrez à l'intérieur et lisez le guide.", \
      "Évacuer : suivez les instructions officielles.", \
      "Vous pouvez changer les niveaux dans les Paramètres.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Stade "
#define S_HELD_BACK    "Retenu"
#define S_RETRY        "Réessayer"
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "7 derniers jours"
#define S_PEAK_WORD    "Pic "
#define S_MILLION      " million"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 h"}
#define L_WEEKDAYS     {"Lu", "Ma", "Me", "Je", "Ve", "Sa", "Di"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " h"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"janv", "févr", "mars", "avr", "mai", "juin", "juil", "août", "sept", "oct", "nov", "déc"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Hors ligne"
