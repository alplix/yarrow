# Adding a language

Each language is its own firmware file, because the font of every language holds only the characters its text uses.
All text lives in one header per language, `src/lang/<code>.h`, with the same macro names in every file.
Turkish (`tr.h`) is the source and English (`en.h`) the reference for translators.

## By hand

1. Copy `src/lang/en.h` to `src/lang/<code>.h` (`de.h`, `ja.h`, …).
2. Translate the strings. Keep the macro names, the order of the lists and the number of entries.
3. Check it:

        python tools/langcheck.py <code>

   It reports missing or extra macros and lists or guide pages with a different shape.
4. See how it looks:

        make sim YLANG=<code>

   This renders every screen to `build/<code>/sim`, once with typical values and once with the widest numbers.
   Lines starting with `LAYOUT` name texts that do not fit their place (and missing glyphs); shorten them.
   A line that says `DIFFERS` means a screen would leave stale pixels after a text changed.
5. Build and try it: `make YLANG=<code>` produces `build/<code>/yarrow-gc03.bin`.

## With a local model

`tools/translate.py` translates `en.h` with any OpenAI-compatible server (for example llama.cpp with a Gemma model):

    python tools/translate.py de --fit

It sends the strings in small batches with a hint for each (screen title, menu entry, one-line explanation), writes the
unit abbreviations, months, weekdays, the date order and the emergency number from tables instead of asking the model,
then runs the simulator and asks for shorter wording wherever a text does not fit. The server address is taken from
`TILVAR_BASE_URL` (default `http://127.0.0.1:8080/v1`). A machine translation always needs a native reader, above all
the emergency guide.

## What to know

- **Length matters.** The screen is 240 pixels wide. Footers show a key hint on the left and a word on the right;
  menu descriptions have two lines; list rows hold about 20 characters. Prefer short words.
- **Dates and times.** `L_MONTHS`, `L_HOUR_UNIT`, `L_MIN_UNIT` and `L_DATE_MON_FIRST` / `L_DAY_SUFFIX` set how
  durations and dates are written (`5 Oca`, `Oct 5`, `10月5日`). `L_WEEKDAYS` starts on Monday.
- **The emergency guide** (`GUIDE_DATA`) is safety advice: keep the meaning of every line and use the emergency number
  that applies where the language is spoken.
- **Fonts.** `tools/genfont.py` reads the characters from the language file and renders only those from the Inter font
  (Latin, Cyrillic, Greek, Vietnamese). Chinese, Japanese and Korean take their characters from Noto Sans CJK, which is
  fetched separately, see `third_party/noto-cjk/README.md`. The 20 point font only holds the characters of screen
  titles, alarm levels and guide page titles. Another script needs another font: add it in `tools/genfont.py`.
- **Units** (µSv/h, nSv, V/m, CPM) stay as they are.

A pull request with a new `src/lang/<code>.h` is all that is needed.
