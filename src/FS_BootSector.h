#ifndef FS_BOOTSECTOR_H
#define FS_BOOTSECTOR_H

#include <Arduino.h>
#include "MjolnConst.h"

#ifdef __cplusplus

/**
 * @brief Mjoln EEPROM File System Boot Sector
 * @note This structure represents the boot sector of the Mjoln EEPROM File System.
 * @note It contains information about the file system version, signature, page size, and file count.
 */
struct FS_BootSector
{
    char signature[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE];       // Signature to identify the file system
    uint8_t version;                                        // Version of the file system
    uint8_t lastDataAddr[3];                                // Last data address in EEPROM
    uint8_t pageSize;                                       // Size of a page in EEPROM
    uint8_t fileCount[MJOLN_FILE_SYSTEM_FILE_COUNT_LENGTH]; // Number of files in the file system
    uint8_t deleted;                                        // Deleted count
    uint32_t bytesInUse;                                    // Total bytes used from available storage space
};

/**
 * @brief Converts a byte buffer to a FS_BootSector structure.
 * @param buffer Pointer to the byte buffer.
 * @return FS_BootSector structure.
 * @note The buffer must be at least sizeof(FS_BootSector) bytes long.
 */
FS_BootSector toBootSector(uint8_t *buffer);

/**
 * @brief Converts a FS_BootSector structure to a byte buffer.
 * @param bootSector Pointer to the FS_BootSector structure.
 * @return Pointer to the byte buffer.
 * @note The caller is responsible for freeing the allocated memory.
 */
uint8_t *bootSectorToBytes(const FS_BootSector *bootSector);

/**
 * @brief Verifies the boot sector.
 * @param bootSector Pointer to the FS_BootSector structure.
 * @return MJOLN_FS_MERR_NO_ERROR if the boot sector is valid, otherwise returns an error code.
 * @note This function checks the signature and version of the boot sector.
 */
uint8_t verifyBootSector(FS_BootSector *bootSector);

#endif // __cplusplus
#endif // FS_BOOTSECTOR_H
       // This file defines the structure of the boot sector of the Mjoln EEPROM File System.