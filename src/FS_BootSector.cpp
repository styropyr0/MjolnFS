#include "FS_BootSector.h"

FS_BootSector toBootSector(uint8_t *buffer)
{
    FS_BootSector bootSector;

    memcpy(bootSector.signature, buffer, MJOLN_FILE_SYSTEM_SIGNATURE_SIZE);
    bootSector.version = buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE];
    memcpy(bootSector.lastDataAddr, &buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 1], 3);
    bootSector.pageSize = buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 4];
    memcpy(bootSector.fileCount, &buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 5], MJOLN_FILE_SYSTEM_FILE_COUNT_LENGTH);
    bootSector.signature[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE - 1] = '\0';
    memcpy(&bootSector.deleted, &buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 7], 1);
    memcpy(&bootSector.bytesInUse, &buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 8], 4);

    return bootSector;
}

uint8_t *bootSectorToBytes(const FS_BootSector *bootSector)
{
    uint8_t *buffer = (uint8_t *)malloc(sizeof(FS_BootSector));

    if (!buffer)
        return NULL;

    memcpy(buffer, bootSector->signature, MJOLN_FILE_SYSTEM_SIGNATURE_SIZE);
    buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE] = bootSector->version;
    memcpy(&buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 1], bootSector->lastDataAddr, 3);
    buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 4] = bootSector->pageSize;
    memcpy(&buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 5], bootSector->fileCount, MJOLN_FILE_SYSTEM_FILE_COUNT_LENGTH);
    memcpy(&buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 7], &bootSector->deleted, 1);
    memcpy(&buffer[MJOLN_FILE_SYSTEM_SIGNATURE_SIZE + 8], &bootSector->bytesInUse, 4);

    return buffer;
}

uint8_t verifyBootSector(FS_BootSector *bootSector)
{
    if (memcmp(bootSector->signature, MJOLN_SIGNATURE, MJOLN_FILE_SYSTEM_SIGNATURE_SIZE) != 0)
        return MJOLN_FS_MERR_INVALID_SIGNATURE;

    if (bootSector->version < MJOLN_MIN_SUPPORTED_VERSION)
        return MJOLN_FS_MERR_VERSION_MISMATCH;
    else if (bootSector->version < MJOLN_FILE_SYSTEM_VERSION)
        return MJOLN_FS_SUCCESS_BUT_VERSION_MISMATCH;
    else if (bootSector->version > MJOLN_FILE_SYSTEM_VERSION)
        return MJOLN_FS_MERR_VERSION_MISMATCH;

    return MJOLN_FS_MERR_NO_ERROR;
}