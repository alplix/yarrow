/* User interface text: Korean. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_NORMAL    "정상"
#define S_ATTENTION "주의"
#define S_DANGER    "위험"
#define S_EVACUATE  "대피"
#define S_LOW       "낮음"
#define S_MEDIUM    "중간"
#define S_HIGH      "높음"

/* titles */
#define S_T_RADIATION "방사선"
#define S_T_LIVE      "실시간"
#define S_T_HISTORY   "기록"
#define S_T_EFIELD    "전기장"
#define S_T_HFIELD    "자기장"
#define S_T_BATTERY   "배터리 및 장치"
#define S_T_GUIDE     "비상 안내"
#define S_T_SETTINGS  "설정"
#define S_T_EVENTS    "이벤트"
#define S_T_BUDGET    "선량 예산"
#define S_T_SAMPLE    "샘플"
#define S_T_LOG       "로그"
#define S_T_STORAGE   "저장"
#define S_T_THRESH    "경보 레벨"
#define S_T_TUBE      "튜브 상태"
#define S_T_SELFTEST  "자체 테스트"
#define S_T_USB       "PC 연결"
#define S_T_MODE      "측정 모드"
#define S_T_SCAN      "필드 탐지기"
#define S_T_MENU      "메뉴"
#define S_T_THEMES    "테마"
#define S_T_ABOUT     "정보"
#define S_T_CLOCK     "시간 및 날짜"
#define S_T_CALIB     "교정"
#define S_CLOCK_DATE  "시각/날짜"
#define S_DAY    "일"
#define S_MONTH  "월"
#define S_YEAR   "연"
#define S_HOUR   "시"
#define S_MINUTE "분"
#define S_NEXT   "다음"
#define S_SAVE   "저장"
#define S_CAL_SENS   "감도"
#define S_CAL_DEAD   "데드 타임"
#define S_CAL_E      "전기 계수"
#define S_CAL_H      "자기 계수"
#define S_CAL_NOTE   "알려진 소스와 비교하여 장치 조정."

/* labels */
#define S_TODAY     "오늘"
#define S_WEEK      "주"
#define S_TOTAL     "총계"
#define S_MIN       "최소"
#define S_AVG       "평균"
#define S_MAX       "최대"
#define S_PEAK      "최고치"
#define S_AVERAGE   "평균"
#define S_NOW       "현재"
#define S_REC_ON    "기록 중"
#define S_BAT_LEFT  "배터리"
#define S_LAST_10   "지난 10분"
#define S_NORMAL_x  "정상"
#define S_TIMES     "배"
#define S_THRESHOLD "수준"
#define S_SCALE     "눈금"
#define S_DETAILS   "상세"
#define S_OPEN      "열기"
#define S_BACK      "뒤로"
#define S_SELECT    "선택"
#define S_MUTE      "음소거"
#define S_GUIDE     "안내"
#define S_CHANGE    "변경"
#define S_RESET     "초기화"
#define S_FINISH    "완료"
#define S_CANCEL    "취소"
#define S_ON        "켜짐"
#define S_OFF       "꺼짐"
#define S_YES       "예"
#define S_NO        "아니오"
#define S_DONE      "완료"

/* battery screen */
#define S_REMAINING "남은 시간"
#define S_HEALTH    "배터리 상태"
#define S_CYCLES    "충전 주기"
#define S_RUNTIME   "총 사용 시간"
#define S_TUBE_LIFE "튜브 수명"
#define S_HOURS     "h"
#define S_MINUTES   "분"
#define S_CHARGING  "충전 중"
#define S_FULL_BAT  "배터리 만충"
#define S_PRESS_ANY "키 누름"
#define S_CHARGED   "완전"
#define S_NO_USB    "USB 없음"

/* alarm */
#define S_STEP1 "실내로 이동"
#define S_STEP2 "문과 창문 닫기"
#define S_STEP3 "뉴스 청취"

/* settings */
#define S_THEME      "테마"
#define S_BRIGHTNESS "밝기"
#define S_SOUND      "경보음"
#define S_VIBRATION  "경보 진동"
#define S_LANGUAGE   "언어"
#define S_ATT_TH     "주의 수준"
#define S_CLICK      "카운트 클릭"
#define S_COUNT_VIBE "카운트 진동"
#define S_USB_START  "USB 전원 시작"
#define S_LED_COLOR  "LED 색상"
#define LED_COLOR_NAMES {"테마", "빨강", "초록", "파랑", "노랑", "시안", "마젠타", "흰색"}
#define S_LED_PULSE  "카운트 LED"
#define S_LED_CHARGE "충전 LED"
#define S_TIME_ZONE  "시간대"
#define S_TH_ATTENTION "주의"
#define S_TH_DANGER    "위험"
#define S_TH_EVACUATE  "대피"
#define S_SOUND_LIGHT  "소리 · 진동 · 빛"
#define S_TURKISH    "터키어"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"다크", "라이트", "종이", "앰버", "사쿠라", "나이트", "터미널", "오션", "포레스트", "대비", "라벤더", "민트", "선셋", "차콜", "샌드", "아이스", "체리", "바이올렛", "캔디", "모노", "크림슨"}
#define S_THEME_DARK   "다크"
#define S_THEME_LIGHT  "라이트"
#define S_THEME_PAPER  "페이퍼"
#define S_THEME_AMBER  "앰버"
#define S_THEME_SAKURA "사쿠라"

/* log and storage */
#define S_PROFILE    "프로필"
#define S_P_DETAIL   "상세"
#define S_P_DENSE    "밀집"
#define S_P_BALANCED "중간"
#define S_P_LONG     "장기"
#define S_P_ARCHIVE  "보관"
#define S_DAYS       "일"
#define S_MONTHS     "월"
#define S_YEARS      "년"
#define S_OLDEST     "가장 오래된 기록"
#define S_WHEN_FULL  "가득 찼을 때"
#define S_OVERWRITE  "덮어쓰기"
#define S_FULL_PCT   "가득 참"
#define S_FREE       "무료"
#define S_RECORDED   "기록 일수"
#define S_ALARM_LOG  "경보 기록"
#define S_AUTO_NOTE  "경보 순간도 매초 저장됩니다."
#define S_USB_DOWNLOAD "USB: 기록 다운로드"

/* sample test, budget, scan */
#define S_BACKGROUND "배경"
#define S_SAMPLE     "샘플"
#define S_NET_DIFF   "순 차이"
#define S_RESULT     "결과"
#define S_HIGHER     "배경보다 명확히 높음"
#define S_SAME       "배경과 동일함"
#define S_MEASURE_TIME "시간"
#define S_GOAL       "목표"
#define S_MISSION    "예산"
#define S_START      "시작됨"
#define S_DURATION   "지속 시간"
#define S_LEFT       "남은"
#define S_USED_PCT   "사용량"
#define S_ON_EXCEED  "한계 초과: 경보"
#define S_NEW_BUDGET "새 한계"
#define S_FIND_SRC   "근접할수록 소리가 커지고 막대가 높아집니다."
#define S_APPROACH   "근접 중"
#define S_SOUND_ON   "소리 켜짐"
#define S_SCAN_SOUND "탐지음"
#define S_MODE_NAV   "모드"
#define S_EXIT       "나가기"

/* tube, self test, usb */
#define S_HV         "고전압"
#define S_HV_FACTORY "공장 값"
#define S_TOTAL_PULSES "총 펄스"
#define S_BG_RATE    "배경"
#define S_DEADTIME   "데드 타임 보정"
#define S_HV_PROTECTED "고전압 보호"
#define S_CHK_HV      "고전압"
#define S_CHK_TUBE    "튜브 펄스"
#define S_CHK_BAT     "배터리 잔량"
#define S_CHK_E       "전기 센서"
#define S_CHK_H       "자기 센서"
#define S_CHK_CLOCK   "시계"
#define S_CHK_MEM     "로그 메모리"
#define S_USB_STATE   "연결됨"
#define S_STREAM      "실시간 스트림"
#define S_CHARGE_A    "전하"
#define S_CLOCK_SYNC  "시계"
#define S_SYNCED      "동기화됨"
#define S_RECORDS     "기록"
#define S_TRANSFER    "전송"
#define S_ALL_ON_PC   "PC로 모든 기록 전송"
#define S_DOWNLOAD_NOW "지금 다운로드"
#define S_AUTO_ON     "자동: 켜짐"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "기록"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"핵 비상사태 시 대처 요령.", "과거 고선량 이벤트 목록.", \
    "선택 선량 사용량 표시", "여행 또는 산책별 분리된 선량 기록", \
    "물체 방사능 검사", "소리 및 막대로 전선과 필드 소스 탐지", \
    "기록 시간 및 메모리 용량.", "경고 및 알람 수준 설정.", \
    "튜브 상태, 데드 타임, 고전압.", "장치 부품을 하나씩 점검합니다.", \
    "컴퓨터에 연결하여 기록을 이동합니다.", "화면 색상 테마를 선택합니다.", \
    "밝기, 소리, 진동 및 시계.", "민감도 및 필드 계수를 수동으로 설정합니다.", \
    "알려진 소스를 사용하여 민감도를 설정합니다.", "버전 및 프로젝트 링크."}
#define SETTINGS_DESC {"화면 색상 테마 선택.", "화면 밝기를 낮추면 배터리 수명이 길어집니다.", \
    "알람 및 경고음 발생 여부.", "알람 시 장치 진동 여부.", \
    "고전 가이거 계수기처럼 튜브 카운트마다 클릭.", "카운트마다 모터가 짧게 진동합니다.", \
    "카운트 시 LED 깜박임. 끄면 전력 절약.", "배터리 충전 시 LED 점등.", \
    "카운트/충전 LED 색상. 테마는 화면 강조색을 따름.", \
    "USB 전원 도착 시 자동 켜짐 여부. 아니면 전원 버튼을 길게 누름.", \
    "시계 표시 시간대 설정.", "시간 및 날짜 수동 설정."}
/* sessions */
#define S_T_SESSIONS   "세션"
#define S_SESS_NEW     "새 세션 시작"
#define S_SESS_RUNNING "실행 중인 세션"
#define S_SESS_NONE    "저장된 세션 없음."
#define S_SESS_PICK    "세션 유형 선택"
#define S_SESS_HINT    "세션 중 선량, 시간, 최고율 기록. 전원을 꺼도 계속 기록됩니다."
#define SESS_KINDS_NAMES  {"여정 / 비행", "도보", "실내 조사", "실외", "샘플 주변 측정", "일반"}
#define S_SESS_DOSE    "누적 선량"
#define S_SESS_TIME    "지속 시간"
#define S_SESS_AVG     "평균"
#define S_SESS_MAX     "최고치"
#define S_SESS_NOW     "현재"
#define S_SESS_ASK     "완료하려면 OK를 다시 누르세요"
#define S_FINISH       "완료"
#define S_BEGIN        "시작"
#define S_APPLY        "적용"
#define S_DISCARD      "취소"
#define S_BROWSE       "탐색"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "자연 선량 일수"
#define S_NATURAL_LESS "자연 선량 일수 미만"
/* calibration wizard */
#define S_CW_T0        "기준 선량률"
#define S_CW_T1        "측정 중"
#define S_CW_T2        "결과"
#define S_CW_HELP0     "알려진 선량률 소스 옆에 Yarrow를 배치하십시오. 다른 장치 값 또는 인증된 소스를 사용하고 해당 값을 입력하세요."
#define S_CW_COUNTS    "카운트"
#define S_CW_HINT1     "1000 카운트에서 약 3% 오차. 측정 시간이 길수록 정확도가 높아집니다."
#define S_CW_SAT       "튜브 과부하: 소스를 멀리 이동하세요."
#define S_CW_NEEDMORE  "완료를 위해 최소 100 카운트가 필요합니다."
#define S_CW_OLD       "현재"
#define S_CW_NEW       "새"
#define S_CW_DIFF      "변경"
#define S_CW_UNC       "불확도"
#define S_CW_RANGE     "범위 초과: 재측정"
#define S_M_GUIDE    "비상 안내"
#define S_M_EVENTS   "이벤트 로그"
#define S_M_BUDGET   "선량 예산"
#define S_M_SAMPLE   "샘플 테스트"
#define S_M_SCAN     "필드/와이어 탐지"
#define S_M_SESSIONS "세션"
#define S_M_CALWIZ   "교정 마법사"
#define S_M_LOG      "로그 및 저장"
#define S_M_THRESH   "경보 레벨"
#define S_M_TUBE     "튜브 상태"
#define S_M_SELFTEST "자체 테스트"
#define S_M_USB      "PC 연결"
#define S_M_SETTINGS "설정"
#define S_M_THEMES   "테마"
#define S_M_ABOUT    "정보"
#define S_M_CALIB    "교정"
#define S_OPEN_SOURCE "오픈 소스 · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"실내에 머무르세요", \
     {"가능한 경우 가장 가까운 건물, 지하실 또는 내부 방으로 이동하십시오.", \
      "문, 창문 및 환기 장치를 닫으십시오.", \
      "에어컨과 팬을 끄십시오.", \
      "라디오나 전화로 공식 발표를 들으십시오.", \
      "당국이 지시할 때까지 외출하지 마십시오.", 0, 0}}, \
    {"외부에 있었다면", \
     {"겉옷을 벗고 봉투에 밀봉하세요. 대부분의 오염 물질이 제거됩니다.", \
      "비누와 물로 샤워하고 머리를 감으세요.", \
      "깨끗한 물로 눈과 상처를 헹구세요.", \
      "피부를 문지르거나 자극하지 마십시오.", 0, 0, 0}}, \
    {"물과 음식", \
     {"밀봉된 포장재의 음식과 음료는 안전합니다.", \
      "밖에 노출된 음식이나 텃밭 작물은 먹지 마십시오.", \
      "수돗물에 대해 당국의 발표를 기다리십시오.", \
      "식사 전 손을 씻으십시오.", 0, 0, 0}}, \
    {"요오드화 칼륨", \
     {"당국이 지시할 때만 복용하십시오.", \
      "이는 방사성 요오드에 대해서만 갑상선을 보호합니다.", \
      "임산부 및 어린이의 경우, 복용량에 대해 의사나 약사에게 문의하십시오.", \
      "임의로 복용을 시작하지 마십시오.", 0, 0, 0}}, \
    {"정보 보기", \
     {"라디오(FM/AM) 및 전화 공지 청취.", \
      "긴급 전화: 119(소방·구급), 112(경찰).", \
      "전원 상실 가능: 배터리 라디오와 충전된 휴대폰을 준비하세요.", 0, 0, 0, 0}}, \
    {"선량표", \
     {"자연 배경: 약 0.1 µSv/h, 연간 2.4 mSv.", \
      "일반인 한도: 연간 1 mSv.", \
      "흉부 X-ray: 0.1 mSv.", \
      "CT 촬영: 2–10 mSv.", \
      "방사선 작업자: 연간 20 mSv.", \
      "한 번에 1000 mSv 이상: 방사선 질병.", 0}}, \
    {"경보 수준", \
     {"주의: 배경치 이상 상승.", \
      "위험: 실내로 이동하고 안내서를 읽으십시오.", \
      "대피: 공식 지침을 따르십시오.", \
      "설정에서 레벨을 변경할 수 있습니다.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "단계 "
#define S_HELD_BACK    "보류됨"
#define S_RETRY        "재시도"
#define S_AXIS_10MIN   "-10 분"
#define S_AXIS_60S     "-60 초"
#define S_T_LAST7      "최근 7일"
#define S_PEAK_WORD    "최고치 "
#define S_MILLION      " 백만"
#define L_PROFILE_STEPS {"1 초", "10 초", "1 분", "5 분", "1 시간"}
#define L_WEEKDAYS     {"월", "화", "수", "목", "금", "토", "일"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    "시간"
#define L_MIN_UNIT     "분"
#define L_MONTHS       {"1월", "2월", "3월", "4월", "5월", "6월", "7월", "8월", "9월", "10월", "11월", "12월"}
#define L_DATE_MON_FIRST 1
#define L_DAY_SUFFIX   "일"
#define S_OFFLINE      "오프라인"
