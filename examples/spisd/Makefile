PROJECT=spisd
INCLUDE=$$NDK32/Include_H
CC=vc
CFLAGS=+aos68k -I$(INCLUDE) -O2 -nostdlib -lamiga
OBJDIR=obj
FLAGS = -rmcfg-as -as="vasmm68k_mot -I$$NDK32/Include_I -quiet -Fhunk -nowarn=62 %s -o %s"

# Variant selection - default to sf2000
VARIANT ?= sf2000

.PHONY: all clean sf2000 standard

# Common sources
SRCS = romtag.c version.c device.c sd.c timer.c loadseg.S mount.S

# Variant-specific configuration
ifeq ($(VARIANT),sf2000)
    SPI_DIR = ../../spi-lib-sf2000
    SPI_SRCS = $(SPI_DIR)/spi.c $(SPI_DIR)/interrupt.asm
else ifeq ($(VARIANT),standard)
    SPI_DIR = ../../spi-lib
    SPI_SRCS = $(SPI_DIR)/spi.c $(SPI_DIR)/spi_low.asm
else
    $(error Invalid VARIANT. Use 'sf2000' or 'standard')
endif

TARGET = $(PROJECT).device
ALL_SRCS = $(SRCS) $(SPI_SRCS)

all: $(TARGET)

sf2000:
	$(MAKE) VARIANT=sf2000

standard:
	$(MAKE) VARIANT=standard

both: sf2000 standard

$(TARGET): $(ALL_SRCS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(FLAGS) -I$(SPI_DIR) $(ALL_SRCS) -o $@
	$(MAKE) -C bootrom

clean:
	rm -f $(PROJECT).device
	rm -rf $(OBJDIR)
	make -C bootrom clean