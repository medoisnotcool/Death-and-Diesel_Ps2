# Diesel and Death PS2 native prototype
#
# PS2 build prerequisites:
#   PS2DEV=/path/to/ps2dev
#   PS2SDK=$PS2DEV/ps2sdk
#   GSKIT=$PS2DEV/gsKit
#
# The host test target intentionally does not require the PS2 toolchain.

EE_BIN := diesel.elf
EE_OBJS := src/ps2_main.o src/game.o
HOST_TEST := build/game_tests

.PHONY: all clean host-test

all: $(EE_BIN)

host-test: $(HOST_TEST)
	./$(HOST_TEST)

build:
	mkdir -p build

$(HOST_TEST): tests/game_tests.c src/game.c src/game.h | build
	gcc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc \
		tests/game_tests.c src/game.c -o $@

# PS2SDK's standard EE make rules are used for the actual ELF. The condition
# keeps `make host-test` and `make clean` usable before PS2DEV is installed.
ifeq ($(filter host-test clean,$(MAKECMDGOALS)),)
ifndef PS2DEV
PS2DEV := /usr/local/ps2dev
endif
ifndef PS2SDK
PS2SDK := $(PS2DEV)/ps2sdk
endif
ifndef GSKIT
GSKIT := $(PS2DEV)/gsKit
endif

EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS += -L$(GSKIT)/lib
EE_LIBS += -lgskit -ldmakit -lpad -lpatches -lm

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
endif

clean:
	rm -rf build $(EE_BIN) $(EE_OBJS)
