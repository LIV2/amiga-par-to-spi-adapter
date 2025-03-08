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
    volatile UWORD card_detect;
    volatile UWORD shift_active;
    volatile UWORD shift_reg;
    volatile UWORD int_req;
    volatile UWORD int_ena;
    volatile UWORD int_act;
};

static struct SF2000SDRegisters *regs;

#define CLK_DIV_16M     2
#define CLK_DIV_400K    124

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
    for (int i = size - 1; i >= 0; i--)
    {
        regs->shift_reg = 0xff;
        while (regs->shift_active) {
        }
        *buf++ = (UBYTE)(regs->shift_reg);
    }
}

void spi_write(__reg("a0") const UBYTE *buf, __reg("d0") ULONG size)
{
    for (int i = size - 1; i >= 0; i--)
    {
        regs->shift_reg = *buf++;
        while (regs->shift_active) {
        }
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
