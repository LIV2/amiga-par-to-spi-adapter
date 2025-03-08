    XDEF    _InterruptServer
    CODE

;struct InterruptData
;{
;    struct SF2000SDRegisters *regs;
;    void (*change_isr)();
;};

    ; a1 points to InterruptData structure

_InterruptServer:
    move.l  (a1),a0

    tst     14(a0)      ; check int_act
    beq.b   .no_irq

    move    #0,12(a0)   ; disable interrupt

    move.l  4(a1),a1
    jsr     (a1)

.no_irq:
    moveq   #0,d0
    rts
