set -x
vc +aos68k -rmcfg-ldnodb -ldnodb="-Rstd -s -kick1x" -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c bootpoint.S ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm -I../../spi-lib-sf2000 -O2 -nostdlib -lamiga -o spisd.device 
make -C bootldr