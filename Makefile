EE_BIN = RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.0.elf
EE_OBJS = src/main.o src/ui.o src/storage.o src/system_version.o \
	src/app_installer.o src/sha256.o
EE_LIBS = -lfileXio -lpad -ldebug -lpatches
EE_INCS = -Iinclude -Ithird_party/hdl-dump -I$(PS2SDK)/ports/include
EE_CFLAGS = -std=gnu11 -Wall -Wextra -Werror -fdata-sections -ffunction-sections
EE_LDFLAGS = -Wl,--gc-sections

STANDARD_IRX = iomanX fileXio ps2dev9 ps2atad ps2fs usbd usbhdfsd
EE_OBJS += $(addsuffix _irx.o,$(STANDARD_IRX)) \
	ps2hdd_psx1_irx.o default_cover_png.o

.PHONY: all clean

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) \
		$(addsuffix _irx.c,$(STANDARD_IRX)) \
		ps2hdd_psx1_irx.c default_cover_png.c

ps2hdd_psx1_irx.c: irx/psx1/ps2hdd-sparse-skip.irx
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_psx1_irx

default_cover_png.c: assets/default-cover.png
	$(PS2SDK)/bin/bin2c $< $@ default_cover_png

%_irx.c:
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/$*.irx $@ $*_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
