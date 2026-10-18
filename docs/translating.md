# Adding a language

Each language is its own firmware file, because the font of every language holds only the characters its text uses.
All text lives in one header per language, `src/lang/<code>.h`, with the same macro names in every file.

## Steps

1. Copy `src/lang/en.h` to `src/lang/<code>.h` (`de.h`, `ja.h`, …).
2. Translate the strings. Keep the macro names, the order of the lists and the number of entries.
3. Check it:

        python tools/langcheck.py <code>

   It reports missing or extra macros and lists or guide pages with a different shape.
4. See how it looks:

        make sim YLANG=<code>

   This renders every screen to `build/<code>/sim`. Lines starting with `LAYOUT` name texts that do not fit their
   place; shorten them.
5. Build and try it: `make YLANG=<code>` produces `build/<code>/yarrow-gc03.bin`.

## What to know

- **Length matters.** The screen is 240 pixels wide. Footers show a key hint on the left and a word on the right;
  menu descriptions have two lines; list rows hold about 20 characters. Prefer short words.
- **Dates and times.** `L_MONTHS`, `L_HOUR_UNIT`, `L_MIN_UNIT` and `L_DATE_MON_FIRST` / `L_DAY_SUFFIX` set how
  durations and dates are written (`5 Oca`, `Oct 5`, `10月5日`). `L_WEEKDAYS` starts on Monday.
- **The emergency guide** (`GUIDE_DATA`) is medical and safety advice: translate it carefully, keep the meaning of every
  line, and use the emergency number that applies where the language is spoken.
- **Fonts.** `tools/genfont.py` reads the characters from the language file and renders only those from the Inter font
  (Latin, Cyrillic and Greek). Scripts Inter does not have need another font, listed in `assets/fonts.spec`.
- **Units** (µSv/h, nSv, V/m, CPM) stay as they are.

A pull request with a new `src/lang/<code>.h` is all that is needed.
