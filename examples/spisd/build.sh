set -x
vc +aos68k -rmcfg-ldnodb -ldnodb="-Rstd -s -kick1x" -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c bootpoint.S ../../spi-lib/spi.c ../../spi-lib/spi_low.asm -I../../spi-lib -O2 -nostdlib -lamiga -o spisd.device
make -C bootldr