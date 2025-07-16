set -x
vc +aos68k -I$NDK32/Include_H -rmcfg-as -as="vasmm68k_mot -I$NDK32/Include_I -quiet -Fhunk -nowarn=62 %s -o %s" romtag.c version.c device.c scsidirect.c sd.c timer.c ../../spi-lib/spi.c ../../spi-lib/spi_low.asm loadseg.S mount.S -I../../spi-lib -O2 -nostdlib -lamiga -lvc -o spisd.device
