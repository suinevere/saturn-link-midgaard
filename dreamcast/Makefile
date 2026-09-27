CMUD_HOST   ?= suinevere.duckdns.org
CMUD_PORT   ?= 2324
CMUD_SECRET ?=

CORE     = ../core
ADAPTERS = ../adapters
BUILD  = build

SOURCES = src/main.c \
          src/video/surface_fb.c \
          src/video/font8x8.c \
          src/input/dc_keyboard.c \
          src/net/dc_link.c \
          $(CORE)/session/session.c \
          $(CORE)/telnet/telnet.c \
          $(CORE)/telnet/ansi.c \
          $(ADAPTERS)/tcp/transport_tcp.c \
          $(CORE)/proxy/authproxy.c \
          $(CORE)/screen/console.c \
          $(CORE)/screen/console_view.c \
          $(CORE)/screen/align_pattern.c \
          $(CORE)/screen/box_glyphs.c \
          $(CORE)/screen/colour_theme.c \
          $(CORE)/input/line_edit.c

OBJS = $(addprefix $(BUILD)/,$(notdir $(SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(SOURCES)))

INCLUDES = $(addprefix -I,$(sort $(dir $(SOURCES))) $(CORE)/ports/) \
           -I$(KOS_BASE)/kernel/arch/dreamcast/util \
           -I$(KOS_BASE)/kernel/net

DEFINES = -DCMUD_DC_HOST='"$(CMUD_HOST)"' \
          -DCMUD_DC_PORT=$(CMUD_PORT) \
          -DCMUD_DC_SECRET='"$(CMUD_SECRET)"'

TARGET = $(BUILD)/cmud.elf
CDI    = $(BUILD)/cmud.cdi

MAKEIP   = $(KOS_BASE)/utils/makeip/makeip
SCRAMBLE = $(KOS_BASE)/utils/scramble/scramble

CDI_TOOL ?= $(if $(shell command -v cdi4dc 2>/dev/null),cdi4dc,mkdcdisc)

.PHONY: all cdi clean run

all: $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	kos-cc -std=gnu11 -Wextra $(INCLUDES) $(DEFINES) -c $< -o $@

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS) -lppp

$(BUILD)/1ST_READ.BIN: $(TARGET)
	sh-elf-objcopy -R .stack -O binary $(TARGET) $(BUILD)/cmud.bin
	mkdir -p $(BUILD)/cd
	$(SCRAMBLE) $(BUILD)/cmud.bin $(BUILD)/cd/1ST_READ.BIN
	cp $(BUILD)/cd/1ST_READ.BIN $@

$(BUILD)/IP.BIN: | $(BUILD)
	$(MAKEIP) -g "SATURN LINK MIDGAARD" -c "SUINEVERE" -n "T-41800" -f $@

$(BUILD)/cmud.iso: $(BUILD)/1ST_READ.BIN $(BUILD)/IP.BIN
	mkisofs -C 0,11702 -V COFFEEMUD -G $(BUILD)/IP.BIN -r -J -l -o $@ $(BUILD)/cd

$(CDI): $(BUILD)/cmud.iso
ifeq ($(CDI_TOOL),cdi4dc)
	cdi4dc $< $@
else
	mkdcdisc --no-banner --allow-overwrite -t $< -o $@
endif

cdi: $(CDI)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

clean:
	rm -rf $(BUILD)
