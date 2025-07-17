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
struct SCSI_CDB_6 {
    UBYTE operation;
    UBYTE lba_high;
    UBYTE lba_mid;
    UBYTE lba_low;
    UBYTE length;
    UBYTE control;
};

struct SCSI_CDB_10 {
    UBYTE operation;
    UBYTE flags;
    ULONG lba;
    UBYTE group;
    UWORD length;
    UBYTE control;
};

struct SCSI_READ_CAPACITY_10 {
    UBYTE operation;
    UBYTE reserved1;
    ULONG lba;
    UWORD reserved2;
    UBYTE flags;
    UBYTE control;
};

// SCSI Response Structs
struct SCSI_Inquiry {
    UBYTE peripheral_type;
    UBYTE removable_media;
    UBYTE version;
    UBYTE response_format;
    UBYTE additional_length;
    UBYTE flags[3];
    UBYTE vendor[8];
    UBYTE product[16];
    UBYTE revision[4];
    UBYTE serial[8];
};

struct SCSI_CAPACITY_10 {
    ULONG lba;
    ULONG block_size;
};

// MODE SENSE_6 structures
struct SCSI_MODE_SENSE_6 {
    UBYTE operation;
    UBYTE flags;
    UBYTE page_code;
    UBYTE reserved;
    UBYTE alloc_length;
    UBYTE control;
};

void process_scsi_direct(struct IOStdReq *ior);

#endif