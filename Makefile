EE_BIN = RepairBox.pl-PSX-XMB-Apps-Installer-v1.0.elf
EE_OBJS = src/main.o src/ui.o src/storage.o src/system_version.o \
	src/app_installer.o src/sha256.o
EE_LIBS = -lfileXio -lpad -ldebug -lpatches
EE_INCS = -Iinclude -I$(PS2SDK)/ports/include
EE_CFLAGS = -std=gnu11 -Wall -Wextra -Werror -fdata-sections -ffunction-sections
EE_LDFLAGS = -Wl,--gc-sections

STANDARD_IRX = iomanX fileXio ps2dev9 ps2atad ps2fs usbd usbhdfsd
EE_OBJS += $(addsuffix _irx.o,$(STANDARD_IRX)) \
	ps2hdd_psx1_irx.o

.PHONY: all clean host-test safety-check symbol-audit

all: host-test safety-check $(EE_BIN) symbol-audit

host-test:
	python3 tests/test_package.py
	python3 tests/test_source_safety.py

safety-check: host-test

symbol-audit: $(EE_BIN)
	python3 tests/test_elf.py $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) \
		$(addsuffix _irx.c,$(STANDARD_IRX)) \
		ps2hdd_psx1_irx.c

ps2hdd_psx1_irx.c: irx/psx1/ps2hdd-sparse-skip.irx
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_psx1_irx

%_irx.c:
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/$*.irx $@ $*_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
