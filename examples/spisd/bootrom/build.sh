#!/bin/bash -x 

case "${1:-all}" in
    "clean")
        rm -f obj/*
        ;;
    "all"|"rom")
        # Build object files
        mkdir -p obj
        vasmm68k_mot -Fhunk -I$NDK32/Include_I -quiet -align -DROM -DBYTEWIDE -o obj/bootldr.o bootldr.S
        vasmm68k_mot -Fhunk -I$NDK32/Include_I -quiet -align -DROM -DBYTEWIDE -o obj/loadseg.o loadseg.S
        vasmm68k_mot -Fhunk -I$NDK32/Include_I -quiet -align -DROM -DBYTEWIDE -o obj/endcode.o endcode.S
        
        # Link bootldr
        vlink -brawbin1 -s -sc -sd -mrel -o obj/bootldr obj/bootldr.o obj/loadseg.o obj/endcode.o
        
        # Create bootnibbles
        ./mungerom.py
        
        # Assemble assets
        vasmm68k_mot -Fhunk -I$NDK32/Include_I -quiet -align -DROM -DBYTEWIDE -o obj/assets.o assets.S
        
        # Build final ROM
        vlink -brawbin1 -s -sc -sd -mrel -Trom.ld -o spisd.rom obj/assets.o
        ;;
    *)
        echo "Usage: $0 [all|clean|rom]"
        exit 1
        ;;
esac