/* User interface text: Brazilian Portuguese. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */

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
#define S_ATTENTION "ATENÇÃO"
#define S_DANGER    "PERIGO"
#define S_EVACUATE  "EVACUAR"
#define S_LOW       "BAIXO"
#define S_MEDIUM    "MÉDIO"
#define S_HIGH      "ALTO"

/* titles */
#define S_T_RADIATION "RADIAÇÃO"
#define S_T_LIVE      "AO VIVO"
#define S_T_HISTORY   "HISTÓRICO"
#define S_T_EFIELD    "CAMPO ELÉTRICO"
#define S_T_HFIELD    "CAMPO MAGNÉTICO"
#define S_T_BATTERY   "BATERIA E DISP."
#define S_T_GUIDE     "GUIA DE EMERG."
#define S_T_SETTINGS  "Configurações"
#define S_T_EVENTS    "Eventos"
#define S_T_BUDGET    "Orçamento de dose"
#define S_T_SAMPLE    "Amostra"
#define S_T_LOG       "Log"
#define S_T_STORAGE   "Armazenamento"
#define S_T_THRESH    "Níveis de alarme"
#define S_T_TUBE      "Status do tubo"
#define S_T_SELFTEST  "Auto teste"
#define S_T_USB       "Link PC"
#define S_T_MODE      "Modo de medição"
#define S_T_SCAN      "Localizador"
#define S_T_MENU      "Menu"
#define S_T_THEMES    "Temas"
#define S_T_ABOUT     "Sobre"
#define S_T_CLOCK     "Hora e data"
#define S_T_CALIB     "Calibração"
#define S_CLOCK_DATE  "Hora e data"
#define S_DAY    "Dia"
#define S_MONTH  "Mês"
#define S_YEAR   "Ano"
#define S_HOUR   "Hora"
#define S_MINUTE "Minuto"
#define S_NEXT   "Próximo"
#define S_SAVE   "Salvar"
#define S_CAL_SENS   "Sensibilidade"
#define S_CAL_DEAD   "Tempo morto"
#define S_CAL_E      "Fator elétrico"
#define S_CAL_H      "Fator Magnético"
#define S_CAL_NOTE   "Ajuste o dispositivo comparando-o com uma fonte conhecida."

/* labels */
#define S_TODAY     "Hoje"
#define S_WEEK      "Semana"
#define S_TOTAL     "Total"
#define S_MIN       "Mín"
#define S_AVG       "Méd"
#define S_MAX       "Máx"
#define S_PEAK      "Pico"
#define S_AVERAGE   "Média"
#define S_NOW       "Agora"
#define S_REC_ON    "Gravando"
#define S_BAT_LEFT  "Bateria"
#define S_LAST_10   "Últimos 10 min"
#define S_NORMAL_x  "de normal"
#define S_TIMES     "vezes"
#define S_THRESHOLD "nível"
#define S_SCALE     "escala"
#define S_DETAILS   "detalhes"
#define S_OPEN      "abrir"
#define S_BACK      "voltar"
#define S_SELECT    "Escolher"
#define S_MUTE      "mudo"
#define S_GUIDE     "guia"
#define S_CHANGE    "mudar"
#define S_RESET     "resetar"
#define S_FINISH    "finalizar"
#define S_CANCEL    "cancelar"
#define S_ON        "Ligado"
#define S_OFF       "Desligado"
#define S_YES       "Sim"
#define S_NO        "Não"
#define S_DONE      "OK"

/* battery screen */
#define S_REMAINING "Tempo restante"
#define S_HEALTH    "Saúde da bateria"
#define S_CYCLES    "Ciclos de carga"
#define S_RUNTIME   "Tempo total de uso"
#define S_TUBE_LIFE "Vida útil do tubo"
#define S_HOURS     "h"
#define S_MINUTES   "min"
#define S_CHARGING  "Carregando"
#define S_FULL_BAT  "Bateria cheia"
#define S_PRESS_ANY "Pressione qualquer tecla"
#define S_CHARGED   "Cheio"
#define S_NO_USB    "Sem USB"

/* alarm */
#define S_STEP1 "Entre"
#define S_STEP2 "Feche portas e janelas"
#define S_STEP3 "Ouça as notícias"

/* settings */
#define S_THEME      "Tema"
#define S_BRIGHTNESS "Brilho"
#define S_SOUND      "Som do alarme"
#define S_VIBRATION  "Vibração do alarme"
#define S_LANGUAGE   "Idioma"
#define S_ATT_TH     "Nível de alerta"
#define S_CLICK      "Contagem"
#define S_COUNT_VIBE "Contagem vibração"
#define S_USB_START  "Iniciar com energia USB"
#define S_LED_COLOR  "Cor LED"
#define LED_COLOR_NAMES {"Tema", "Vermelho", "Verde", "Azul", "Amarelo", "Ciano", "Magenta", "Branco"}
#define S_LED_PULSE  "LED Contagem"
#define S_LED_CHARGE "LED Carga"
#define S_TIME_ZONE  "Fuso horário"
#define S_TH_ATTENTION "Cuidado"
#define S_TH_DANGER    "Perigo"
#define S_TH_EVACUATE  "Evacuar"
#define S_SOUND_LIGHT  "som · vibração · luz"
#define S_TURKISH    "Turco"
/* theme names, in the order of the theme table */
#define THEME_NAMES {"Escuro", "Claro", "Papel", "Âmbar", "Sakura", "Noite", "Terminal", "Oceano", "Floresta", "Contraste", "Lavanda", "Menta", "Sol Poente", "Carvão", "Areia", "Gelo", "Cereja", "Violeta", "Doce", "Mono", "Carmesim"}
#define S_THEME_DARK   "Escuro"
#define S_THEME_LIGHT  "Claro"
#define S_THEME_PAPER  "Papel"
#define S_THEME_AMBER  "Âmbar"
#define S_THEME_SAKURA "Sakura"

/* log and storage */
#define S_PROFILE    "Perfil"
#define S_P_DETAIL   "Detalhado"
#define S_P_DENSE    "Denso"
#define S_P_BALANCED "Médio"
#define S_P_LONG     "Longo"
#define S_P_ARCHIVE  "Arquivo"
#define S_DAYS       "dias"
#define S_MONTHS     "meses"
#define S_YEARS      "anos"
#define S_OLDEST     "Registro mais antigo"
#define S_WHEN_FULL  "Cheio"
#define S_OVERWRITE  "Trocar"
#define S_FULL_PCT   "Cheio"
#define S_FREE       "livre"
#define S_RECORDED   "dias de registros"
#define S_ALARM_LOG  "Alarmes"
#define S_AUTO_NOTE  "momentos de alarme são salvos a cada segundo."
#define S_USB_DOWNLOAD "USB: baixar registros"

/* sample test, budget, scan */
#define S_BACKGROUND "fundo"
#define S_SAMPLE     "amostra"
#define S_NET_DIFF   "diferença líquida"
#define S_RESULT     "resultado"
#define S_HIGHER     "claramente acima do fundo"
#define S_SAME       "igual ao fundo"
#define S_MEASURE_TIME "tempo"
#define S_GOAL       "meta"
#define S_MISSION    "orçamento"
#define S_START      "Iniciado"
#define S_DURATION   "Duração"
#define S_LEFT       "Restante"
#define S_USED_PCT   "Usado"
#define S_ON_EXCEED  "Limite: Alarme."
#define S_NEW_BUDGET "Novo Lmt."
#define S_FIND_SRC   "Quanto mais perto da fonte, mais alto o tom e mais altas as barras."
#define S_APPROACH   "Aproximando-se"
#define S_SOUND_ON   "Som ligado"
#define S_SCAN_SOUND "Som de busca"
#define S_MODE_NAV   "Modo"
#define S_EXIT       "Sair"

/* tube, self test, usb */
#define S_HV         "Alta tensão"
#define S_HV_FACTORY "Valor de fábrica"
#define S_TOTAL_PULSES "Pulsos Totais"
#define S_BG_RATE    "Fundo"
#define S_DEADTIME   "Correção de tempo morto"
#define S_HV_PROTECTED "Protegido HV"
#define S_CHK_HV      "Alta tensão"
#define S_CHK_TUBE    "Pulsos do tubo"
#define S_CHK_BAT     "Leitura da bateria"
#define S_CHK_E       "Sensor elétrico"
#define S_CHK_H       "Sensor magnético"
#define S_CHK_CLOCK   "Relógio"
#define S_CHK_MEM     "Memória de log"
#define S_USB_STATE   "CONECTADO"
#define S_STREAM      "Fluxo ao vivo"
#define S_CHARGE_A    "Carga"
#define S_CLOCK_SYNC  "Relógio"
#define S_SYNCED      "Sincronizado"
#define S_RECORDS     "Registros"
#define S_TRANSFER    "Transferir"
#define S_ALL_ON_PC   "Todos os registros no PC"
#define S_DOWNLOAD_NOW "Baixar"
#define S_AUTO_ON     "Auto: ligado"
#define S_RANGE_E     "1–1999 V/m"
#define S_RANGE_H     "0.01–99.99 µT"

/* event list */
#define S_EVENT_COUNT "Registros"

/* one-line explanations shown under the menu, same order as the menu */
#define MENU_DESC {"O que fazer em uma emergência nuclear.", "Lista de eventos de alta dose anteriores.", \
    "Mostra quanto da dose escolhida foi utilizada.", "Registros de dose separados para viagem ou caminhada.", \
    "Verifica se um objeto emite radiação.", "Localiza fios e fontes de campo por som e barras.", \
    "Tempo de gravação e nível de memória.", "Definir níveis de aviso e alarme.", \
    "Condição do tubo, tempo morto, alta tensão.", "Verifica as partes do dispositivo sequencialmente.", \
    "Conectar a um computador e transferir registros.", "Escolher o tema de cores da tela.", \
    "Brilho, som, vibração e relógio.", "Ajustar sensibilidade e fatores de campo manualmente.", \
    "Define a sensibilidade com uma fonte conhecida.", "Versão e links do projeto."}
#define SETTINGS_DESC {"Escolher o tema de cores da tela.", "Uma tela com brilho reduzido prolonga a bateria.", \
    "Alarme e aviso devem emitir som.", "O dispositivo deve vibrar em alarme.", \
    "Um clique por contagem de tubo, como nos contadores Geiger clássicos.", "O motor emite um bipe curto a cada contagem.", \
    "O LED pisca em cada contagem. Desligá-lo economiza energia.", "O LED acende enquanto a bateria carrega.", \
    "Cor do LED de contagem e carga. O tema segue a cor de destaque da tela.", \
    "O dispositivo deve ligar ao receber energia USB? Se não, segure a tecla de energia para iniciar.", \
    "Qual fuso horário o relógio exibe.", "Definir hora e data manualmente."}
/* sessions */
#define S_T_SESSIONS   "Sessões"
#define S_SESS_NEW     "Iniciar nova sessão"
#define S_SESS_RUNNING "Sessão em execução"
#define S_SESS_NONE    "Nenhuma sessão salva ainda."
#define S_SESS_PICK    "Escolher o tipo de sessão"
#define S_SESS_HINT    "Durante a sessão, dose, tempo e taxa máxima são registrados. Continua mesmo se desligar o dispositivo."
#define SESS_KINDS_NAMES  {"Viagem / Voo", "Caminhada", "Pesquisa interna", "Externo", "Ao redor de amostra", "Geral"}
#define S_SESS_DOSE    "Dose recebida"
#define S_SESS_TIME    "Duração"
#define S_SESS_AVG     "Média"
#define S_SESS_MAX     "Máximo"
#define S_SESS_NOW     "Agora"
#define S_SESS_ASK     "Pressione OK para finalizar"
#define S_FINISH       "Finalizar"
#define S_BEGIN        "Iniciar"
#define S_APPLY        "Aplicar"
#define S_DISCARD      "Descartar"
#define S_BROWSE       "Navegar"
#define S_APPROX       "≈ "
#define S_NATURAL_DAYS "Dias de dose natural"
#define S_NATURAL_LESS "Menos de um dia de dose natural"
/* calibration wizard */
#define S_CW_T0        "Taxa de dose de referência"
#define S_CW_T1        "Medindo"
#define S_CW_T2        "Resultado"
#define S_CW_HELP0     "Coloque o Yarrow perto de uma fonte cuja taxa de dose você conhece: o valor de outro dispositivo ou uma fonte certificada. Insira esse valor."
#define S_CW_COUNTS    "Contagens"
#define S_CW_HINT1     "Erro de cerca de 3% em 1000 contagens. Uma medição mais longa é mais precisa."
#define S_CW_SAT       "O tubo está muito ocupado: afaste a fonte."
#define S_CW_NEEDMORE  "Pelo menos 100 contagens são necessárias para terminar."
#define S_CW_OLD       "Atual"
#define S_CW_NEW       "Novo"
#define S_CW_DIFF      "Mudar"
#define S_CW_UNC       "Incerteza"
#define S_CW_RANGE     "Valor fora da faixa: meça novamente."
#define S_M_GUIDE    "Guia de emergência"
#define S_M_EVENTS   "Log de Eventos"
#define S_M_BUDGET   "Limite de dose"
#define S_M_SAMPLE   "Teste de amostra"
#define S_M_SCAN     "Busca de campo e fio"
#define S_M_SESSIONS "Sessões"
#define S_M_CALWIZ   "Assistente de calibração"
#define S_M_LOG      "Log e Armazenamento"
#define S_M_THRESH   "Níveis de alarme"
#define S_M_TUBE     "Status do tubo"
#define S_M_SELFTEST "Auto teste"
#define S_M_USB      "Link PC"
#define S_M_SETTINGS "Configurações"
#define S_M_THEMES   "Temas"
#define S_M_ABOUT    "Sobre"
#define S_M_CALIB    "Calibração"
#define S_OPEN_SOURCE "Código aberto · GPL-3.0"

/* emergency guide: GUIDE_PAGES pages of a title and up to seven lines (see guide_page_t in strings.h) */
#define GUIDE_DATA {\
    {"Fique dentro", \
     {"Vá ao prédio mais próximo, de preferência ao porão.", \
      "Feche portas, janelas e ventilação.", \
      "Desligue ar condicionado e ventiladores.", \
      "Ouça anúncios oficiais no rádio ou telefone.", \
      "Não saia até serem orientados.", 0, 0}}, \
    {"Se você estava fora", \
     {"Retire suas roupas externas e sele-as em um saco. Isso remove a maior parte da contaminação.", \
      "Tome banho com sabão e água, lave o cabelo.", \
      "Lave olhos e feridas com água limpa.", \
      "Não esfregue ou irrite a pele.", 0, 0, 0}}, \
    {"Água e alimentos", \
     {"Alimentos e bebidas em embalagens seladas são seguros.", \
      "Não coma alimentos expostos ou produtos de jardim.", \
      "Aguarde as autoridades sobre a água da torneira.", \
      "Lave as mãos antes de comer.", 0, 0, 0}}, \
    {"Iodeto de potássio", \
     {"Tome apenas quando as autoridades indicarem.", \
      "Protege a tireoide apenas contra iodo radioativo.", \
      "Para gestantes e crianças, consulte médico ou farmacêutico sobre a dose.", \
      "Não comece a tomar por conta própria.", 0, 0, 0}}, \
    {"Obter informações", \
     {"Ouça rádio (FM/AM) e anúncios telefônicos.", \
      "Número de emergência: 192 (SAMU), 193 (bombeiros), 112 (Portugal).", \
      "Falha de energia: mantenha rádio a pilha e celular carregado.", 0, 0, 0, 0}}, \
    {"Tabela de dose", \
     {"Fundo natural: cerca de 0.1 µSv/h, 2.4 mSv/ano.", \
      "Limite público: 1 mSv/ano.", \
      "Raio-X de tórax: 0.1 mSv.", \
      "Tomografia: 2–10 mSv.", \
      "Trabalhador em radiação: 20 mSv/ano.", \
      "1000 mSv ou mais de uma vez: doença por radiação.", 0}}, \
    {"Níveis de alarme", \
     {"Cuidado: aumento acima do fundo.", \
      "Perigo: entre em casa e leia o guia.", \
      "Evacuar: siga as instruções oficiais.", \
      "Você pode mudar os níveis em Configurações.", 0, 0, 0}}, \
}

/* words and formats that used to sit in the C sources */
#define S_STAGE_WORD   "estágio "
#define S_HELD_BACK    "Retido"
#define S_RETRY        "Repetir"
#define S_AXIS_10MIN   "-10 min"
#define S_AXIS_60S     "-60 s"
#define S_T_LAST7      "Últimos 7 dias"
#define S_PEAK_WORD    "pico "
#define S_MILLION      " milhão"
#define L_PROFILE_STEPS {"1 s", "10 s", "1 min", "5 min", "1 h"}
#define L_WEEKDAYS     {"Sg", "Te", "Qa", "Qi", "Sx", "Sá", "Do"} /* Monday first */

/* time and date: a duration is "<number><unit>", a date is "<day> <month>" (L_DATE_MON_FIRST 0) or "<month> <day><suffix>" */
#define L_HOUR_UNIT    " h"
#define L_MIN_UNIT     " min"
#define L_MONTHS       {"jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"}
#define L_DATE_MON_FIRST 0
#define L_DAY_SUFFIX   ""
#define S_OFFLINE      "offline"
