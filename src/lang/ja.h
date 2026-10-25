/* User interface text: Japanese. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_NORMAL    "正常"
#define S_ATTENTION "注意"
#define S_DANGER    "危険"
#define S_EVACUATE  "避難"
#define S_LOW       "低"
#define S_MEDIUM    "中"
#define S_HIGH      "高"

/* titles */
#define S_T_RADIATION "放射線"
#define S_T_LIVE      "測定"
#define S_T_HISTORY   "履歴"
#define S_T_EFIELD    "電場"
#define S_T_HFIELD    "磁場"
#define S_T_BATTERY   "バッテリー/機器"
#define S_T_GUIDE     "緊急ガイド"
#define S_T_SETTINGS  "設定"
#define S_T_EVENTS    "イベント"
#define S_T_BUDGET    "線量予算"
#define S_T_SAMPLE    "サンプル"
#define S_T_LOG       "ログ"
#define S_T_STORAGE   "ストレージ"
#define S_T_THRESH    "アラームレベル"
#define S_T_TUBE      "チューブ状態"
#define S_T_SELFTEST  "自己診断"
#define S_T_USB       "PCリンク"
#define S_T_MODE      "測定モード"
#define S_T_SCAN      "フィールド"
#define S_T_MENU      "メニュー"
#define S_T_THEMES    "テーマ"
#define S_T_ABOUT     "概要"
#define S_T_CLOCK     "日付と時刻"
#define S_T_CALIB     "校正"
#define S_CLOCK_DATE  "日時"
#define S_DAY    "日"
#define S_MONTH  "月"
#define S_YEAR   "年"
#define S_HOUR   "時"
#define S_MINUTE "分"
#define S_NEXT   "次へ"
#define S_SAVE   "保存"
#define S_CAL_SENS   "感度"
#define S_CAL_DEAD   "デッドタイム"
#define S_CAL_E      "電圧係数"
#define S_CAL_H      "磁気係数"
#define S_CAL_NOTE   "既知の源と比較して装置を調整してください。"

/* labels */
#define S_TODAY     "今日"
#define S_WEEK      "週"
#define S_TOTAL     "合計"
#define S_MIN       "最小"
#define S_AVG       "平均"
#define S_MAX       "最大"
#define S_PEAK      "ピーク"
#define S_AVERAGE   "平均"
#define S_NOW       "現在"
#define S_REC_ON    "記録中"
#define S_BAT_LEFT  "バッテリー"
#define S_LAST_10   "過去10分"
#define S_NORMAL_x  "通常の"
#define S_TIMES     "倍"
#define S_THRESHOLD "レベル"
#define S_SCALE     "スケール"
#define S_DETAILS   "詳細"
#define S_OPEN      "開く"
#define S_BACK      "戻る"
#define S_SELECT    "選択"
#define S_MUTE      "消音"
#define S_GUIDE     "ガイド"
#define S_CHANGE    "変更"
#define S_RESET     "リセット"
#define S_FINISH    "完了"
#define S_CANCEL    "キャンセル"
#define S_ON        "オン"
#define S_OFF       "オフ"
#define S_YES       "はい"
#define S_NO        "いいえ"
#define S_DONE      "完了"

/* battery screen */
#define S_REMAINING "残り時間"
#define S_HEALTH    "バッテリー状態"
#define S_CYCLES    "充電サイクル数"
#define S_RUNTIME   "総稼働時間"
#define S_TUBE_LIFE "チューブ寿命"
#define S_HOURS     "h"
#define S_MINUTES   "分"
#define S_CHARGING  "充電中"
#define S_FULL_BAT  "バッテリー満充電"
#define S_PRESS_ANY "キーを押す"
#define S_CHARGED   "フル"
#define S_NO_USB    "USBなし"

/* alarm */
#define S_STEP1 "屋内へ"
#define S_STEP2 "ドアと窓を閉めてください"
#define S_STEP3 "ニュースを聞いてください"

/* settings */
#define S_THEME      "テーマ"
#define S_BRIGHTNESS "明るさ"
#define S_SOUND      "アラーム音"
#define S_VIBRATION  "アラーム振動"
#define S_LANGUAGE   "言語"
#define S_ATT_TH     "注意レベル"
#define S_CLICK      "カウントクリック"
#define S_COUNT_VIBE "カウント振動"
#define S_USB_START  "USB電源で起動"
#define S_LED_COLOR  "LED色"
#define LED_COLOR_NAMES {"テーマ", "赤", "緑", "青", "黄色", "シアン", "マゼンタ", "白"}
#define S_LED_PULSE  "カウントLED"
#define S_LED_CHARGE "照射LED"
#define S_TIME_ZONE  "タイムゾーン"
#define S_TH_ATTENTION "注意"
#define S_TH_DANGER    "危険"
#define S_TH_EVACUATE  "避難"
#define S_SOUND_LIGHT  "音・振動・光"
#define S_TURKISH    "トルコ語"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"ダーク", "ライト", "ペーパー", "アンバー", "桜", "ナイト", "ターミナル", "オーシャン", "フォレスト", "コントラスト", "ラベンダー", "ミント", "サンセット", "チャコール", "サンド", "アイス", "チェリー", "バイオレット", "キャンディ", "モノ", "クリムゾン"}
#define S_THEME_DARK   "ダーク"
#define S_THEME_LIGHT  "ライト"
#define S_THEME_PAPER  "ペーパー"
#define S_THEME_AMBER  "アンバー"
#define S_THEME_SAKURA "サクラ"

/* log and storage */
#define S_PROFILE    "プロファイル"
#define S_P_DETAIL   "詳細"
#define S_P_DENSE    "密"
#define S_P_BALANCED "中"
#define S_P_LONG     "長"
#define S_P_ARCHIVE  "アーカイブ"
#define S_DAYS       "日"
#define S_MONTHS     "月"
#define S_YEARS      "年"
#define S_OLDEST     "最古記録"
#define S_WHEN_FULL  "満杯時"
#define S_OVERWRITE  "上書き"
#define S_FULL_PCT   "満"
#define S_FREE       "無料"
#define S_RECORDED   "記録日数"
#define S_ALARM_LOG  "アラーム記録"
#define S_AUTO_NOTE  "警報発生時刻も毎秒保存されます。"
#define S_USB_DOWNLOAD "USB: 記録をダウンロード"

/* sample test, budget, scan */
#define S_BACKGROUND "バックグラウンド"
#define S_SAMPLE     "サンプル"
#define S_NET_DIFF   "純差"
#define S_RESULT     "結果"
#define S_HIGHER     "背景より明らかに高い"
#define S_SAME       "背景と同じ"
#define S_MEASURE_TIME "時間"
#define S_GOAL       "目標"
#define S_MISSION    "予算"
#define S_START      "開始"
#define S_DURATION   "時間"
#define S_LEFT       "残り"
#define S_USED_PCT   "使用"
#define S_ON_EXCEED  "制限超過: アラーム"
#define S_NEW_BUDGET "設定値"
#define S_FIND_SRC   "近づくほど、音の高さとバーの高さが上がります。"
#define S_APPROACH   "近づくにつれて"
#define S_SOUND_ON   "音あり"
#define S_SCAN_SOUND "探知音"
#define S_MODE_NAV   "モード"
#define S_EXIT       "終了"

/* tube, self test, usb */
#define S_HV         "高電圧"
#define S_HV_FACTORY "初期値"
#define S_TOTAL_PULSES "総パルス"
#define S_BG_RATE    "背景"
#define S_DEADTIME   "デッドタイム補正"
#define S_HV_PROTECTED "HV保護"
#define S_CHK_HV      "高電圧"
#define S_CHK_TUBE    "チューブパルス"
#define S_CHK_BAT     "バッテリー電圧"
#define S_CHK_E       "電気センサー"
#define S_CHK_H       "磁気センサー"
#define S_CHK_CLOCK   "時計"
#define S_CHK_MEM     "ログメモリ"
#define S_USB_STATE   "接続"
#define S_STREAM      "ライブストリーム"
#define S_CHARGE_A    "充電"
#define S_CLOCK_SYNC  "時計"
#define S_SYNCED      "同期済み"
#define S_RECORDS     "記録"
#define S_TRANSFER    "転送"
#define S_ALL_ON_PC   "全記録をPCに転送"
#define S_DOWNLOAD_NOW "今すぐダウンロード"
#define S_AUTO_ON     "自動: オン"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "記録"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"原子力緊急事態時の対応について。", "過去の高線量イベント一覧。", \
    "設定した線量の使用状況を表示します。", "移動や散歩ごとの線量記録を分ける。", \
    "対象物が放射線を放出しているか確認します。", "音とバーで配線や場源を検出します。", \
    "記録時間とメモリの充填度。", "警告およびアラームレベルを設定します。", \
    "チューブの状態、デッドタイム、高電圧。", "デバイスの各部品を順にチェックします。", \
    "PCに接続し、記録を移動します。", "画面のカラーテーマを選択します。", \
    "明るさ、音、振動、時計。", "感度と場係数を手動で設定します。", \
    "既知の源で感度を設定します。", "バージョンとプロジェクトリンク。"}
#define SETTINGS_DESC {"画面のカラーテーマを選択します。", "ディマー画面はバッテリー寿命を延ばします。", \
    "アラームと警告を音で鳴らすか。", "アラーム時にデバイスを振動させるか。", \
    "各管カウントごとにクリック。古典的なガイガーカウンターと同様です。", "カウントごとにモーターが短いビープ音を発します。", \
    "カウントごとにLEDが点滅します。オフにすると消費電力を節約できます。", "バッテリー充電中はLEDが点灯します。", \
    "カウントおよび充電LEDの色。テーマは画面のアクセントカラーに合わせます。", \
    "USB電源接続時に自動起動するかどうか。自動起動しない場合は電源キーを長押しして起動します。", \
    "時計に表示するタイムゾーンを選択します。", "時間と日付を手動で設定します。", \
    "待機中は画面を消さずに、大きな時計と放射線レベルを表示します。", \
    "常時接続用：画面は30秒後に消え、USBが止まると自動で再起動します。"}
/* sessions */
#define S_T_SESSIONS   "セッション"
#define S_SESS_NEW     "新しいセッションを開始"
#define S_SESS_RUNNING "実行中のセッション"
#define S_SESS_NONE    "保存されたセッションはありません。"
#define S_SESS_PICK    "セッションの種類を選択"
#define S_SESS_HINT    "セッション中は線量、時間、最高レートが記録されます。電源オフ時も記録は継続されます。"
#define SESS_KINDS_NAMES  {"巡回 / フライト", "ウォーク", "屋内調査", "屋外", "サンプル周囲", "一般"}
#define S_SESS_DOSE    "受信線量"
#define S_SESS_TIME    "時間"
#define S_SESS_AVG     "平均"
#define S_SESS_MAX     "最大"
#define S_SESS_NOW     "現在"
#define S_SESS_ASK     "終了するにはOKを再度押す"
#define S_FINISH       "終了"
#define S_BEGIN        "開始"
#define S_APPLY        "適用"
#define S_DISCARD      "破棄"
#define S_BROWSE       "閲覧"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "自然線量日数"
#define S_NATURAL_LESS "自然線量1日未満"
/* calibration wizard */
#define S_CW_T0        "参照線量率"
#define S_CW_T1        "測定中"
#define S_CW_T2        "結果"
#define S_CW_HELP0     "既知線量率の源（他機器の値または認証源）の隣にYarrowを配置し、その値を入力してください。"
#define S_CW_COUNTS    "カウント"
#define S_CW_HINT1     "1000カウントでの誤差は約3%。測定時間が長いほど精度が上がります。"
#define S_CW_SAT       "チューブが過負荷です。源を遠ざけてください。"
#define S_CW_NEEDMORE  "完了には最低100カウントが必要です。"
#define S_CW_OLD       "電流"
#define S_CW_NEW       "新"
#define S_CW_DIFF      "変更"
#define S_CW_UNC       "不確かさ"
#define S_CW_RANGE     "範囲外の値: 再測定してください。"
#define S_M_GUIDE    "緊急ガイド"
#define S_M_EVENTS   "イベントログ"
#define S_M_BUDGET   "線量予算"
#define S_M_SAMPLE   "サンプル試験"
#define S_M_SCAN     "フィールド・ワイヤー検索"
#define S_M_SESSIONS "セッション"
#define S_M_CALWIZ   "校正ウィザード"
#define S_M_LOG      "ログと保存"
#define S_M_THRESH   "アラームレベル"
#define S_M_TUBE     "チューブの状態"
#define S_M_SELFTEST "自己診断"
#define S_M_USB      "PCリンク"
#define S_M_SETTINGS "設定"
#define S_M_THEMES   "テーマ"
#define S_M_ABOUT    "情報"
#define S_M_CALIB    "校正"
#define S_OPEN_SOURCE "オープンソース · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"室内にとどまる", \
     {"最寄りの建物、地下室、または内側の部屋へ避難してください。", \
      "ドア、窓、換気口を閉めてください。", \
      "エアコンとファンを停止してください。", \
      "ラジオや電話で公式発表を聞いてください。", \
      "指示まで外出しないで", 0, 0}}, \
    {"外にいた場合", \
     {"外着を脱ぎ、袋に密閉してください。これにより汚染の大部分が除去されます。", \
      "石鹸と水でシャワーを浴び、髪を洗いましょう。", \
      "目や傷口をきれいな水で洗い流してください。", \
      "皮膚をこすったり刺激したりしないでください。", 0, 0, 0}}, \
    {"水と食料", \
     {"密閉された包装の食品と飲料は安全です。", \
      "屋外に放置された食品や家庭菜園の作物は食べないでください。", \
      "水道水については当局の発表を待ってください。", \
      "食事の前に手を洗いましょう。", 0, 0, 0}}, \
    {"ヨウ化カリウム", \
     {"当局の指示があるときだけ服用してください。", \
      "これは放射性ヨウ素に対してのみ甲状腺を保護します。", \
      "妊娠中や小児の場合、服用量については医師または薬剤師に相談してください。", \
      "自己判断で服用を始めてはいけません。", 0, 0, 0}}, \
    {"情報", \
     {"ラジオ (FM/AM) や電話の放送を聞く。", \
      "緊急通報：119（消防・救急）、110（警察）。", \
      "停電の可能性：電池式ラジオと充電済み携帯電話を準備。", 0, 0, 0, 0}}, \
    {"線量表", \
     {"自然放射線：約 0.1 µSv/h、年間 2.4 mSv。", \
      "一般公開限度：年間 1 mSv。", \
      "胸部X線：0.1 mSv。", \
      "CTスキャン：2–10 mSv。", \
      "放射線作業員：年間 20 mSv。", \
      "一度に 1000 mSv 以上：放射線障害。", 0}}, \
    {"アラームレベル", \
     {"注意：背景値からの上昇。", \
      "危険：屋内に入り、ガイドを読むこと。", \
      "避難：公式指示に従ってください。", \
      "設定でレベルを変更できます。", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "段階 "
#define S_HELD_BACK    "保留中"
#define S_RETRY        "再試行"
#define S_SAVER        "スクリーンセーバー"
#define S_STATION      "ステーションモード"
#define S_AXIS_10MIN   "-10 分"
#define S_AXIS_60S     "-60 秒"
#define S_T_LAST7      "過去7日間"
#define S_PEAK_WORD    "ピーク "
#define S_MILLION      " 百万"
#define L_PROFILE_STEPS {"1 秒", "10 秒", "1 分", "5 分", "1 時間"}
#define L_WEEKDAYS     {"月", "火", "水", "木", "金", "土", "日"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    "時間"
#define L_MIN_UNIT     "分"
#define L_MONTHS       {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"}
#define L_DATE_MON_FIRST 1
#define L_DAY_SUFFIX   "日"
#define S_OFFLINE      "オフライン"
