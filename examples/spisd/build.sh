set -x
vc +aos68k -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c bootblock.S ../../spi-lib/spi.c ../../spi-lib/spi_low.asm -I../../spi-lib -O2 -nostdlib -lamiga -o spisd.device
