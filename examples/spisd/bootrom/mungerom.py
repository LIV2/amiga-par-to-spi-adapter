#!/usr/bin/env python3
import os
import sys
# Chop up the bootrom section into nibbles for Z2 nibblewise bootrom  - Kick 1.3 and below don't support DAC_BYTEWIDE :(
# The device driver itself will be loaded bytewise by reloc

c_bright_red = "\033[1;31m"
c_reset = "\033[0m"


with open("obj/bootldr", "rb") as s:
    romSize = os.fstat(s.fileno()).st_size
    
    rom = s.read()
    
    with open("obj/bootnibbles","wb") as d:
        for b in rom:
            d.write(bytes([(b & 0xF0) | 0x0F]))
            d.write(bytes([((b << 4) & 0xF0) | 0x0F]))
