/* User interface text: Indonesian. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "PERHATIAN"
#define S_DANGER    "BAHAYA"
#define S_EVACUATE  "EVAKUASI"
#define S_LOW       "RENDAH"
#define S_MEDIUM    "SEDANG"
#define S_HIGH      "TINGGI"

/* titles */
#define S_T_RADIATION "Radiasi"
#define S_T_LIVE      "Langsung"
#define S_T_HISTORY   "Riwayat"
#define S_T_EFIELD    "Medan listrik"
#define S_T_HFIELD    "Medan magnet"
#define S_T_BATTERY   "Baterai & alat"
#define S_T_GUIDE     "Panduan darurat"
#define S_T_SETTINGS  "Pengaturan"
#define S_T_EVENTS    "Peristiwa"
#define S_T_BUDGET    "Anggaran Dosis"
#define S_T_SAMPLE    "Sampel"
#define S_T_LOG       "Log"
#define S_T_STORAGE   "Penyimpanan"
#define S_T_THRESH    "Tingkat Alarm"
#define S_T_TUBE      "Status Tabung"
#define S_T_SELFTEST  "Uji Mandiri"
#define S_T_USB       "Tautan PC"
#define S_T_MODE      "Mode Pengukuran"
#define S_T_SCAN      "Pencari Medan"
#define S_T_MENU      "Menu"
#define S_T_THEMES    "Tema"
#define S_T_ABOUT     "Tentang"
#define S_T_CLOCK     "Waktu & Tanggal"
#define S_T_CALIB     "Kalibrasi"
#define S_CLOCK_DATE  "Waktu & Tanggal"
#define S_DAY    "Hari"
#define S_MONTH  "Bulan"
#define S_YEAR   "Tahun"
#define S_HOUR   "Jam"
#define S_MINUTE "Menit"
#define S_NEXT   "Berikut"
#define S_SAVE   "Simpan"
#define S_CAL_SENS   "Sensitivitas"
#define S_CAL_DEAD   "Waktu Mati"
#define S_CAL_E      "Faktor Listrik"
#define S_CAL_H      "Faktor magnetik"
#define S_CAL_NOTE   "Sesuaikan alat dengan membandingkannya dengan sumber yang diketahui."

/* labels */
#define S_TODAY     "Hari"
#define S_WEEK      "Minggu"
#define S_TOTAL     "Total"
#define S_MIN       "Min"
#define S_AVG       "Avg"
#define S_MAX       "Maks"
#define S_PEAK      "Puncak"
#define S_AVERAGE   "Rata-rata"
#define S_NOW       "Sekarang"
#define S_REC_ON    "Perekaman"
#define S_BAT_LEFT  "Baterai"
#define S_LAST_10   "10 min terakhir"
#define S_NORMAL_x  "Normal"
#define S_TIMES     "Kali"
#define S_THRESHOLD "Tingkat"
#define S_SCALE     "Skala"
#define S_DETAILS   "Detail"
#define S_OPEN      "Buka"
#define S_BACK      "Kembali"
#define S_SELECT    "Pilih"
#define S_MUTE      "Senyap"
#define S_GUIDE     "Panduan"
#define S_CHANGE    "Ubah"
#define S_RESET     "Reset"
#define S_FINISH    "Selesai"
#define S_CANCEL    "Batal"
#define S_ON        "ON"
#define S_OFF       "OFF"
#define S_YES       "Ya"
#define S_NO        "Tidak"
#define S_DONE      "Selesai"

/* battery screen */
#define S_REMAINING "Sisa waktu"
#define S_HEALTH    "Kesehatan baterai"
#define S_CYCLES    "Siklus pengisian"
#define S_RUNTIME   "Total waktu operasi"
#define S_TUBE_LIFE "Umur tabung"
#define S_HOURS     "h"
#define S_MINUTES   "menit"
#define S_CHARGING  "Mengisi daya"
#define S_FULL_BAT  "Baterai penuh"
#define S_PRESS_ANY "Tekan tombol"
#define S_CHARGED   "Penuh"
#define S_NO_USB    "Tanpa USB"

/* alarm */
#define S_STEP1 "Masuk ke dalam"
#define S_STEP2 "Tutup pintu dan jendela"
#define S_STEP3 "Dengarkan berita"

/* settings */
#define S_THEME      "Tema"
#define S_BRIGHTNESS "Kecerahan"
#define S_SOUND      "Suara alarm"
#define S_VIBRATION  "Getaran alarm"
#define S_LANGUAGE   "Bahasa"
#define S_ATT_TH     "Tingkat bahaya"
#define S_CLICK      "Hitungan klik"
#define S_COUNT_VIBE "Hitungan getaran"
#define S_USB_START  "Mulai dengan daya USB"
#define S_LED_COLOR  "Warna LED"
#define LED_COLOR_NAMES {"Tema", "Merah", "Hijau", "Biru", "Kuning", "Cyan", "Magenta", "Putih"}
#define S_LED_PULSE  "LED Hitung"
#define S_LED_CHARGE "LED Isi Daya"
#define S_TIME_ZONE  "Zona waktu"
#define S_TH_ATTENTION "Hati-hati"
#define S_TH_DANGER    "Bahaya"
#define S_TH_EVACUATE  "Evakuasi"
#define S_SOUND_LIGHT  "suara · getaran · cahaya"
#define S_TURKISH    "Turki"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Gelap", "Terang", "Kertas", "Amber", "Sakura", "Malam", "Terminal", "Samudra", "Hutan", "Kontras", "Lavender", "Mint", "Matahari Terbenam", "Arang", "Pasir", "Es", "Ceri", "Ungu", "Permen", "Mono", "Crimson"}
#define S_THEME_DARK   "Gelap"
#define S_THEME_LIGHT  "Terang"
#define S_THEME_PAPER  "Kertas"
#define S_THEME_AMBER  "Amber"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Profil"
#define S_P_DETAIL   "Detail"
#define S_P_DENSE    "Padat"
#define S_P_BALANCED "Sedang"
#define S_P_LONG     "Panjang"
#define S_P_ARCHIVE  "Arsip"
#define S_DAYS       "Hari"
#define S_MONTHS     "Bulan"
#define S_YEARS      "Tahun"
#define S_OLDEST     "Data tertua"
#define S_WHEN_FULL  "Saat penuh"
#define S_OVERWRITE  "Timpa"
#define S_FULL_PCT   "Penuh"
#define S_FREE       "Gratis"
#define S_RECORDED   "Hari catatan"
#define S_ALARM_LOG  "Catatan alarm"
#define S_AUTO_NOTE  "Momen alarm juga disimpan setiap detik."
#define S_USB_DOWNLOAD "USB: unduh catatan"

/* sample test, budget, scan */
#define S_BACKGROUND "Latar belakang"
#define S_SAMPLE     "Sampel"
#define S_NET_DIFF   "Selisih bersih"
#define S_RESULT     "Hasil"
#define S_HIGHER     "Jelas di atas latar belakang"
#define S_SAME       "Sama dengan latar belakang"
#define S_MEASURE_TIME "Waktu"
#define S_GOAL       "Target"
#define S_MISSION    "Anggaran"
#define S_START      "Dimulai"
#define S_DURATION   "Durasi"
#define S_LEFT       "Sisa"
#define S_USED_PCT   "terpakai"
#define S_ON_EXCEED  "Batas: alarm"
#define S_NEW_BUDGET "Anggaran"
#define S_FIND_SRC   "Semakin dekat ke sumber, semakin tinggi nada dan batang."
#define S_APPROACH   "mendekat"
#define S_SOUND_ON   "suara aktif"
#define S_SCAN_SOUND "suara pencari"
#define S_MODE_NAV   "mode"
#define S_EXIT       "keluar"

/* tube, self test, usb */
#define S_HV         "Tegangan tinggi"
#define S_HV_FACTORY "nilai pabrik"
#define S_TOTAL_PULSES "Pulsa Total"
#define S_BG_RATE    "Latar Belakang"
#define S_DEADTIME   "Koreksi waktu mati"
#define S_HV_PROTECTED "Terlindungi HV"
#define S_CHK_HV      "Tegangan tinggi"
#define S_CHK_TUBE    "Pulsa tabung"
#define S_CHK_BAT     "Pembacaan baterai"
#define S_CHK_E       "Sensor listrik"
#define S_CHK_H       "Sensor magnetik"
#define S_CHK_CLOCK   "Jam"
#define S_CHK_MEM     "Memori log"
#define S_USB_STATE   "TERHUBUNG"
#define S_STREAM      "Aliran langsung"
#define S_CHARGE_A    "Muatan"
#define S_CLOCK_SYNC  "Jam"
#define S_SYNCED      "Tersinkron"
#define S_RECORDS     "Catatan"
#define S_TRANSFER    "Transfer"
#define S_ALL_ON_PC   "Semua catatan di PC"
#define S_DOWNLOAD_NOW "Unduh kini"
#define S_AUTO_ON     "Auto: hidup"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Catatan"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"Apa yang harus dilakukan saat darurat nuklir.", "Daftar kejadian dosis tinggi masa lalu.", \
    "Menunjukkan sisa dosis yang telah Anda gunakan.", "Catatan dosis terpisah untuk perjalanan atau jalan kaki.", \
    "Memeriksa apakah objek memancarkan radiasi.", "Menemukan kabel dan sumber medan dengan suara dan bar.", \
    "Waktu perekaman dan seberapa penuh memori.", "Atur tingkat peringatan dan alarm.", \
    "Kondisi tabung, waktu mati, tegangan tinggi.", "Memeriksa bagian perangkat satu per satu.", \
    "Hubungkan ke komputer dan pindahkan data.", "Pilih tema warna layar.", \
    "Kecerahan, suara, getaran, dan jam.", "Atur sensitivitas dan faktor medan secara manual.", \
    "Mengatur sensitivitas dengan sumber yang diketahui.", "Versi dan tautan proyek.", \
    "Mengukur waktu mati tabung dengan dua sumber."}
#define SETTINGS_DESC {"Pilih tema warna layar.", "Layar yang lebih redup membuat baterai lebih awet.", \
    "Haruskah alarm dan peringatan mengeluarkan suara.", "Haruskah perangkat bergetar saat alarm.", \
    "Klik pada setiap hitungan tabung, seperti Geiger klasik.", "Motor mengeluarkan bunyi dengungan pendek pada setiap hitungan.", \
    "LED berkedip pada setiap hitungan. Mematikannya menghemat daya.", "LED menyala saat baterai sedang mengisi daya.", \
    "Warna LED hitungan dan pengisian daya. Tema mengikuti warna aksen layar.", \
    "Apakah perangkat menyala otomatis saat daya USB tiba. Jika tidak, tahan tombol daya untuk menyalakannya.", \
    "Zona waktu yang ditampilkan jam.", "Atur waktu dan tanggal secara manual.", \
    "Saat diam, menampilkan jam besar dan tingkat radiasi alih-alih mematikan layar.", \
    "Selalu tersambung: layar mati 30 detik, USB restart jika macet."}
/* sessions */
#define S_T_SESSIONS   "Sesi"
#define S_SESS_NEW     "Mulai sesi baru"
#define S_SESS_RUNNING "Sesi berjalan"
#define S_SESS_NONE    "Belum ada sesi tersimpan."
#define S_SESS_PICK    "Pilih jenis sesi"
#define S_SESS_HINT    "Selama sesi, dosis, waktu, dan laju tertinggi dicatat. Ini terus berjalan saat perangkat dimatikan."
#define SESS_KINDS_NAMES  {"Perjalanan / Penerbangan", "Berjalan", "Survei dalam ruangan", "Luar ruangan", "Sekitar sampel", "Umum"}
#define S_SESS_DOSE    "Dosis diterima"
#define S_SESS_TIME    "Durasi"
#define S_SESS_AVG     "Rata-rata"
#define S_SESS_MAX     "Tertinggi"
#define S_SESS_NOW     "Sekarang"
#define S_SESS_ASK     "Tekan OK lagi untuk selesai"
#define S_FINISH       "Selesai"
#define S_BEGIN        "Mulai"
#define S_APPLY        "Terapkan"
#define S_DISCARD      "Buang"
#define S_BROWSE       "Jelajahi"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Hari dosis alami"
#define S_NATURAL_LESS "Kurang dari satu hari dosis alami"
/* calibration wizard */
#define S_CW_T0        "Laju dosis referensi"
#define S_CW_T1        "Mengukur"
#define S_CW_T2        "Hasil"
#define S_CW_HELP0     "Letakkan Yarrow di dekat sumber dengan laju dosis yang diketahui: nilai yang ditunjukkan perangkat lain, atau sumber bersertifikat. Masukkan nilai tersebut."
#define S_CW_COUNTS    "Hitungan"
#define S_CW_HINT1     "Kesalahan sekitar 3 % pada 1000 hitungan. Pengukuran lebih lama lebih akurat."
#define S_CW_SAT       "Tabung terlalu sibuk: pindahkan sumber menjauh."
#define S_CW_NEEDMORE  "Minimal 100 hitungan diperlukan untuk selesai."
#define S_CW_OLD       "Saat ini"
#define S_CW_NEW       "Baru"
#define S_CW_DIFF      "Ubah"
#define S_CW_UNC       "Ketidakpastian"
#define S_CW_RANGE     "Nilai di luar jangkauan: ukur lagi."
#define S_M_GUIDE    "Panduan darurat"
#define S_M_EVENTS   "Log kejadian"
#define S_M_BUDGET   "Anggaran dosis"
#define S_M_SAMPLE   "Uji sampel"
#define S_M_SCAN     "Pencari medan & kabel"
#define S_M_SESSIONS "Sesi"
#define S_M_CALWIZ   "Wizard kalibrasi"
#define S_M_LOG      "Log & penyimpanan"
#define S_M_THRESH   "Tingkat alarm"
#define S_M_TUBE     "Status tabung"
#define S_M_SELFTEST "Uji mandiri"
#define S_M_USB      "Tautan PC"
#define S_M_SETTINGS "Pengaturan"
#define S_M_THEMES   "Tema"
#define S_M_ABOUT    "Tentang"
#define S_M_CALIB    "Kalibrasi"
#define S_OPEN_SOURCE "Sumber terbuka · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Tetap di dalam", \
     {"Masuk ke bangunan terdekat, ruang bawah tanah, atau ruangan dalam.", \
      "Tutup pintu, jendela, dan ventilasi.", \
      "Matikan pendingin udara dan kipas.", \
      "Dengarkan pengumuman resmi melalui radio atau telepon.", \
      "Jangan keluar sampai ada perintah resmi.", 0, 0}}, \
    {"Jika Anda berada di luar", \
     {"Lepaskan pakaian luar Anda dan segel dalam kantong. Ini menghilangkan sebagian besar kontaminasi.", \
      "Mandi dengan sabun dan air, cuci rambut Anda.", \
      "Bilas mata dan luka dengan air bersih.", \
      "Jangan menggosok atau mengiritasi kulit.", 0, 0, 0}}, \
    {"Air dan makanan", \
     {"Makanan dan minuman dalam kemasan tertutup aman.", \
      "Jangan makan makanan yang terbuka di luar atau hasil kebun.", \
      "Tunggu otoritas berbicara tentang air keran.", \
      "Cuci tangan sebelum makan.", 0, 0, 0}}, \
    {"Kalium iodida", \
     {"Minum hanya jika otoritas menyuruh Anda.", \
      "Ini melindungi tiroid hanya dari iodin radioaktif.", \
      "Untuk hamil dan anak, tanyakan dosis ke dokter atau apoteker.", \
      "Jangan mulai meminumnya sendiri.", 0, 0, 0}}, \
    {"Dapatkan informasi", \
     {"Dengarkan pengumuman radio (FM/AM) dan telepon.", \
      "Nomor darurat: 112 (polisi 110, ambulans 118).", \
      "Daya mungkin gagal: siapkan radio baterai dan ponsel terisi.", 0, 0, 0, 0}}, \
    {"Tabel dosis", \
     {"Latar belakang alami: sekitar 0.1 µSv/h, 2.4 mSv/tahun.", \
      "Batas publik: 1 mSv/tahun.", \
      "Rontgen dada: 0.1 mSv.", \
      "CT scan: 2–10 mSv.", \
      "Pekerja radiasi: 20 mSv/tahun.", \
      "1000 mSv atau lebih sekaligus: sakit radiasi.", 0}}, \
    {"Tingkat alarm", \
     {"Hati-hati: kenaikan di atas latar belakang.", \
      "Bahaya: pindah ke dalam dan baca panduan.", \
      "Evakuasi: ikuti instruksi resmi.", \
      "Anda bisa ubah level di Pengaturan.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "Tahap "
#define S_HELD_BACK    "Ditahan"
#define S_RETRY        "Ulangi"
#define S_SAVER        "Screensaver"
#define S_STATION      "Mode stasiun"
#define S_H_TITLE       "Kesehatan"
#define S_H_STARTS     "Mulai"
#define S_H_THIS       "Mulai ini"
#define S_H_WDT        "Reset Watchdog"
#define S_H_CRASHES    "Crash"
#define S_H_USB        "Restart USB"
#define S_H_RUNNING    "Berjalan"
#define S_H_STARTUP    "Startup"
#define S_H_STAGE      "tahap"
#define S_H_LASTCRASH  "Crash terakhir pada"
#define S_M_DEADTIME   "Wizard Waktu Mati"
#define S_DT_A         "Sumber A saja"
#define S_DT_AB        "Sumber A dan B bersama"
#define S_DT_B         "Sumber B saja"
#define S_DT_NONE      "Tidak ada sumber"
#define S_DT_INTRO     "Membutuhkan dua sumber kekuatan serupa, masing-masing minimal 500 hit/detik, dan tabung tetap di tempat. Empat hit masing-masing dua menit: A, A+B, B, dan tidak ada."
#define S_DT_WARN      "Tanpa dua sumber seperti itu, hasilnya tidak dapat dipercaya, dan wizard menyatakannya."
#define S_DT_PLACE     "Letakkan sumber atau sumber yang disebutkan di atas di depan tabung, lalu mulai. Jangan memindahkan apa pun saat menghitung."
#define S_DT_NORES     "Tidak ada hasil yang dapat digunakan."
#define S_DT_NORES_D   "Sumber terlalu lemah, atau salah satu bergerak. Perbedaan hit tidak jelas lebih besar dari kebisingan mereka."
#define H_REASONS {"tidak dikenal", "daya hidup", "pin reset", "watchdog", "watchdog jendela", "software", "daya rendah", "crash"}
#define S_AXIS_10MIN   "-10 mnt"
#define S_AXIS_60S     "-60 dtk"
#define S_T_LAST7      "7 hari terakhir"
#define S_PEAK_WORD    "Puncak "
#define S_MILLION      " Juta"
#define L_PROFILE_STEPS {"1 dtk", "10 dtk", "1 mnt", "5 mnt", "1 jam"}
#define L_WEEKDAYS     {"Sn", "Sl", "Rb", "Km", "Jm", "Sb", "Mg"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " jam"
#define L_MIN_UNIT     " mnt"
#define L_MONTHS       {"Jan", "Feb", "Mar", "Apr", "Mei", "Jun", "Jul", "Agu", "Sep", "Okt", "Nov", "Des"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "Offline"
