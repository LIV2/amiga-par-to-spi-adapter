set -x
GREEN="\e[32;1m";
RESET="\e[0m"
echo -e "${GREEN}## Building spisd.device${RESET}"
vc +aos68k -rmcfg-ldnodb -ldnodb="-Rstd -s -kick1" -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c bootpoint.S ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm -I../../spi-lib-sf2000 -O2 -nostdlib -lamiga -o spisd.device && \
echo -e "${GREEN}## Building spisd boot rom${RESET}" && \
make -C bootldr