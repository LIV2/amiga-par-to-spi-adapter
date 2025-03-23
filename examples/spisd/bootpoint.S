    XDEF _bootpoint
    XDEF _bootpoint_end

_bootpoint: lea     DosName(PC),A1
            jsr     -96(A6)         /* FindResident */
            tst.l   D0
            beq.s   fail
            move.l  D0,A0
            move.l  22(A0),A0       /* RT_INIT */
            jsr     (A0)
fail:       rts

DosName: DC.B "dos.library"

_bootpoint_end:
