/* User interface text: Turkish (the source language). Every character used here ends up in the font data automatically.
 * A translation is a copy of this file with the same macro names; see lang/README.md. */

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
#define S_ATTENTION "DİKKAT"
#define S_DANGER    "TEHLİKE"
#define S_EVACUATE  "TAHLİYE"
#define S_LOW       "DÜŞÜK"
#define S_MEDIUM    "ORTA"
#define S_HIGH      "YÜKSEK"

/* titles */
#define S_T_RADIATION "Radyasyon"
#define S_T_LIVE      "Canlı"
#define S_T_HISTORY   "Geçmiş"
#define S_T_EFIELD    "Elektrik alan"
#define S_T_HFIELD    "Manyetik alan"
#define S_T_BATTERY   "Pil ve cihaz"
#define S_T_GUIDE     "Acil rehber"
#define S_T_SETTINGS  "Ayarlar"
#define S_T_EVENTS    "Olaylar"
#define S_T_BUDGET    "Doz bütçesi"
#define S_T_SAMPLE    "Numune"
#define S_T_LOG       "Kayıt"
#define S_T_STORAGE   "Depolama"
#define S_T_THRESH    "Alarm eşikleri"
#define S_T_TUBE      "Tüp durumu"
#define S_T_SELFTEST  "Sınama"
#define S_T_USB       "PC bağlantısı"
#define S_T_MODE      "Ölçüm modu"
#define S_T_SCAN      "Kaynak bul"
#define S_T_MENU      "Menü"
#define S_T_THEMES    "Temalar"
#define S_T_ABOUT     "Hakkında"
#define S_T_CLOCK     "Saat ve tarih"
#define S_T_CALIB     "Kalibrasyon"
#define S_CLOCK_DATE  "Saat ve tarih"
#define S_DAY    "Gün"
#define S_MONTH  "Ay"
#define S_YEAR   "Yıl"
#define S_HOUR   "Saat"
#define S_MINUTE "Dakika"
#define S_NEXT   "sonraki"
#define S_SAVE   "kaydet"
#define S_CAL_SENS   "Hassasiyet"
#define S_CAL_DEAD   "Ölü zaman"
#define S_CAL_E      "Elektrik çarpanı"
#define S_CAL_H      "Manyetik çarpanı"
#define S_CAL_NOTE   "Cihazı bilinen bir kaynakla karşılaştırarak ayarla."

/* labels */
#define S_TODAY     "Bugün"
#define S_WEEK      "Hafta"
#define S_TOTAL     "Toplam"
#define S_MIN       "Min"
#define S_AVG       "Ort"
#define S_MAX       "Maks"
#define S_PEAK      "Tepe"
#define S_AVERAGE   "Ortalama"
#define S_NOW       "şimdi"
#define S_REC_ON    "kayıt açık"
#define S_BAT_LEFT  "pil"
#define S_LAST_10   "son 10 dk"
#define S_NORMAL_x  "normalin"
#define S_TIMES     "katı"
#define S_THRESHOLD "eşik"
#define S_SCALE     "ölçek"
#define S_DETAILS   "ayrıntı"
#define S_OPEN      "aç"
#define S_BACK      "geri"
#define S_SELECT    "seç"
#define S_MUTE      "sustur"
#define S_GUIDE     "rehber"
#define S_CHANGE    "değiştir"
#define S_RESET     "sıfırla"
#define S_FINISH    "bitir"
#define S_CANCEL    "iptal"
#define S_ON        "açık"
#define S_OFF       "kapalı"
#define S_YES       "evet"
#define S_NO        "hayır"
#define S_DONE      "tamam"

/* battery screen */
#define S_REMAINING "Kalan süre"
#define S_HEALTH    "Pil sağlığı"
#define S_CYCLES    "Şarj döngüsü"
#define S_RUNTIME   "Toplam çalışma"
#define S_TUBE_LIFE "Tüp ömrü"
#define S_HOURS     "sa"
#define S_MINUTES   "dk"
#define S_CHARGING  "şarj oluyor"
#define S_FULL_BAT  "pil dolu"
#define S_PRESS_ANY "bir tuşa bas"
#define S_CHARGED   "dolu"
#define S_NO_USB    "USB yok"

/* alarm */
#define S_STEP1 "İçeri gir"
#define S_STEP2 "Kapı ve pencereleri kapat"
#define S_STEP3 "Haber dinle"

/* settings */
#define S_THEME      "Tema"
#define S_BRIGHTNESS "Parlaklık"
#define S_SOUND      "Alarm sesi"
#define S_VIBRATION  "Alarm titreşimi"
#define S_LANGUAGE   "Dil"
#define S_ATT_TH     "Dikkat eşiği"
#define S_CLICK      "Sayım tıkı"
#define S_COUNT_VIBE "Sayım titreşimi"
#define S_USB_START  "USB güçle aç"
#define S_LED_COLOR  "LED rengi"
#define LED_COLOR_NAMES {"Tema", "Kırmızı", "Yeşil", "Mavi", "Sarı", "Camgöbeği", "Eflatun", "Beyaz"}
#define S_LED_PULSE  "Sayım LED'i"
#define S_LED_CHARGE "Şarj LED'i"
#define S_TIME_ZONE  "Saat dilimi"
#define S_TH_ATTENTION "Dikkat"
#define S_TH_DANGER    "Tehlike"
#define S_TH_EVACUATE  "Tahliye"
#define S_SOUND_LIGHT  "ses · titreşim · ışık"
#define S_TURKISH    "Türkçe"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Koyu", "Açık", "Kağıt", "Amber", "Sakura", "Gece", "Terminal", "Okyanus", "Orman", "Kontrast",                         "Lavanta", "Nane", "Gün batımı", "Kömür", "Kum", "Buz", "Kiraz", "Mor", "Şeker", "Mono", "Kızıl"}
#define S_THEME_DARK   "Koyu"
#define S_THEME_LIGHT  "Açık"
#define S_THEME_PAPER  "Kağıt"
#define S_THEME_AMBER  "Amber"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profil"
#define S_P_DETAIL   "Ayrıntılı"
#define S_P_DENSE    "Yoğun"
#define S_P_BALANCED "Dengeli"
#define S_P_LONG     "Uzun"
#define S_P_ARCHIVE  "Arşiv"
#define S_DAYS       "gün"
#define S_MONTHS     "ay"
#define S_YEARS      "yıl"
#define S_OLDEST     "En eski kayıt"
#define S_WHEN_FULL  "Dolunca"
#define S_OVERWRITE  "eskiyi ez"
#define S_FULL_PCT   "dolu"
#define S_FREE       "boş"
#define S_RECORDED   "gün kayıt var"
#define S_ALARM_LOG  "Alarm kaydı"
#define S_AUTO_NOTE  "Alarm anları ayrıca 1 sn kaydedilir."
#define S_USB_DOWNLOAD "USB: kayıtları indir"

/* sample test, budget, scan */
#define S_BACKGROUND "Arka plan"
#define S_SAMPLE     "Numune"
#define S_NET_DIFF   "Net fark"
#define S_RESULT     "Sonuç"
#define S_HIGHER     "Arka plandan belirgin yüksek"
#define S_SAME       "Arka planla aynı"
#define S_MEASURE_TIME "ölçüm süresi"
#define S_GOAL       "hedef"
#define S_MISSION    "Görev"
#define S_START      "Başlangıç"
#define S_DURATION   "Süre"
#define S_LEFT       "Kalan"
#define S_USED_PCT   "kullanıldı"
#define S_ON_EXCEED  "aşılınca: alarm"
#define S_NEW_BUDGET "yeni bütçe"
#define S_FIND_SRC   "Kaynağa yaklaştıkça ses tizleşir ve çubuklar yükselir."
#define S_APPROACH   "yaklaşıyor"
#define S_SOUND_ON   "ses açık"
#define S_SCAN_SOUND "tarama sesi"
#define S_MODE_NAV   "mod"
#define S_EXIT       "çık"

/* tube, self test, usb */
#define S_HV         "Yüksek voltaj"
#define S_HV_FACTORY "fabrika değeri"
#define S_TOTAL_PULSES "Toplam darbe"
#define S_BG_RATE    "Arka plan"
#define S_DEADTIME   "Ölü zaman düzeltmesi"
#define S_HV_PROTECTED "HV korumalı"
#define S_CHK_HV      "Yüksek voltaj"
#define S_CHK_TUBE    "Tüp darbeleri"
#define S_CHK_BAT     "Pil ölçümü"
#define S_CHK_E       "Elektrik sensörü"
#define S_CHK_H       "Manyetik sensör"
#define S_CHK_CLOCK   "Saat"
#define S_CHK_MEM     "Kayıt belleği"
#define S_USB_STATE   "BAĞLI"
#define S_STREAM      "Canlı akış"
#define S_CHARGE_A    "Şarj"
#define S_CLOCK_SYNC  "Saat"
#define S_SYNCED      "eşitlendi"
#define S_RECORDS     "Kayıtlar"
#define S_TRANSFER    "Aktarım"
#define S_ALL_ON_PC   "tüm kayıtlar PC'de"
#define S_DOWNLOAD_NOW "şimdi indir"
#define S_AUTO_ON     "otomatik: açık"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "kayıt"

/* menu entries */
/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Nükleer acil durumda ne yapılacağını anlatır.", "Geçmişte yaşanan yüksek doz olaylarının listesi.", \
    "Belirlediğin dozdan ne kadarını kullandığını gösterir.", "Yolculuk ya da yürüyüş için ayrı doz kaydı tutar.", \
    "Bir nesnenin radyasyon yayıp yaymadığını ölçer.", "Duvardaki kabloyu ve alan kaynaklarını ses ve çubukla bulur.", \
    "Kayıt süresi ve belleğin ne kadar dolduğu.", "Uyarı ve alarm seviyelerini ayarla.", \
    "Sayaç tüpünün durumu, ölü zaman, yüksek voltaj.", "Cihazın parçalarını sırayla kontrol eder.", \
    "USB ile bilgisayara bağlan, kayıtları aktar.", "Ekranın renk temasını seç.", \
    "Parlaklık, ses, titreşim ve saat.", "Ölçü hassasiyetini ve alan değerlerini elle ayarla.", \
    "Bilinen bir kaynakla duyarlılığı ayarlar.", "Sürüm ve proje bağlantıları.", \
    "Tüpün ölü zamanını iki kaynakla ölçer."}
#define SETTINGS_DESC {"Ekranın renk temasını seç.", "Parlaklığı azaltmak pil ömrünü uzatır.", \
    "Alarm ve uyarılarda ses çalsın mı.", "Alarmda cihaz titresin mi.", \
    "Tüpten gelen her sayımda eski Geiger sayaçları gibi tık sesi çıkar.", "Her sayımda motor kısa bir titreşim verir.", \
    "Her sayımda LED yanıp söner. Kapatmak biraz pil kazandırır.", "Kablo takılıyken şarj olurken LED yanar.", \
    "Sayım ve şarj LED'inin rengi. Tema seçiliyse ekranın vurgu rengini izler.", \
    "Kapalıyken USB güç gelince cihaz kendiliğinden açılsın mı. Kapalıysa açmak için güç tuşunu basılı tut.", \
    "Saat hangi bölgeye göre gösterilsin (Türkiye UTC+3).", "Saati ve tarihi elle ayarla.", \
    "Boşta kalınca ekranı kapatmak yerine büyük saat ve radyasyon seviyesini gösterir.", \
    "Hep takılı: ekran 30 sn sonra kararır, USB donarsa yeniden açılır."}
/* oturumlar */
#define S_T_SESSIONS   "Oturumlar"
#define S_SESS_NEW     "Yeni oturum başlat"
#define S_SESS_RUNNING "Devam eden oturum"
#define S_SESS_NONE    "Henüz kayıtlı oturum yok."
#define S_SESS_PICK    "Oturum türünü seç"
#define S_SESS_HINT    "Oturum boyunca doz, süre ve en yüksek hız kaydedilir. Cihazı kapatsan da devam eder."
#define SESS_KINDS_NAMES  {"Yolculuk / uçuş", "Yürüyüş", "Bina içi tarama", "Dışarıda", "Numune çevresi", "Genel"}
#define S_SESS_DOSE    "Alınan doz"
#define S_SESS_TIME    "Süre"
#define S_SESS_AVG     "Ortalama"
#define S_SESS_MAX     "En yüksek"
#define S_SESS_NOW     "Şimdi"
#define S_SESS_ASK     "Bitirmek için OK'e tekrar bas"
#define S_FINISH       "bitir"
#define S_BEGIN        "başlat"
#define S_APPLY        "uygula"
#define S_DISCARD      "vazgeç"
#define S_BROWSE       "gez"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "günlük doğal doz"
#define S_NATURAL_LESS "bir günden az doğal doz"
/* kalibrasyon sihirbazı */
#define S_CW_T0        "Referans doz hızı"
#define S_CW_T1        "Ölçüm"
#define S_CW_T2        "Sonuç"
#define S_CW_HELP0     "Yarrow'u, doz hızını bildiğin bir kaynağın yanına koy: orijinal cihazın okuduğu değer ya da sertifikalı bir kaynak. Değeri gir."
#define S_CW_COUNTS    "sayım"
#define S_CW_HINT1     "1000 sayımda yaklaşık %3 hata. Uzun ölçüm daha doğru."
#define S_CW_SAT       "Tüp çok yoğun çalışıyor: kaynağı uzaklaştır."
#define S_CW_NEEDMORE  "Bitirmek için en az 100 sayım gerekir."
#define S_CW_OLD       "Şimdiki"
#define S_CW_NEW       "Yeni"
#define S_CW_DIFF      "Fark"
#define S_CW_UNC       "Belirsizlik"
#define S_CW_RANGE     "Değer sınır dışı: ölçümü tekrarla."
#define S_M_GUIDE    "Acil rehber"
#define S_M_EVENTS   "Olay günlüğü"
#define S_M_BUDGET   "Doz bütçesi"
#define S_M_SAMPLE   "Numune testi"
#define S_M_SCAN     "Alan ve kablo bulucu"
#define S_M_SESSIONS "Oturumlar"
#define S_M_CALWIZ   "Kalibrasyon sihirbazı"
#define S_M_LOG      "Kayıt ve depolama"
#define S_M_THRESH   "Alarm eşikleri"
#define S_M_TUBE     "Tüp durumu"
#define S_M_SELFTEST "Öz test"
#define S_M_USB      "PC bağlantısı"
#define S_M_SETTINGS "Ayarlar"
#define S_M_THEMES   "Temalar"
#define S_M_ABOUT    "Hakkında"
#define S_M_CALIB    "Kalibrasyon"
#define S_OPEN_SOURCE "Açık kaynak · GPL-3.0"

/* emergency guide: title and up to seven lines per page */
/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"İçeride kal", \
     {"En yakın binaya gir. Mümkünse bodrum ya da iç oda.", \
      "Kapı, pencere ve havalandırmayı kapat.", \
      "Klima ve fanları durdur.", \
      "Radyo ya da telefondan resmi duyuruları dinle.", \
      "Yetkililer söyleyene kadar dışarı çıkma.", 0, 0}}, \
    {"Dışarıdaydıysan", \
     {"Üstündeki kıyafetleri çıkar, kapalı bir poşete koy. Bulaşmanın büyük kısmını giderir.", \
      "Sabun ve suyla duş al, saçını yıka.", \
      "Gözleri ve yarayı temiz suyla yıka.", \
      "Cildi ovma, tahriş etme.", 0, 0, 0}}, \
    {"Su ve gıda", \
     {"Kapalı ambalajdaki yiyecek ve içecekler güvenlidir.", \
      "Dışarıda açık kalmış yiyecekleri ve bahçe ürünlerini yeme.", \
      "Şebeke suyu için yetkililerin açıklamasını bekle.", \
      "Elleri yıkamadan yemek yeme.", 0, 0, 0}}, \
    {"Potasyum iyodür", \
     {"Yalnızca yetkililer söylediğinde kullan.", \
      "Sadece radyoaktif iyoda karşı tiroidi korur.", \
      "Hamilelik ve çocuklar için dozu doktora ya da eczacıya danış.", \
      "Kendi başına almaya başlama.", 0, 0, 0}}, \
    {"Haber al", \
     {"Radyo (FM/AM) ve telefon duyurularını dinle.", \
      "Acil durum numarası: 112.", \
      "Şebeke kesilebilir: pilli radyo ve dolu telefon hazır tut.", 0, 0, 0, 0}}, \
    {"Doz tablosu", \
     {"Doğal arka plan: yaklaşık 0.1 µSv/h, yılda 2.4 mSv.", \
      "Halk için yıllık sınır: 1 mSv.", \
      "Akciğer filmi: 0.1 mSv.", \
      "Bilgisayarlı tomografi: 2–10 mSv.", \
      "Radyasyon çalışanı: yılda 20 mSv.", \
      "Tek seferde 1000 mSv ve üzeri: radyasyon hastalığı.", 0}}, \
    {"Alarm seviyeleri", \
     {"Dikkat: arka plan üstünde bir artış var.", \
      "Tehlike: kapalı alana geç, rehbere bak.", \
      "Tahliye: resmi yönlendirmeyi izle.", \
      "Eşikleri Ayarlar'dan değiştirebilirsin.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "aşama "
#define S_HELD_BACK    "Tutuldu"
#define S_RETRY        "Tekrar"
#define S_SAVER        "Ekran koruyucu"
#define S_STATION      "İstasyon modu"
#define S_H_TITLE       "Sağlık"
#define S_H_STARTS     "Açılış"
#define S_H_THIS       "Bu açılış"
#define S_H_WDT        "Watchdog sıfırlama"
#define S_H_CRASHES    "Çökme"
#define S_H_USB        "USB yeniden başlatma"
#define S_H_RUNNING    "Çalışma süresi"
#define S_H_STARTUP    "Açılış aşaması"
#define S_H_STAGE      "aşama"
#define S_H_LASTCRASH  "Son çökme adresi"
#define S_M_DEADTIME   "Ölü zaman sihirbazı"
#define S_DT_A         "Yalnız kaynak A"
#define S_DT_AB        "A ve B birlikte"
#define S_DT_B         "Yalnız kaynak B"
#define S_DT_NONE      "Hiç kaynak yok"
#define S_DT_INTRO     "Benzer güçte iki kaynak gerekir; her biri tüpte saniyede en az 500 sayım vermeli, tüp yerinden oynamamalı. İkişer dakikalık dört sayım: A, A+B, B ve hiçbiri."
#define S_DT_WARN      "Böyle iki kaynak yoksa sonuç güvenilmez olur ve sihirbaz bunu söyler."
#define S_DT_PLACE     "Yukarıda yazan kaynağı tüpün önüne koyun, sonra başlatın. Sayım sürerken hiçbir şeyi oynatmayın."
#define S_DT_NORES     "Kullanılabilir sonuç yok."
#define S_DT_NORES_D   "Kaynaklar çok zayıftı ya da biri kıpırdadı. Sayımlar arasındaki fark gürültülerinden belirgin biçimde büyük değildi."
#define H_REASONS {"bilinmiyor", "güç açıldı", "reset pini", "watchdog", "pencere watchdog", "yazılım", "düşük güç", "çökme"}
#define S_AXIS_10MIN   "-10 dk"
#define S_AXIS_60S     "-60 sn"
#define S_T_LAST7      "Son 7 gün"
#define S_PEAK_WORD    "tepe "
#define S_MILLION      " milyon"
#define L_PROFILE_STEPS {"1 sn", "10 sn", "1 dk", "5 dk", "1 sa"}
#define L_WEEKDAYS     {"Pt", "Sa", "Ça", "Pe", "Cu", "Ct", "Pz"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " sa"
#define L_MIN_UNIT     " dk"
#define L_MONTHS       {"Oca", "Şub", "Mar", "Nis", "May", "Haz", "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "çevrimdışı"
