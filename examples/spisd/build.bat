vc romtag.c version.c device.c sd.c timer.c mounter/mounter.c mounter/bootblock.S ../../spi-lib/spi.c ../../spi-lib/spi_low.asm -I../../spi-lib -DNO_CONFIGDEV=1 -O2 -nostdlib -lamiga -o spisd.device
