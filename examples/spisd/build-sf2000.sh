set -x
vc +aos68k -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm -I../../spi-lib-sf2000 -O2 -nostdlib -lamiga -o spisd.device
