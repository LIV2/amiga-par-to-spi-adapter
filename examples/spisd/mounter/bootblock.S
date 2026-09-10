    include "lvo/exec_lib.i"
    include "exec/resident.i"
    XDEF _bootblock
    XDEF _bootblock_end
    XDEF _entrypoint
    XDEF _entrypoint_end

    CNOP 0,4

_entrypoint EQU 0
_entrypoint_end EQU 0

;; Boot block code needed by mounter for FakeConfigDev
_bootblock: lea     DosName(PC),A1
            jsr     _LVOFindResident(A6)
            tst.l   D0
            beq.s   .fail
            move.l  D0,A0
            move.l  RT_INIT(A0),A0
            jsr     (A0)
.fail:      rts

DosName: dc.b "dos.library",0

_bootblock_end:
