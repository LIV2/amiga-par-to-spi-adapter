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
    struct SCSI_READ_CAPACITY_10 *cdb = (struct SCSI_READ_CAPACITY_10 *)cmd->scsi_Command;
    
    if (ci->type == sdCardType_None) {
        return TDERR_DiskChanged;
    }
    
    if (cmd->scsi_Length < sizeof(struct SCSI_CAPACITY_10)) {
        return IOERR_BADLENGTH;
    }
    
    struct SCSI_CAPACITY_10 *response = (struct SCSI_CAPACITY_10 *)cmd->scsi_Data;
    uint32_t last_block;
    uint32_t block_size = 1 << ci->block_size;
    
    // Check PMI bit (bit 0 of flags) - This is needed for HDToolbox drive setup
    if (cdb->flags & 0x01) {
        // PMI=1: Return last block of cylinder containing specified LBA
        uint32_t lba = cdb->lba;
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
    
    response->lba = last_block;
    response->block_size = block_size;
    
    cmd->scsi_Actual = sizeof(struct SCSI_CAPACITY_10);
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
    
    struct SCSI_Inquiry *response = (struct SCSI_Inquiry *)cmd->scsi_Data;
    memset(response, 0, sizeof(struct SCSI_Inquiry));
    
    response->peripheral_type   = 0x00;  // Direct access device
    response->removable_media   = 0x80;  // Removable media
    response->version           = 0x02;  // SCSI-2 compliant
    response->response_format   = 0x02;  // Response data format
    response->additional_length = 0x1F;  // Additional length (31 bytes follow)
    
    // Vendor identification (8 bytes, padded with spaces)
    memcpy(response->vendor, "SPISD   ", 8);
    
    // Product identification (16 bytes, padded with spaces)
    memcpy(response->product, "SD/MMC Card     ", 16);
    
    // Product revision (4 bytes, padded with spaces)
    memcpy(response->revision, "1.0 ", 4);
    
    cmd->scsi_Actual = sizeof(struct SCSI_Inquiry);
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
    
    uint8_t *data = (uint8_t *)cmd->scsi_Data;
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
        uint32_t cylinders = ci->total_sectors / 4096;  // From geometry
        
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
    
    cmd->scsi_Actual = idx;
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
            length = cdb6->length;
            sector = (cdb6->lba_high << 16) | (cdb6->lba_mid << 8) | cdb6->lba_low;

            if (sd_read(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_READ_10:
        {
            struct SCSI_CDB_10 *cdb10 = (struct SCSI_CDB_10 *)cdb;
            length = cdb10->length;
            sector = cdb10->lba;

            if (sd_read(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_WRITE_6:
        {
            struct SCSI_CDB_6 *cdb6 = (struct SCSI_CDB_6 *)cdb;
            length = cdb6->length;
            sector = (cdb6->lba_high << 16) | (cdb6->lba_mid << 8) | cdb6->lba_low;

            if (sd_write(buf, sector, length) == 0)
                cmd->scsi_Actual = cmd->scsi_Length;
            else
                ior->io_Error = TDERR_NotSpecified;
            break;
        }
        case SCSI_CMD_WRITE_10:
        {
            struct SCSI_CDB_10 *cdb10 = (struct SCSI_CDB_10 *)cdb;
            length = cdb10->length;
            sector = cdb10->lba;

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
