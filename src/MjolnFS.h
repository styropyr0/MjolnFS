#ifndef MJOLNFS_H
#define MJOLNFS_H

#ifdef __cplusplus

#include "MjolnFS.h"
#include "MjolnConst.h"
#include "FS_BootSector.h"
#include "FileSystemManager.h"
#include "FS_FATEntry.h"
#include <Wire.h>
#include <Arduino.h>

enum AT24CXType
{
    AT24C04 = 0x09,
    AT24C08 = 0x0A,
    AT24C16 = 0x0B,
    AT24C32 = 0x0C,
    AT24C64 = 0x0D,
    AT24C128 = 0x0E,
    AT24C256 = 0x0F,
    AT24C512 = 0x10,
};

/**
 * @brief MjolnFileSystem class for EEPROM file management.
 *
 * Manages file operations, storage tracking, and system control on an AT24C EEPROM.
 * @author Saurav Sajeev
 */
class MjolnFileSystem
{
public:
    /**
     * @brief Constructs the MjolnFileSystem object.
     * @param eepromModel Specifies the EEPROM type from AT24CXType enum.
     */
    MjolnFileSystem(AT24CXType eepromModel);

    /**
     * @brief Initializes and mounts the file system.
     * @return True if initialization succeeds, false otherwise.
     * @note Must be called once before performing any file operations.
     * If the eeprom is not recogonized as a MjolnFS compatible eeprom, it should be formatted using @fn format() method.
     */
    bool mount();

    /**
     * @brief Writes data to a file.
     * @param filename Name of the file to write to.
     * @param data Data to be written.
     * @return True if write operation is successful, false otherwise.
     */
    bool writeFile(const char *filename, const char *data);

    /**
     * @brief Reads data from a file.
     * @param filename Name of the file to read.
     * @param buffer Buffer to store the read data.
     * @return Length of the file contents.
     * @note Caller is responsible for deallocating the buffer.
     */
    uint32_t readFile(const char *filename, char *buffer);

    /**
     * @brief Updates data in a file.
     * @param filename Name of the file to write to.
     * @param data Data to be written.
     * @return True if update operation is successful, false otherwise.
     * @note Overwrites existing content if the file already exists.
     */
    bool updateFile(const char *filename, const char *data);

    /**
     * @brief Deletes a specified file.
     * @param filename Name of the file to delete.
     * @return True if deletion succeeds, false otherwise.
     * @note Deleted files cannot be recovered.
     */
    bool deleteFile(const char *filename);

    /**
     * @brief Lists all available files in the system.
     * @note Outputs file names to the serial terminal.
     */
    void listFiles();

    /**
     * @brief Prints general information about the file system.
     * @note Displays EEPROM model, total storage, used storage, and available space.
     */
    void printFileSystemInfo();

    /**
     * @brief Prints detailed information about a specific file.
     * @param filename Name of the file to inspect.
     * @note Displays file size, metadata, and last modification time.
     */
    void printFileInfo(const char *filename);

    /**
     * @brief Formats the file system, erasing all data and reset the bootsector.
     * @return True if formatting succeeds, false otherwise.
     * @note **WARNING:** This action deletes all stored files permanently.
     */
    bool format();

    /**
     * @brief Formats the file system, erasing all data. This doesn't reflash the bootsector.
     * @return True if formatting succeeds, false otherwise.
     * @note **WARNING:** This action deletes all stored files permanently. Bootsector will be cleared.
     */
    bool cleanFormat();

    /**
     * @brief Enables or disables system logs.
     * @param show Set to true to enable logs, false to disable.
     * @note Logs display system activity for debugging purposes.
     */
    void showLogs(bool show);

    /**
     * @brief Retrieves the percentage of storage used.
     * @return Storage usage as a percentage (0.0 to 100.0).
     * @note Useful for monitoring EEPROM capacity.
     */
    float getStorageUsage();

    /**
     * @brief Gets the number of bytes used in storage.
     * @return Total bytes occupied in EEPROM.
     */
    uint32_t getBytesUsed();

    /**
     * @brief Handles user commands via a serial terminal.
     * @note Supports file manipulation, system queries, and formatting operations.
     */
    void terminal();

    /**
     * @brief Defragments the file system by compacting file data to eliminate holes. Speeds up file access and optimizes storage.
     * @note Holes are created by file deletions and size-increasing updates.
     * @note No effect if there are no holes.
     */
    void defragment();

private:
    uint8_t _deviceAddress; // I2C address of the EEPROM
    uint32_t _eepromSize;   // Size of the EEPROM in bytes
    uint16_t _pageSize;     // Size of a page in EEPROM
    const char *signature;  // File system signature
    AT24CXType _eepromType; // Type of the EEPROM
    bool isInit = false;
    bool loadBalancingState = false;

    FS_BootSector _bootSector;
    FS_FATEntry tempFatEntry;
    uint16_t _fatEntryCount;
    String fileLookupList;

    FS_BootSector readBootSector();
    bool writeBootSector(const FS_BootSector &bootSector);
    FS_FATEntry readFATEntry(uint16_t index);
    bool writeFATEntry(uint16_t index, const FS_FATEntry &entry);
    bool updateFATEntry(uint16_t index, const FS_FATEntry &entry);
    uint16_t checkFileExistence(const char *filename);
    bool isFileSystemInitialized();
    uint8_t getPageSize();
    uint16_t getUsableSize();
    uint16_t getReservedSize();
    void processCommand(String command);
    void extractArgs(String command, String &filename, String &data);
    uint16_t findFileFromCache(const char *filename);
    void runInitialIndexingAndStore();
    void moveData(uint32_t srcAddr, uint32_t dstAddr, uint32_t length);

    AT24CX_ADDR_SIZE getAddressSize();
};

#endif // __cplusplus
#endif // MJOLNFS_C
