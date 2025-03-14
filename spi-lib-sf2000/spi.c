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
    volatile UWORD status;
    volatile UWORD shift_ctrl;
    volatile UWORD int_req;
    volatile UWORD int_ena;
    volatile UWORD int_act;
    volatile UWORD fifo;
};

static struct SF2000SDRegisters *regs;

#define CLK_DIV_50M     0
#define CLK_DIV_25M     1
#define CLK_DIV_16M     2
#define CLK_DIV_400K    124

#define MODE_STOP       0
#define MODE_RX         1
#define MODE_TX         2
#define MODE_BOTH       3

#define STATUS_SHIFTER_BUSY     0x0040
#define STATUS_TX_HALF_EMPTY    0x0020
#define STATUS_RX_HALF_FULL     0x0010
#define STATUS_TX_CB_FULL       0x0008
#define STATUS_TX_CB_EMPTY      0x0004
#define STATUS_RX_CB_FULL       0x0002
#define STATUS_RX_CB_EMPTY      0x0001

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
    regs->clock_divisor = speed == SPI_SPEED_FAST ? CLK_DIV_25M : CLK_DIV_400K;
}

void spi_read(__reg("a0") UBYTE *buf, __reg("d0") WORD size)
{
    volatile UWORD *word_access = (volatile UWORD *)&(regs->fifo);
    volatile UBYTE *byte_access = (volatile UBYTE *)word_access;

    regs->shift_ctrl = (UWORD)((MODE_RX << 14) | (size & 0x1fff));

    if (((ULONG)buf) & 1)
    {
        while ((regs->status & STATUS_RX_CB_EMPTY) != 0) {
        }
        *buf++ = *byte_access;
        size -= 1;
    }

    WORD chunk_count = size >> 4;

    if (chunk_count)
    {
        size -= chunk_count << 4;

        ULONG *buf_longword = (ULONG *)buf;
        volatile ULONG *longword_access = (volatile ULONG *)word_access;

        for (WORD i = chunk_count - 1; i >= 0; i--)
        {
            while ((regs->status & STATUS_RX_HALF_FULL) == 0) {
            }
            *buf_longword++ = *longword_access;
            *buf_longword++ = *longword_access;
            *buf_longword++ = *longword_access;
            *buf_longword++ = *longword_access;
        }

        buf = (UBYTE *)buf_longword;
    }

    UWORD *buf_word = (UWORD *)buf;

    for (WORD i = (size >> 1) - 1; i >= 0; i--)
    {
        while ((regs->status & STATUS_RX_CB_FULL) == 0) {
        }
        *buf_word++ = *word_access;
    }

    if (size & 1)
    {
        buf = (UBYTE *)buf_word;
        while ((regs->status & STATUS_RX_CB_EMPTY) != 0) {
        }
        *buf++ = *byte_access;
    }
}

void spi_write(__reg("a0") const UBYTE *buf, __reg("d0") WORD size)
{
    volatile UWORD *word_access = (volatile UWORD *)&(regs->fifo);
    volatile UBYTE *byte_access = (volatile UBYTE *)word_access;

    regs->shift_ctrl = (UWORD)(MODE_TX << 14);

    if (((ULONG)buf) & 1)
    {
        *byte_access = *buf++;
        size -= 1;
    }

    WORD chunk_count = size >> 4;

    if (chunk_count)
    {
        size -= chunk_count << 4;

        const ULONG *buf_longword = (const ULONG *)buf;
        volatile ULONG *longword_access = (volatile ULONG *)word_access;

        for (WORD i = chunk_count - 1; i >= 0; i--)
        {
            while ((regs->status & STATUS_TX_HALF_EMPTY) == 0) {
            }
            *longword_access = *buf_longword++;
            *longword_access = *buf_longword++;
            *longword_access = *buf_longword++;
            *longword_access = *buf_longword++;
        }

        buf = (const UBYTE *)buf_longword;
    }

    const UWORD *buf_word = (const UWORD *)buf;

    for (WORD i = (size >> 1) - 1; i >= 0; i--)
    {
        while ((regs->status & STATUS_TX_CB_EMPTY) == 0) {
        }
        *word_access = *buf_word++;
    }

    if (size & 1)
    {
        buf = (const UBYTE *)buf_word;
        while ((regs->status & STATUS_TX_CB_FULL) != 0) {
        }
        *byte_access = *buf++;
    }

    while (regs->status & STATUS_SHIFTER_BUSY) {
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
