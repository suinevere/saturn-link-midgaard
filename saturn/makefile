SRL_MAX_TEXTURES = 1
SRL_MODE = NTSC
SRL_HIGH_RES = 1
SRL_FRAMERATE = 1
SRL_MAX_CD_BACKGROUND_JOBS = 3
SRL_MAX_CD_FILES = 256
SRL_MAX_CD_RETRIES = 5

SRL_USE_SGL_SOUND_DRIVER = 0
SRL_ENABLE_FREQ_ANALYSIS = 0

SGL_MAX_VERTICES = 16
SGL_MAX_POLYGONS = 8
SGL_MAX_EVENTS = 1
SGL_MAX_WORKS = 1

CD_NAME = CoffeeMUD Saturn (USA)

BUILD_DROP = ./BuildDrop

SRL_INSTALL_ROOT ?= ../SaturnRingLib

CORE                = ../core
NETBIN_ONLY_SOURCES = src/system/netbin_nocd.c
SOURCES  = $(patsubst ./%,%,$(shell find src/ $(CORE)/ -name '*.c'))
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))
SOURCES := $(filter-out $(NETBIN_ONLY_SOURCES),$(SOURCES))

ifeq ($(strip $(NETBIN)),1)
SOURCES += $(NETBIN_ONLY_SOURCES)
SRL_CUSTOM_CCFLAGS += -DNETBIN
BUILD_NETBIN = $(BUILD_DROP)/coffeemud.netbin
endif

SRL_CUSTOM_CCFLAGS += $(patsubst %,-I%,$(shell find src $(CORE) -type d))

SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
include $(SDK_ROOT)/shared.mk

LIBS += $(SGLLDIR)/LIBPCM.A
