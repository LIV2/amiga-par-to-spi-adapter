/*
 * scsidirect.c
 * 
 * Emulate a few SCSI commands to enable the use of HDToolbox
 *  
*/
#include <stdint.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/scsidisk.h>
#include <devices/trackdisk.h>

#include "scsidirect.h"
#include "sd.h"
#include <string.h>

static uint8_t handle_read_capacity_10(struct SCSICmd *cmd, struct IOStdReq *ior)
{
    const sd_card_info_t *ci = sd_get_card_info();
    const uint8_t *cdb = cmd->scsi_Command;
    
    if (ci->type == sdCardType_None) {
        return TDERR_DiskChanged;
    }
    
    if (cmd->scsi_Length < 8) {
        return IOERR_BADLENGTH;
    }
    
    uint8_t *response = (uint8_t *)cmd->scsi_Data;
    uint32_t last_block;
    uint32_t block_size = 1 << ci->block_size;
    
    // Check PMI bit (bit 0 of flags) - This is needed for HDToolbox drive setup
    if (cdb[8] & 0x01) {
        // PMI=1: Return last block of cylinder containing specified LBA
        uint32_t lba = ((uint32_t)cdb[2] << 24) | ((uint32_t)cdb[3] << 16) | ((uint32_t)cdb[4] << 8) | cdb[5];
        uint32_t sectors_per_cylinder = 4096;  // Same as in device_get_geometry()
        
        if (lba >= ci->total_sectors) {
            last_block = ci->total_sectors - 1;
        } else {
            uint32_t cylinder = lba / sectors_per_cylinder;
            last_block = ((cylinder + 1) * sectors_per_cylinder) - 1;
            
            // Don't exceed actual device size
            if (last_block >= ci->total_sectors) {
                last_block = ci->total_sectors - 1;
            }
        }
    } else {
        // PMI=0: Return the last logical block address
        last_block = ci->total_sectors - 1;
    }
    
    // Big-endian, byte-wise: scsi_Data is caller-supplied and need not be
    // even-aligned, and a ULONG store through an odd pointer is an address
    // error on the 68000.
    response[0] = last_block >> 24;
    response[1] = last_block >> 16;
    response[2] = last_block >> 8;
    response[3] = last_block;
    response[4] = block_size >> 24;
    response[5] = block_size >> 16;
    response[6] = block_size >> 8;
    response[7] = block_size;
    
    cmd->scsi_Actual = 8;
    return 0;
}

static uint8_t handle_inquiry(struct SCSICmd *cmd, struct IOStdReq *ior)
{
    const sd_card_info_t *ci = sd_get_card_info();
    
    if (ci->type == sdCardType_None) {
        return TDERR_DiskChanged;
    }
    
    if (cmd->scsi_Length < 36) {
        return IOERR_BADLENGTH;
    }

    uint8_t response[36];
    memset(response, 0, sizeof(response));
    
    response[0] = 0x00;  // Direct access device
    response[1] = 0x80;  // Removable media
    response[2] = 0x02;  // SCSI-2 compliant
    response[3] = 0x02;  // Response data format
    response[4] = 0x1F;  // Additional length (31 bytes follow)
    
    // Vendor identification (8 bytes, padded with spaces)
    memcpy(&response[8], "SPISD   ", 8);
    
    // Product identification (16 bytes, padded with spaces)
    memcpy(&response[16], "SD/MMC Card     ", 16);
    
    // Product revision (4 bytes, padded with spaces)
    memcpy(&response[32], "1.0 ", 4);
    
    // Never write past what the caller sized (scsi_Length) or asked for
    // (CDB allocation length, byte 4).
    uint32_t n = sizeof(response);
    if (cmd->scsi_Length < n) n = cmd->scsi_Length;
    if (cmd->scsi_Command[4] && cmd->scsi_Command[4] < n) n = cmd->scsi_Command[4];
    memcpy(cmd->scsi_Data, response, n);
    cmd->scsi_Actual = n;
    return 0;
}

static uint8_t handle_mode_sense_6(struct SCSICmd *cmd, struct IOStdReq *ior)
{
    const sd_card_info_t *ci = sd_get_card_info();
    uint8_t *cdb = cmd->scsi_Command;
    
    if (ci->type == sdCardType_None) {
        return TDERR_DiskChanged;
    }
    
    if (cmd->scsi_Length < 4) {
        return IOERR_BADLENGTH;
    }
    
    uint8_t data[64];  // header (4) + page 3 (24) + page 4 (25) = 53 max
    uint8_t page_code = cdb[2] & 0x3F;
    uint32_t idx = 0;
    
    // Mode Parameter Header (4 bytes)
    data[idx++] = 0;    // Mode data length (will be set later)
    data[idx++] = 0;    // Medium type
    data[idx++] = 0;    // Device specific param (no write protect, no cache)
    data[idx++] = 0;    // Block descriptor length (no block descriptors)
    
    // Add requested page(s)
    if (page_code == 0x03 || page_code == 0x3F) {
        // Format Device Page (0x03)
        data[idx++] = 0x03;         // Page code
        data[idx++] = 0x16;         // Page length (22 bytes)
        
        // Tracks per zone, alt sectors/tracks per zone, alt tracks per volume (8 zero bytes)
        for (int i = 0; i < 8; i++) {
            data[idx++] = 0x00;
        }
        
        data[idx++] = 0x01;         // Sectors per track (MSB) - 256 
        data[idx++] = 0x00;         // Sectors per track (LSB)
        
        uint32_t bytes_per_sector = 1 << ci->block_size;
        data[idx++] = (bytes_per_sector >> 8) & 0xFF;  // Bytes per sector (MSB)
        data[idx++] = bytes_per_sector & 0xFF;         // Bytes per sector (LSB)
        
        for (int i = 0; i < 10; i++) {
            data[idx++] = 0x00;
        }
    }
    
    if (page_code == 0x04 || page_code == 0x3F) {
        // Rigid Disk Drive Geometry Page (0x04)
        // Round up: a truncated (floor) cylinder count would make the
        // reported geometry (cylinders * 4096 sectors) imply less capacity
        // than READ_CAPACITY_10 reports, which a filesystem that
        // cross-checks the two could reasonably reject as inconsistent.
        // 4096 (dg_CylSectors) must match device_get_geometry().
        uint32_t cylinders = (ci->total_sectors + 4095) / 4096;  // From geometry
        
        data[idx++] = 0x04;                      // Page code
        data[idx++] = 0x17;                      // Page length (23 bytes)
        data[idx++] = (cylinders >> 16) & 0xFF;  // Cylinders (MSB)
        data[idx++] = (cylinders >> 8) & 0xFF;   // Cylinders (middle)
        data[idx++] = cylinders & 0xFF;          // Cylinders (LSB)
        data[idx++] = 16;                        // Heads
        
        for (int i = 0; i < 18; i++) {
            data[idx++] = 0x00;
        }
    }
    
    // Check if unsupported page requested
    if (page_code != 0x03 && page_code != 0x04 && page_code != 0x3F) {
        return IOERR_NOCMD;
    }
    
    // Set the total length in the header
    data[0] = idx - 1;  // Length excludes the length byte itself

    // Never write past what the caller sized (scsi_Length) or asked for
    // (CDB allocation length, byte 4).
    uint32_t n = idx;
    if (cmd->scsi_Length < n) n = cmd->scsi_Length;
    if (cdb[4] && cdb[4] < n) n = cdb[4];
    memcpy(cmd->scsi_Data, data, n);
    
    cmd->scsi_Actual = n;
    return 0;
}

void process_scsi_direct(struct IOStdReq *ior)
{
    struct SCSICmd *cmd = (struct SCSICmd *)ior->io_Data;
    
    uint8_t *cdb = cmd->scsi_Command;
    uint8_t *buf = (uint8_t *)cmd->scsi_Data;
    uint32_t length = 0;
    uint32_t sector = 0;
    ior->io_Error = 0;
    cmd->scsi_Status = 0;

    switch (cdb[0]) {
        case SCSI_CMD_TEST_UNIT_READY:
            cmd->scsi_Actual = 0;
            ior->io_Error = 0;
            break;
        case SCSI_CMD_READ_6:
        {
            struct SCSI_CDB_6 *cdb6 = (struct SCSI_CDB_6 *)cdb;
            // SCSI-2: a 6-byte CDB's length field of 0 means 256 blocks, not
            // zero -- passing 0 straight through to sd_read() would silently
            // transfer nothing and still report success.
            length = cdb6->length ? cdb6->length : 256;
            sector = (cdb6->lba_high << 16) | (cdb6->lba_mid << 8) | cdb6->lba_low;

            if (sd_read(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_READ_10:
        {
            length = ((uint32_t)cdb[7] << 8) | cdb[8];
            sector = ((uint32_t)cdb[2] << 24) | ((uint32_t)cdb[3] << 16) | ((uint32_t)cdb[4] << 8) | cdb[5];

            if (sd_read(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_WRITE_6:
        {
            struct SCSI_CDB_6 *cdb6 = (struct SCSI_CDB_6 *)cdb;
            // See SCSI_CMD_READ_6: length 0 means 256 blocks, not zero.
            length = cdb6->length ? cdb6->length : 256;
            sector = (cdb6->lba_high << 16) | (cdb6->lba_mid << 8) | cdb6->lba_low;

            if (sd_write(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_WRITE_10:
        {
            // See SCSI_CMD_READ_10: transfer length is CDB bytes 7-8.
            length = ((uint32_t)cdb[7] << 8) | cdb[8];
            sector = ((uint32_t)cdb[2] << 24) | ((uint32_t)cdb[3] << 16) | ((uint32_t)cdb[4] << 8) | cdb[5];

            if (sd_write(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_READ_CAPACITY_10:
            ior->io_Error = handle_read_capacity_10(cmd, ior);
            break;
        case SCSI_CMD_INQUIRY:
            ior->io_Error = handle_inquiry(cmd, ior);
            break;
        case SCSI_CMD_MODE_SENSE_6:
            ior->io_Error = handle_mode_sense_6(cmd, ior);
            break;

        default:
            ior->io_Error = IOERR_NOCMD;
    }

    if (ior->io_Error != 0)
        cmd->scsi_Status = 0x02;
}
