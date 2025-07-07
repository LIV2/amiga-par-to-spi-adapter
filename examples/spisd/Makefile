PROJECT=spisd
INCLUDE=$$NDK32/Include_H
CC=vc
CFLAGS=+aos68k -I$(INCLUDE) -O2 -nostdlib -lamiga
OBJDIR=obj
FLAGS = -rmcfg-as -as="vasmm68k_mot -I$$NDK32/Include_I -quiet -Fhunk -nowarn=62 %s -o %s"

# Variant selection - default to sf2000

.PHONY: all clean both

# Common sources
SRCS = romtag.c version.c device.c sd.c timer.c loadseg.S mount.S

ALL_SRCS = $(SRCS) $(SPI_SRCS)

all: both

sf2000: sfsd.device

standard: spisd.device

both: spisd.device sfsd.device

spisd.device: $(SRCS) ../../spi-lib/spi.c ../../spi-lib/spi_low.asm
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(FLAGS) -I../../spi-lib $(SRCS) ../../spi-lib/spi.c ../../spi-lib/spi_low.asm -o $@

sfsd.device: $(SRCS) ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(FLAGS) -DSF2000 -I../../spi-lib-sf2000 $(SRCS) ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm -o $@
	$(MAKE) -C bootrom

clean:
	rm -f spisd.device sfsd.device
	rm -rf $(OBJDIR)
	make -C bootrom clean