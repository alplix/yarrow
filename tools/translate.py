#!/usr/bin/env python3
"""Translates src/lang/en.h into another language with a local OpenAI-compatible model server (Tilvar / llama.cpp):

    python tools/translate.py de            translate and write src/lang/de.h
    python tools/translate.py de --fit      then shorten what does not fit on the screen (needs `make sim YLANG=de`)

The server address comes from TILVAR_BASE_URL (default http://127.0.0.1:8080/v1). Units, key hints, dates and the
emergency number are not left to the model: the tables below carry them. Always have a native speaker read the result,
above all the emergency guide."""
import json
import os
import re
import subprocess
import sys
import urllib.request

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
BASE = os.environ.get("TILVAR_BASE_URL", "http://127.0.0.1:8080/v1")

# name for the prompt, short time units (second, minute, hour), month and weekday abbreviations (Monday first),
# whether a date reads "month day", the suffix after the day, emergency line
LANGS = {
    "de": dict(name="German", s="s", m="min", h="Std", months="Jan Feb Mär Apr Mai Jun Jul Aug Sep Okt Nov Dez", wd="Mo Di Mi Do Fr Sa So", mon_first=0, suffix="", hour=" Std", minute=" min",
               emergency="Notruf: 112."),
    "es": dict(name="Spanish", s="s", m="min", h="h", months="Ene Feb Mar Abr May Jun Jul Ago Sep Oct Nov Dic", wd="Lu Ma Mi Ju Vi Sá Do", mon_first=0, suffix="", hour=" h", minute=" min",
               emergency="Número de emergencias: 112."),
    "fr": dict(name="French", s="s", m="min", h="h", months="janv févr mars avr mai juin juil août sept oct nov déc", wd="Lu Ma Me Je Ve Sa Di", mon_first=0, suffix="", hour=" h", minute=" min",
               emergency="Numéro d'urgence : 112."),
    "pt": dict(name="Brazilian Portuguese", s="s", m="min", h="h", months="jan fev mar abr mai jun jul ago set out nov dez", wd="Sg Te Qa Qi Sx Sá Do", mon_first=0, suffix="", hour=" h", minute=" min",
               emergency="Número de emergência: 192 (SAMU), 193 (bombeiros), 112 (Portugal)."),
    "it": dict(name="Italian", s="s", m="min", h="h", months="gen feb mar apr mag giu lug ago set ott nov dic", wd="Lu Ma Me Gi Ve Sa Do", mon_first=0, suffix="", hour=" h", minute=" min",
               emergency="Numero di emergenza: 112."),
    "nl": dict(name="Dutch", s="s", m="min", h="u", months="jan feb mrt apr mei jun jul aug sep okt nov dec", wd="Ma Di Wo Do Vr Za Zo", mon_first=0, suffix="", hour=" u", minute=" min",
               emergency="Alarmnummer: 112."),
    "pl": dict(name="Polish", s="s", m="min", h="godz", months="sty lut mar kwi maj cze lip sie wrz paź lis gru", wd="Pn Wt Śr Cz Pt So Nd", mon_first=0, suffix="", hour=" godz", minute=" min",
               emergency="Numer alarmowy: 112."),
    "id": dict(name="Indonesian", s="dtk", m="mnt", h="jam", months="Jan Feb Mar Apr Mei Jun Jul Agu Sep Okt Nov Des", wd="Sn Sl Rb Km Jm Sb Mg", mon_first=0, suffix="", hour=" jam", minute=" mnt",
               emergency="Nomor darurat: 112 (polisi 110, ambulans 118)."),
    "vi": dict(name="Vietnamese", s="giây", m="phút", h="giờ", months="thg 1|thg 2|thg 3|thg 4|thg 5|thg 6|thg 7|thg 8|thg 9|thg 10|thg 11|thg 12", wd="T2 T3 T4 T5 T6 T7 CN", mon_first=0, suffix="", hour=" giờ", minute=" phút",
               emergency="Số khẩn cấp: 115 (cấp cứu), 114 (cứu hỏa)."),
    "ru": dict(name="Russian", s="с", m="мин", h="ч", months="янв фев мар апр мая июн июл авг сен окт ноя дек", wd="Пн Вт Ср Чт Пт Сб Вс", mon_first=0, suffix="", hour=" ч", minute=" мин",
               emergency="Номер экстренных служб: 112."),
    "uk": dict(name="Ukrainian", s="с", m="хв", h="год", months="січ лют бер кві тра чер лип сер вер жов лис гру", wd="Пн Вт Ср Чт Пт Сб Нд", mon_first=0, suffix="", hour=" год", minute=" хв",
               emergency="Номер екстрених служб: 112."),
    "zh": dict(name="Simplified Chinese", s="秒", m="分", h="时", months="|".join("%d月" % i for i in range(1, 13)), wd="一 二 三 四 五 六 日", mon_first=1, suffix="日", hour="小时", minute="分钟",
               emergency="急救电话：120，消防：119，报警：110。"),
    "ja": dict(name="Japanese", s="秒", m="分", h="時間", months="|".join("%d月" % i for i in range(1, 13)), wd="月 火 水 木 金 土 日", mon_first=1, suffix="日", hour="時間", minute="分",
               emergency="緊急通報：119（消防・救急）、110（警察）。"),
    "ko": dict(name="Korean", s="초", m="분", h="시간", months="|".join("%d월" % i for i in range(1, 13)), wd="월 화 수 목 금 토 일", mon_first=1, suffix="일", hour="시간", minute="분",
               emergency="긴급 전화: 119(소방·구급), 112(경찰)."),
}

KEEP = {"µSv/h", "mSv/h", "nSv/h", "µSv", "mSv", "nSv", "CPM", "V/m", "µT", "OK", "UTC+", "Yarrow Hub", "YARROW"}
SKIP_MACROS = re.compile(r"^(L_|S_AXIS_|S_APPROX|S_RANGE_|S_USVH|S_MSVH|S_NSVH|S_USV$|S_MSV$|S_NSV$|S_CPM|S_VM|S_UT)")

HINTS = [
    (r"^S_T_", "screen title, very short"),
    (r"^S_M_", "menu entry, short"),
    (r"^MENU_DESC", "one-sentence explanation under a menu entry"),
    (r"^SETTINGS_DESC", "one-sentence explanation of a setting"),
    (r"^GUIDE_DATA", "emergency guide: title and short advice lines for the public"),
    (r"^THEME_NAMES|^S_THEME_", "name of a colour theme"),
    (r"^LED_COLOR_NAMES", "name of a colour"),
    (r"^SESS_KINDS_NAMES", "kind of measuring session"),
    (r"^S_CW_", "calibration wizard text"),
    (r"^S_(ATTENTION|DANGER|EVACUATE|NORMAL|LOW|MEDIUM|HIGH)$", "alarm level, in capitals, one word"),
]

STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


def hint_for(name):
    for pat, h in HINTS:
        if re.search(pat, name):
            return h
    return "label on a small screen of a radiation detector, keep it short"


def ask(system, user):
    body = {"model": "tilvar", "messages": [{"role": "system", "content": system}, {"role": "user", "content": user}],
            "temperature": 0.1, "max_tokens": 3000}
    req = urllib.request.Request(BASE + "/chat/completions", data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"})
    r = json.load(urllib.request.urlopen(req, timeout=900))
    return r["choices"][0]["message"]["content"]


def parse_json(text):
    text = text.strip()
    text = re.sub(r"^```(?:json)?\s*|\s*```$", "", text)
    a, b = text.find("{"), text.rfind("}")
    return json.loads(text[a:b + 1])


def macros(text):
    """macro name -> (start, end) span of its definition in the text"""
    out = []
    for m in re.finditer(r"^#define\s+(\w+)\s+((?:.*\\\n)*.*)$", text, flags=re.M):
        out.append((m.group(1), m.start(2), m.end(2)))
    return out


def units_of(text):
    """every translatable string literal: (macro, start, end, literal)"""
    units = []
    for name, a, b in macros(text):
        if SKIP_MACROS.match(name):
            continue
        for m in STRING.finditer(text[a:b]):
            lit = m.group(1)
            core = lit.strip()
            if core in KEEP or not re.search(r"[A-Za-z]{2,}", core) and not re.search(r"[^\x00-\x7f]", core):
                continue
            if re.fullmatch(r"[0-9.,–\-+ ]*(µ?[mn]?Sv(/h)?|µT|V/m)?", core):
                continue
            units.append((name, a + m.start(1), a + m.end(1), lit))
    return units


def ask_json(system, payload, keys):
    """asks for one JSON object with the given keys, retrying when the reply is not usable"""
    for attempt in range(4):
        try:
            reply = parse_json(ask(system, json.dumps(payload, ensure_ascii=False)))
            out = {}
            for k in keys:
                v = reply[k]
                out[k] = (v["text"] if isinstance(v, dict) else v).strip()
            return out
        except Exception as e:  # malformed JSON, missing key: try again
            print("  retry (%s)" % e, file=sys.stderr)
    raise SystemExit("the model did not return usable JSON")


def translate_batch(lang, batch):
    system = ("You translate the text of a radiation detector's small colour screen from English into %s. "
              "Answer with one JSON object only, with the same keys as the input. Rules: keep units (uSv/h, CPM, V/m) and "
              "numbers as they are; use the shortest natural wording that device menus use in that language; never longer "
              "than the English plus 30 percent; keep any leading or trailing space out of the translation; keep "
              "the meaning exact, this is safety equipment." % lang["name"])
    payload = {k: {"text": u["text"], "kind": u["hint"], "max_chars": u["max"]} for k, u in batch.items()}
    return ask_json(system, payload, list(batch))


def build(code):
    lang = LANGS[code]
    en = open(os.path.join(ROOT, "src/lang/en.h"), encoding="utf-8").read()
    units = units_of(en)
    emergency_src = "Emergency number: 112 (EU), 911 (US)."
    todo = {}
    for i, (name, a, b, lit) in enumerate(units):
        if lit == emergency_src:
            continue
        todo["u%03d" % i] = dict(text=lit.strip(), hint=hint_for(name), max=int(len(lit.strip()) * 1.3) + 2)
    print("%s: %d strings to translate" % (code, len(todo)))
    done = {}
    keys = list(todo)
    for i in range(0, len(keys), 14):
        chunk = {k: todo[k] for k in keys[i:i + 14]}
        done.update(translate_batch(lang, chunk))
        print("  %d / %d" % (min(i + 14, len(keys)), len(keys)))
    return en, units, done, lang


def assemble(code, en, units, done, lang):
    out = en
    # replace from the end so that positions stay valid
    for i in range(len(units) - 1, -1, -1):
        name, a, b, lit = units[i]
        key = "u%03d" % i
        if lit == "Emergency number: 112 (EU), 911 (US).":
            new = lang["emergency"]
        else:
            new = done[key]
            lead = lit[:len(lit) - len(lit.lstrip())]
            trail = lit[len(lit.rstrip()):]
            new = lead + new + trail
        new = new.replace("\\", "\\\\").replace('"', '\\"')
        out = out[:a] + new + out[b:]
    # tables that are not left to the model
    def setmacro(name, value):
        nonlocal out
        pat = re.compile(r"^(#define\s+%s\s+).*$" % name, re.M)
        assert pat.search(out), name
        out = pat.sub(lambda m: m.group(1) + value, out, count=1)

    months = lang["months"].split("|") if "|" in lang["months"] else lang["months"].split()
    setmacro("L_MONTHS", "{" + ", ".join('"%s"' % m for m in months) + "}")
    setmacro("L_WEEKDAYS", "{" + ", ".join('"%s"' % w for w in lang["wd"].split()) + '} /* Monday first */')
    s, m, h = lang["s"], lang["m"], lang["h"]
    setmacro("L_PROFILE_STEPS", '{"1 %s", "10 %s", "1 %s", "5 %s", "1 %s"}' % (s, s, m, m, h))
    setmacro("S_AXIS_10MIN", '"-10 %s"' % m)
    setmacro("S_AXIS_60S", '"-60 %s"' % s)
    setmacro("L_HOUR_UNIT", '"%s"' % lang["hour"])
    setmacro("L_MIN_UNIT", '"%s"' % lang["minute"])
    setmacro("L_DATE_MON_FIRST", str(lang["mon_first"]))
    setmacro("L_DAY_SUFFIX", '"%s"' % lang["suffix"])
    out = re.sub(r"^/\* User interface text: English\..*$", "/* User interface text: %s. Same macro names as lang/tr.h; tools/langcheck.py compares the two. */" % lang["name"], out, count=1, flags=re.M)
    path = os.path.join(ROOT, "src/lang/%s.h" % code)
    open(path, "w", encoding="utf-8", newline="\n").write(out)
    return path


def fit(code, lang, rounds=4):
    """Runs the simulator, finds texts that do not fit and asks the model for shorter ones."""
    for rnd in range(rounds):
        res = subprocess.run(["mingw32-make", "sim", "YLANG=%s" % code], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
        bad = re.findall(r'^LAYOUT ([^:]+): "(.*)" needs (\d+) px, room for (\d+)$', res.stdout, flags=re.M)
        if not bad:
            print("%s: everything fits" % code)
            return True
        text = open(os.path.join(ROOT, "src/lang/%s.h" % code), encoding="utf-8").read()
        todo = {}
        for i, (kind, s, need, room) in enumerate(bad):
            ratio = int(room) / int(need)
            parts = s.split(" | ") if kind == "footer" else [s]
            if kind.startswith("text runs"):
                ratio = 0.7  # the text needs fewer lines, not fewer pixels
            for j, part in enumerate(parts):
                part = re.sub(r"^(\[[^\]]*\])+\s*", "", part).strip()  # the key hint is not part of the text
                if len(part) < 3:
                    continue
                pieces = [part]
                if ('"%s"' % part) not in text and ('"%s' % part) not in text:
                    pieces = [p for p in re.split(r"(?::| ·) ", part) if len(p) >= 3]  # a line put together from two texts
                for k2, piece in enumerate(pieces):
                    todo["f%02d_%d_%d" % (i, j, k2)] = dict(text=piece, hint="%s on a small screen; the whole line is %d px wide and must fit in %d px" % (kind, int(need), int(room)),
                                                            max=max(2, int(len(piece) * ratio * max(0.5, (0.9 if len(pieces) == 1 else 0.75) - 0.12 * rnd))))
        if not todo:
            break
        system = ("The following %s texts are too wide for a small screen. Give a shorter version of each (same meaning, "
                  "natural wording, at most max_chars characters). Answer with one JSON object with the same keys." % lang["name"])
        reply = {}
        keys = list(todo)
        for i in range(0, len(keys), 10):
            chunk = {k: todo[k] for k in keys[i:i + 10]}
            reply.update(ask_json(system, chunk, list(chunk)))
        for k, u in todo.items():
            new = reply[k]
            old = u["text"]
            if old in text and new and new != old:
                text = text.replace('"%s"' % old, '"%s"' % new.strip().replace('"', '\\"'))
            elif ('"%s' % old) in text and new and new != old:
                text = text.replace('"%s' % old, '"%s' % new.strip().replace('"', '\\"'))
        open(os.path.join(ROOT, "src/lang/%s.h" % code), "w", encoding="utf-8", newline="\n").write(text)
    print("%s: some texts still do not fit, shorten them by hand" % code)
    return False


def main():
    code = sys.argv[1]
    lang = LANGS[code]
    if "--fit-only" not in sys.argv:
        en, units, done, lang = build(code)
        print("written", assemble(code, en, units, done, lang))
    if "--fit" in sys.argv or "--fit-only" in sys.argv:
        fit(code, lang)


if __name__ == "__main__":
    main()
