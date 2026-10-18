# Noto Sans CJK

The Chinese, Japanese and Korean firmware files are built with Noto Sans CJK (Regular) by Google and Adobe, under the SIL
Open Font License 1.1 (`LICENSE.txt`). The font files are large and are not kept in the repository. Fetch them before
building `YLANG=zh`, `YLANG=ja` or `YLANG=ko`:

    cd third_party/noto-cjk
    curl -LO https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf
    curl -LO https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf
    curl -LO https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Korean/NotoSansCJKkr-Regular.otf

Only the characters the interface text uses end up in the firmware.
