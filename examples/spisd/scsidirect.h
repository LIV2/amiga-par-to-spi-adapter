#ifndef SCSIDIRECT_H_
#define SCSIDIRECT_H_

#define SCSI_CMD_TEST_UNIT_READY  0x00
#define SCSI_CMD_READ_6           0x08
#define SCSI_CMD_WRITE_6          0x0A
#define SCSI_CMD_READ_10          0x28
#define SCSI_CMD_WRITE_10         0x2A
#define SCSI_CMD_INQUIRY          0x12
#define SCSI_CMD_MODE_SENSE_6     0x1A
#define SCSI_CMD_READ_CAPACITY_10 0x25
#define SCSI_CMD_MODE_SENSE_10    0x5A

// SCSI Command Descriptor Blocks (CDBs)
//
// Only all-UBYTE structs may be overlaid on a CDB or response buffer. Any
// UWORD/ULONG member gets padded to an even offset by the compiler, which
// silently misreads the wire format (a READ(10)'s transfer length lives at
// bytes 7-8, but a UWORD after a UBYTE at 6 lands at offset 8: a 1-block
// request decoded as 0x0100 = 256), and "fixing" that with #pragma pack(1)
// just makes vbcc emit an odd-address word access -- an address error on the
// 68000. Decode multi-byte fields byte-wise in scsidirect.c instead.
struct SCSI_CDB_6 {
    UBYTE operation;
    UBYTE lba_high;
    UBYTE lba_mid;
    UBYTE lba_low;
    UBYTE length;
    UBYTE control;
};

void process_scsi_direct(struct IOStdReq *ior);

#endif