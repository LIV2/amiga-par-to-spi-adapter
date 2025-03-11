/*
 * Written in the end of April 2020 by Niklas Ekström.
 * Updated in July 2021 by Niklas Ekström to handle Card Present signal.
 * Adapted in March 2025 by Niklas Ekström to work with SF2000 SD card slot.
 */
#include <exec/types.h>
#include <exec/interrupts.h>
#include <exec/libraries.h>

#include <hardware/intbits.h>

#include <proto/exec.h>

#include "spi.h"

static const char spi_lib_name[] = "spi-lib-sf2000";

struct SF2000SDRegisters
{
    volatile UWORD clock_divisor;
    volatile UWORD slave_select;
    // The read-only register card_detect is reused
    // as a write-only register for mode and rx_length.
    volatile UWORD card_detect;
    volatile UWORD status;
    volatile UWORD shift_reg;
    volatile UWORD int_req;
    volatile UWORD int_ena;
    volatile UWORD int_act;
};

static struct SF2000SDRegisters *regs;

#define CLK_DIV_25M     1
#define CLK_DIV_16M     2
#define CLK_DIV_400K    124

#define MODE_STOP       0
#define MODE_RX         1
#define MODE_TX         2
#define MODE_BOTH       3

#define STATUS_IN_FULL      0x0004
#define STATUS_OUT_FULL     0x0002
#define STATUS_BUSY         0x0001

#define IRQ_CD_CHANGED  1

struct InterruptData
{
    struct SF2000SDRegisters *regs;
    void (*change_isr)();
};

extern void InterruptServer();

struct InterruptData interrupt_data;
struct Interrupt ports_interrupt;

void spi_select()
{
    regs->slave_select = 1;
}

void spi_deselect()
{
    regs->slave_select = 0;
}

int spi_get_card_present()
{
    int present = regs->card_detect;

    // Clear the CD changed interrupt.
    regs->int_req = IRQ_CD_CHANGED;

    // Re-enable the CD changed interrupt.
    regs->int_ena = IRQ_CD_CHANGED;

    return present;
}

void spi_set_speed(long speed)
{
    regs->clock_divisor = speed == SPI_SPEED_FAST ? CLK_DIV_16M : CLK_DIV_400K;
}

void spi_read(__reg("a0") UBYTE *buf, __reg("d0") ULONG size)
{
    regs->card_detect = (UWORD)((MODE_RX << 14) | (size & 0x1fff));

    for (int i = size - 1; i >= 0; i--)
    {
        while ((regs->status & STATUS_OUT_FULL) == 0) {
        }
        UWORD tmp = regs->shift_reg;
        *buf++ = (UBYTE)tmp;
    }
}

void spi_write(__reg("a0") const UBYTE *buf, __reg("d0") ULONG size)
{
    regs->card_detect = (UWORD)(MODE_TX << 14);

    for (int i = size - 1; i >= 0; i--)
    {
        regs->shift_reg = *buf++;
        while (regs->status & STATUS_IN_FULL) {
        }
    }
    while (regs->status & STATUS_BUSY) {
    }
}

int spi_initialize(void (*change_isr)())
{
    // TODO: This address should not be hardcoded,
    // and should be read from autoconfig/expansion.library.
    regs = (struct SF2000SDRegisters *)0xEE0000;

    regs->clock_divisor = CLK_DIV_400K;

    regs->int_ena = 0;
    regs->int_req = IRQ_CD_CHANGED;

    interrupt_data.regs = regs;
    interrupt_data.change_isr = change_isr;

    ports_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    ports_interrupt.is_Node.ln_Pri = -60;
    ports_interrupt.is_Node.ln_Name = (char *)spi_lib_name;
    ports_interrupt.is_Data = (APTR)&interrupt_data;
    ports_interrupt.is_Code = InterruptServer;

    AddIntServer(INTB_PORTS, &ports_interrupt);

    return spi_get_card_present();
}

void spi_shutdown()
{
    regs->int_ena = 0;
    regs->int_req = IRQ_CD_CHANGED;
    RemIntServer(INTB_PORTS, &ports_interrupt);
}
