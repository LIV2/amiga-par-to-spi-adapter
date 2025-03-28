#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <dos/dos.h>

#include "version.h"

extern ULONG auto_init_tables[];
extern struct Library *init(__reg("a1") BPTR seg_list);

LONG noexec(void) {
    return -1;
}

// Need to be const to be placed as constant in code segment
const struct Resident romtag =
{
    .rt_MatchWord = RTC_MATCHWORD,
    .rt_MatchTag = (void *)&romtag,
    .rt_EndSkip = &romtag + 1,
    .rt_Flags = RTF_COLDSTART,
    .rt_Version = VERSION,
    .rt_Type = NT_DEVICE,
    .rt_Pri = 10,
    .rt_Name = device_name,
    .rt_IdString = id_string,
    .rt_Init = (APTR)init,
};
