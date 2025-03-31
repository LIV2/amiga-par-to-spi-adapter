set -x
GREEN="\e[32;1m";
RESET="\e[0m"
DEBUG=15
if [[ -z ${DEBUG} ]]
then
    DEBUG=0
fi

L2="vlink -bamigahunk -x -Bstatic -Cvbcc -nostdlib -mrel %s %s -L$VBCC/targets/m68k-amigaos/lib -L$NDK32/lib -o %s"
echo -e "${GREEN}## Building spisd.device${RESET}"
vc +aos68k -DDEBUG=${DEBUG} -rmcfg-ldnodb -ldnodb="-Rstd -s -kick1" -rmcfg-l2 -l2="${L2}" -I$NDK32/Include_H romtag.c version.c device.c sd.c timer.c mounter.c debug.c bootpoint.S ../../spi-lib-sf2000/spi.c ../../spi-lib-sf2000/interrupt.asm -I../../spi-lib-sf2000 -O2 -nostdlib -lamiga -ldebug -o spisd.device && \
echo -e "${GREEN}## Building spisd boot rom${RESET}" && \
make -C bootldr