set -x
GREEN="\e[32;1m";
RESET="\e[0m"
echo -e "${GREEN}## Building spisd.device${RESET}"
vc +aos68k -rmcfg-ldnodb -ldnodb="-Rstd -s -kick1" -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c bootpoint.S ../../spi-lib/spi.c ../../spi-lib/spi_low.asm -I../../spi-lib -O2 -nostdlib -lamiga -o spisd.device && \
echo -e "${GREEN}## Building spisd boot rom${RESET}" && \
make -C bootldr