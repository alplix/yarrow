# Yarrow firmware for the FNIRSI GC-03
# Needs: arm-none-eabi-gcc on PATH, GNU make (mingw32-make on Windows)

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

.DEFAULT_GOAL := all
# Language of the device text: make YLANG=en (lang/<code>.h). Every language gets its own firmware file, because the
# fonts differ and take most of the space.
YLANG ?= tr
BUILD = $(if $(filter tr,$(YLANG)),build,build/$(YLANG))
TARGET = $(BUILD)/yarrow-gc03

CFLAGS = -mcpu=cortex-m3 -mthumb -Os -g \
         -std=c11 -Wall -Wextra -ffunction-sections -fdata-sections \
         -ffreestanding -fno-common -MMD -MP -DYLOG_MAX_PAGE=4096 -DYARROW_LANG_HEADER='"lang/$(YLANG).h"' -Ihal/gc03 -Isrc
LDFLAGS = -mcpu=cortex-m3 -mthumb -nostartfiles -nostdlib \
          -Thal/gc03/gc03.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map

SRCS = hal/gc03/startup.c hal/gc03/support.c hal/gc03/board.c hal/gc03/display.c hal/gc03/hal_gc03.c hal/gc03/usb_gc03.c        src/main.c src/font.c src/ui.c src/render.c src/ui_boot.c src/screens.c src/screens2.c src/fmt.c        src/ylc.c src/ylog.c src/kv.c src/meas.c src/alarm.c src/bat.c src/cal.c src/app.c src/proto.c
OBJS = $(SRCS:%.c=$(BUILD)/%.o) $(BUILD)/font_data.o
-include $(OBJS:.o=.d)

all: $(TARGET).bin
	@$(SIZE) $(TARGET).elf

# the font of the chosen language: only the characters its text uses
$(BUILD)/font_data.c: src/lang/$(YLANG).h assets/fonts.spec tools/genfont.py
	@mkdir -p $(BUILD)
	python tools/genfont.py --lang $(YLANG) --out $(BUILD)/font_data.c

$(BUILD)/font_data.o: $(BUILD)/font_data.c
	$(CC) $(CFLAGS) -c $< -o $@

# the same font for the host tests and the simulator (Turkish)
fonts:
	python tools/genfont.py

# firmware file for a release: make release YLANG=en  ->  dist/yarrow-gc03-en.bin
release: all
	@mkdir -p dist
	cp $(TARGET).bin dist/yarrow-gc03-$(YLANG).bin

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -lgcc -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	python tools/finalize.py $@

# Runs the image in an ARM emulator: normal run, truncated and damaged images (needs: pip install unicorn)
test: $(TARGET).bin
	python tools/emutest.py $(TARGET).bin
	python tools/emufull.py $(TARGET).bin 70

# Host unit tests for the portable code
check:
	@mkdir -p $(BUILD)/tests
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_ylc.c src/ylc.c -lm -o $(BUILD)/tests/test_ylc
	$(BUILD)/tests/test_ylc
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_ylog.c src/ylog.c src/ylc.c -lm -o $(BUILD)/tests/test_ylog
	$(BUILD)/tests/test_ylog
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_meas.c src/meas.c -lm -o $(BUILD)/tests/test_meas
	$(BUILD)/tests/test_meas
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_kv.c src/kv.c -o $(BUILD)/tests/test_kv
	$(BUILD)/tests/test_kv
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_alarm.c src/alarm.c -o $(BUILD)/tests/test_alarm
	$(BUILD)/tests/test_alarm
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_bat.c src/bat.c -o $(BUILD)/tests/test_bat
	$(BUILD)/tests/test_bat
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_fmt.c src/fmt.c -o $(BUILD)/tests/test_fmt
	$(BUILD)/tests/test_fmt
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_cal.c src/cal.c src/meas.c -lm -o $(BUILD)/tests/test_cal
	$(BUILD)/tests/test_cal
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_falsealarm.c src/meas.c src/alarm.c -lm -o $(BUILD)/tests/test_falsealarm
	$(BUILD)/tests/test_falsealarm
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_bias.c src/meas.c -lm -o $(BUILD)/tests/test_bias
	$(BUILD)/tests/test_bias
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_render.c src/render.c -o $(BUILD)/tests/test_render
	$(BUILD)/tests/test_render
	gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_app.c src/app.c src/cal.c src/meas.c src/alarm.c src/bat.c src/kv.c src/ylog.c src/ylc.c src/ui.c src/screens.c src/screens2.c src/font.c src/font_data.c src/fmt.c src/proto.c src/render.c -lm -o $(BUILD)/tests/test_app
	$(BUILD)/tests/test_app

# The Python decoder must agree with the C decoder on a real log image
hostcheck:
	@mkdir -p $(BUILD)/tests
	gcc -std=c11 -O2 -Wall -Wextra -Isrc tests/dump_log.c src/ylog.c src/ylc.c -lm -o $(BUILD)/tests/dump_log
	$(BUILD)/tests/dump_log $(BUILD)/tests
	python tools/ylog_decode.py $(BUILD)/tests/flash.bin 1024 > $(BUILD)/tests/decoded.csv
	python tools/compare_csv.py $(BUILD)/tests/expected.csv $(BUILD)/tests/decoded.csv

# Tiny single-purpose images for the first steps on the device: make canary N=A  (or B)
CANARY_BASE = hal/gc03/startup.c hal/gc03/support.c hal/gc03/board.c hal/gc03/display.c src/render.c
CANARY_SRCS_A = $(CANARY_BASE)
CANARY_SRCS_B = $(CANARY_BASE) hal/gc03/hal_gc03.c hal/gc03/usb_gc03.c src/font.c src/font_data.c src/fmt.c
CANARY_SRCS_G = $(CANARY_BASE) hal/gc03/hal_gc03.c hal/gc03/usb_gc03.c src/font.c src/font_data.c
CANARY_SRCS_F = $(CANARY_BASE) src/font.c src/font_data.c
CANARY_SRCS_E = $(CANARY_BASE)
CANARY_SRCS_D = $(CANARY_BASE) hal/gc03/hal_gc03.c hal/gc03/usb_gc03.c
CANARY_SRCS_C = $(CANARY_BASE) src/font.c src/font_data.c src/fmt.c
canary:
	@mkdir -p $(BUILD)/canary
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$(BUILD)/canary/canary-$(N).map $(CANARY_SRCS_$(N)) canary/canary_$(shell echo $(N) | tr A-Z a-z).c -lgcc -o $(BUILD)/canary/canary-$(N).elf
	$(OBJCOPY) -O binary $(BUILD)/canary/canary-$(N).elf $(BUILD)/canary/canary-$(N).bin
	python tools/finalize.py $(BUILD)/canary/canary-$(N).bin

# Host simulator: renders the screens to $(BUILD)/sim/*.ppm and contact sheets (make sim YLANG=en for another language)
sim: $(BUILD)/font_data.c
	@mkdir -p $(BUILD)/sim
	gcc -std=c11 -O1 -Wall -Wextra -Isrc -DYARROW_LANG_HEADER='"lang/$(YLANG).h"' sim/sim.c src/render.c src/font.c $(BUILD)/font_data.c src/ui.c src/screens.c src/screens2.c src/fmt.c src/ui_boot.c -o $(BUILD)/sim/sim
	$(BUILD)/sim/sim $(BUILD)/sim
	python tools/montage.py $(BUILD)/sim

clean:
	rm -rf $(BUILD)

.PHONY: all clean sim test check hostcheck canary fonts release
