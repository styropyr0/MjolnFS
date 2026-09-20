#include "MjolnFS.h"

MjolnFileSystem::MjolnFileSystem(AT24CXType eepromModel)
    : _eepromType(eepromModel), _deviceAddress(MJOLN_STORAGE_DEVICE_ADDRESS), _eepromSize(0), _pageSize(0), _fatEntryCount(0), signature(MJOLN_SIGNATURE)
{
}

FS_BootSector MjolnFileSystem::readBootSector()
{
    FS_BootSector bootSector;
    uint8_t buffer[sizeof(FS_BootSector)];
    eepromReadBytes(MJOLN_STORAGE_DEVICE_ADDRESS, 0, getAddressSize(), buffer, sizeof(FS_BootSector), getPageSize());
    bootSector = toBootSector(buffer);
    return bootSector;
}

bool MjolnFileSystem::writeBootSector(const FS_BootSector &bootSector)
{
    uint8_t *buffer = bootSectorToBytes(&bootSector);
    if (eepromWriteBytes(MJOLN_STORAGE_DEVICE_ADDRESS, 0, getAddressSize(), buffer, sizeof(FS_BootSector), getPageSize()))
    {
        printLogs("Boot sector written successfully.\n");
        printLogs("Boot sector data: ");

        for (size_t i = 0; i < sizeof(FS_BootSector); i++)
            printLogs(String(buffer[i], HEX) + " ");

        printLogs("\n");
        free(buffer);
        return true;
    }

    free(buffer);
    return false;
}

FS_FATEntry MjolnFileSystem::readFATEntry(uint16_t index)
{
    FS_FATEntry fatEntry;
    uint8_t buffer[sizeof(FS_FATEntry)];
    eepromReadBytes(MJOLN_STORAGE_DEVICE_ADDRESS, sizeof(FS_BootSector) + (index * sizeof(FS_FATEntry)), getAddressSize(), buffer, sizeof(FS_FATEntry), getPageSize());
    fatEntry = toFATEntry(buffer, sizeof(buffer));
    return fatEntry;
}

bool MjolnFileSystem::writeFATEntry(uint16_t index, const FS_FATEntry &entry)
{
    uint8_t *buffer = fatToBytes(&entry);
    if (eepromWriteBytes(MJOLN_STORAGE_DEVICE_ADDRESS, sizeof(FS_BootSector) + (index * sizeof(FS_FATEntry)), getAddressSize(), buffer, sizeof(FS_FATEntry), getPageSize()))
    {
        free(buffer);
        printLogs("FAT entry written successfully.\n");
        return true;
    }
    else
    {
        free(buffer);
        printLogs("Failed to write FAT entry.\n");
        return false;
    }
}

bool MjolnFileSystem::updateFATEntry(uint16_t index, const FS_FATEntry &entry)
{
    uint8_t *buffer = fatToBytes(&entry);
    if (eepromWriteBytes(MJOLN_STORAGE_DEVICE_ADDRESS, sizeof(FS_BootSector) + index * sizeof(FS_FATEntry), getAddressSize(), buffer, sizeof(FS_FATEntry), getPageSize()))
    {
        free(buffer);
        return true;
    }
    else
    {
        free(buffer);
        return false;
    }
}

bool MjolnFileSystem::mount()
{
    Wire.begin();
    _bootSector = readBootSector();

    uint8_t verificationResult = verifyBootSector(&_bootSector);

    switch (verificationResult)
    {
    case MJOLN_FS_SUCCESS_BUT_VERSION_MISMATCH:
    case MJOLN_FS_MERR_NO_ERROR:
    {
        _pageSize = _bootSector.pageSize;
        _fatEntryCount = _bootSector.fileCount[0] | (_bootSector.fileCount[1] << 8);
        uint32_t lastDataAddr = _bootSector.lastDataAddr[0] | (_bootSector.lastDataAddr[1] << 8) | (_bootSector.lastDataAddr[2] << 16);

        printLogs("Mounting file system...\n");
        printLogs("File system mounted.\n\n");

        printLogs("\nMjoln File System\n-----------------\n");
        printLogs("EEPROM type: " + String(_eepromType) + "\n");
        printLogs("Valid file system signature.\n");
        printLogs("File system version: " + String(_bootSector.version) + "\n");
        printLogs("File system signature: " + String(_bootSector.signature) + "\n");
        printLogs("Last data address: " + String(lastDataAddr) + "\n");
        printLogs("File count: " + String(_fatEntryCount - (uint16_t)_bootSector.deleted) + "\n\n");
        isInit = true;
        defragment();
        getBytesUsed();
        runInitialIndexingAndStore();

        if (verificationResult == MJOLN_FS_SUCCESS_BUT_VERSION_MISMATCH)
            Serial.println("You may need to update your Mjoln File System to version " + String(MJOLN_FILE_SYSTEM_VERSION) + ".\nFormat using format() function to update the file system.\n");

        return true;
    }

    case MJOLN_FS_MERR_INVALID_SIGNATURE:
    {
        Serial.println("Invalid file system signature.\n");
        return false;
        break;
    }

    case MJOLN_FS_MERR_VERSION_MISMATCH:
    {
        Serial.println("Failed to mount file system.\nFile system version mismatch. Expected version: " + String(MJOLN_FILE_SYSTEM_VERSION) + ", Found version: " + String(_bootSector.version) + "\nUpdate the file system using the format() function.\n");
        return false;
        break;
    }

    case MJOLN_FS_MERR_INVALID_BOOT_SECTOR:
    {
        Serial.println("Invalid boot sector.\n");
        return false;
        break;
    }

    default:
        Serial.println("Unknown error occurred while mounting the file system.\n");
        return false;
    }
}

bool MjolnFileSystem::format()
{
    printLogs("Formatting file system...\n");
    if (!cleanFormat())
        return false;

    _bootSector.version = MJOLN_FILE_SYSTEM_VERSION;
    memcpy(_bootSector.signature, signature, MJOLN_FILE_SYSTEM_SIGNATURE_SIZE);
    _bootSector.pageSize = MJOLN_FILE_SYSTEM_PAGE_SIZE;
    uint16_t reservedSize = getReservedSize();
    _bootSector.lastDataAddr[0] = reservedSize & 0xFF;
    _bootSector.lastDataAddr[1] = (reservedSize >> 8) & 0xFF;
    _bootSector.lastDataAddr[2] = (reservedSize >> 16) & 0xFF;
    _bootSector.fileCount[0] = 0;
    _bootSector.fileCount[1] = 0;
    _bootSector.deleted = 0;
    _bootSector.bytesInUse = 0;
    _fatEntryCount = 0;

    if (!writeBootSector(_bootSector))
    {
        _bootSector = readBootSector();
        return false;
    }
    isInit = true;
    return true;
}

bool MjolnFileSystem::cleanFormat()
{
    if (!Wire.available())
        Wire.begin();

    if (!deletePartition(MJOLN_STORAGE_DEVICE_ADDRESS, (uint16_t)(pow(2, (uint8_t)_eepromType) - 1)))
    {
        printLogs("Failed to delete partition.\n");
        return false;
    }

    fileLookupList = "";
    return true;
}

bool MjolnFileSystem::isFileNameValid(const char *filename)
{
    if (strlen(filename) > MJOLN_FILE_NAME_MAX_LENGTH - 1)
        return false;

    for (size_t i = 0; i < strlen(filename); i++)
    {
        char c = filename[i];
        if (!isalnum(c) && c != '_' && c != '-' && c != '.')
            return false;
    }
    return true;
}

bool MjolnFileSystem::renameFile(const char *oldFilename, const char *newFilename)
{
    if (!isFileNameValid(newFilename))
    {
        printLogs("Invalid new file name. File names can only contain alphanumeric characters, underscores (_), hyphens (-), and periods (.).\n");
        return false;
    }

    if (!isFileSystemInitialized())
        return false;

    uint16_t index = checkFileExistence(oldFilename);
    if (index != MJOLN_FILE_NOT_FOUND)
    {
        FS_FATEntry fatEntry = readFATEntry(index);
        memcpy(fatEntry.filename, newFilename, MJOLN_FILE_NAME_MAX_LENGTH);
        if (updateFATEntry(index, fatEntry))
        {
            fileLookupList.replace(String(oldFilename), String(newFilename));
            printLogs("File renamed successfully from " + String(oldFilename) + " to " + String(newFilename) + ".\n");
            return true;
        }
        else
        {
            printLogs("Failed to rename the file.\n");
            return false;
        }
    }
    printLogs("File not found: " + String(oldFilename) + ".\n");
    return false;
}

bool MjolnFileSystem::updateFile(const char *filename, const char *data)
{
    if (!isFileNameValid(filename))
    {
        printLogs("Invalid file name. File names can only contain alphanumeric characters, underscores (_), hyphens (-), and periods (.).\n");
        return false;
    }

    if (!isFileSystemInitialized())
        return false;

    uint16_t index = checkFileExistence(filename);

    if (index != MJOLN_FILE_NOT_FOUND)
    {
        bool logState = logEnabled;
        showLogs(false);
        bool res = deleteFile(filename);
        uint16_t nextFATIndex = getNextAvailableFATEntryIndex();
        
        if (nextFATIndex == MJOLN_FILE_NOT_FOUND)
            nextFATIndex = _fatEntryCount + 1;
        
        res = res && writeFile(filename, data);
        if (!res)
        {
            printLogs("Failed to update the file data.\n");
            return false;
        }

        FS_FATEntry fatEntry = readFATEntry(nextFATIndex);
        char *updatedData = readFile(filename);
        showLogs(logState);

        if (logEnabled)
        {
            printLogs("\nFILE UPDATE LOGS\n");
            printLogs("----------------\n");
            printLogs("File updated successfully.\n");
            printLogs("File name: " + String(fatEntry.filename) + "\n");
            printLogs("File size: " + String(fatEntry.size[0] | (fatEntry.size[1] << 8) | (fatEntry.size[2] << 16)) + " bytes\n");
            printLogs("File start address: " + String(fatEntry.startAddr[0] | (fatEntry.startAddr[1] << 8) | (fatEntry.startAddr[2] << 16)) + "\n");
            printLogs("File status: " + String(fatEntry.status) + "\n");
            printLogs("File data: ");
            printLogs(String(updatedData));
            printLogs("\n\n");
        }

        delete[] updatedData;
        return true;
    }
    printLogs("File not found!\n");
    return false;
}

bool MjolnFileSystem::writeFile(const char *filename, const char *data)
{
    if (!isFileNameValid(filename))
    {
        printLogs("Invalid file name. File names can only contain alphanumeric characters, underscores (_), hyphens (-), and periods (.).\n");
        return false;
    }

    if (!isFileSystemInitialized())
        return false;

    if (checkFileExistence(filename) == MJOLN_FILE_NOT_FOUND)
    {
        uint32_t length = strlen(data);
        FS_FATEntry fatEntry;
        fatEntry.status = 1;
        memcpy(fatEntry.filename, filename, MJOLN_FILE_NAME_MAX_LENGTH);
        fatEntry.size[0] = length & 0xFF;
        fatEntry.size[1] = (length >> 8) & 0xFF;
        fatEntry.size[2] = (length >> 16) & 0xFF;
        memcpy(fatEntry.startAddr, _bootSector.lastDataAddr, sizeof(fatEntry.startAddr));

        uint16_t nextFATIndex = getNextAvailableFATEntryIndex();
        FS_FATEntry nextFATEntry;
        if (nextFATIndex != MJOLN_FILE_NOT_FOUND)
        {
            nextFATEntry = readFATEntry(nextFATIndex);
        }

        printLogs("Writing file...\n");
        uint32_t startAddr = _bootSector.lastDataAddr[0] | (_bootSector.lastDataAddr[1] << 8) | (_bootSector.lastDataAddr[2] << 16);

        if ((nextFATIndex == MJOLN_FILE_NOT_FOUND ? writeFATEntry(_fatEntryCount + 1, fatEntry) : updateFATEntry(nextFATIndex, fatEntry)))
        {
            if (eepromWriteBytes(MJOLN_STORAGE_DEVICE_ADDRESS, startAddr, getAddressSize(), (const uint8_t *)data, length, getPageSize()))
            {
                _bootSector.lastDataAddr[0] = (startAddr + length) & 0xFF;
                _bootSector.lastDataAddr[1] = ((startAddr + length) >> 8) & 0xFF;
                _bootSector.lastDataAddr[2] = ((startAddr + length) >> 16) & 0xFF;
                _bootSector.bytesInUse += length;

                if (nextFATIndex == MJOLN_FILE_NOT_FOUND)
                {
                    _bootSector.fileCount[0] += 1;
                    _bootSector.fileCount[1] += (_bootSector.fileCount[0] >> 8) & 0xFF;
                    _fatEntryCount++;
                    if (_fatEntryCount > 1)
                        fileLookupList.concat(",");
                    fileLookupList.concat(fatEntry.filename);
                }
                else
                {
                    if (_bootSector.deleted > 0)
                        _bootSector.deleted--;
                    fileLookupList.replace(String(nextFATEntry.filename), String(fatEntry.filename));
                }

                writeBootSector(_bootSector);
            }
            else
            {
                printLogs("Failed to write file data.\n");
                return false;
            }
        }

        if (logEnabled)
        {
            printLogs("\nFILE WRITE LOGS\n");
            printLogs("----------------\n");
            printLogs("File written successfully.\n");
            printLogs("File name: " + String(fatEntry.filename) + "\n");
            printLogs("File size: " + String(length) + " Bytes\n");
            printLogs("File start address: " + String(startAddr) + "\n");
            printLogs("File status: " + String(fatEntry.status) + "\n");
            printLogs("File data: ");
            for (size_t i = 0; i < length; i++)
                printLogs(String(data[i]));
            printLogs("\n\n");
        }

        return true;
    }
    printLogs("Couldn't write this file. File already exists!\n\n");
    return false;
}

char *MjolnFileSystem::readFile(const char *filename)
{
    if (!isFileSystemInitialized())
        return nullptr;

    uint16_t i = checkFileExistence(filename);

    if (i != MJOLN_FILE_NOT_FOUND)
    {
        uint32_t length = tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16);
        char *result = new char[length + 1];
        uint32_t startAddr = tempFatEntry.startAddr[0] | (tempFatEntry.startAddr[1] << 8) | (tempFatEntry.startAddr[2] << 16);
        printLogs("Reading file...\n");
        eepromReadBytes(MJOLN_STORAGE_DEVICE_ADDRESS, startAddr, getAddressSize(), (uint8_t *)result, length, getPageSize());
        result[length] = '\0';
        if (logEnabled)
        {
            printLogs("\nFILE READ LOGS\n");
            printLogs("----------------\n");
            printLogs("File read successfully.\n");
            printLogs("File name: " + String(tempFatEntry.filename) + "\n");
            printLogs("File size: " + String(length) + " Bytes\n");
            printLogs("File start address: " + String(startAddr) + "\n");
            printLogs("File data: ");
            for (size_t i = 0; i < length; i++)
                printLogs(String(result[i]));
            printLogs("\n\n");
        }
        return result;
    }
    printLogs("File not found.\n");
    return nullptr;
}

bool MjolnFileSystem::deleteFile(const char *filename)
{
    if (!isFileSystemInitialized())
        return false;

    uint16_t i = checkFileExistence(filename);
    if (i != MJOLN_FILE_NOT_FOUND)
    {
        tempFatEntry.status = 0;
        if (updateFATEntry(i, tempFatEntry))
        {
            uint32_t length = tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16);
            uint32_t startAddr = tempFatEntry.startAddr[0] | (tempFatEntry.startAddr[1] << 8) | (tempFatEntry.startAddr[2] << 16);
            printLogs("Deleting file...\n");

            if ((_bootSector.lastDataAddr[0] | (_bootSector.lastDataAddr[1] << 8) | (_bootSector.lastDataAddr[2] << 16)) - length == startAddr)
            {
                _bootSector.lastDataAddr[0] = startAddr & 0xFF;
                _bootSector.lastDataAddr[1] = (startAddr >> 8) & 0xFF;
                _bootSector.lastDataAddr[2] = (startAddr >> 16) & 0xFF;
            }

            _bootSector.bytesInUse -= length;
            _bootSector.deleted++;
            updateFATEntry(i, tempFatEntry);
            writeBootSector(_bootSector);
            delay(5);
            _bootSector = readBootSector();

            if (logEnabled)
            {
                printLogs("\nFILE DELETE LOGS\n");
                printLogs("----------------\n");
                printLogs("File deleted successfully.\n");
                printLogs("File name: " + String(tempFatEntry.filename) + "\n");
                printLogs("File size: " + String(length) + " Bytes\n");
                printLogs("File start address: " + String(startAddr) + "\n");
                printLogs("File status: DELETED\n\n");
            }
            return true;
        }
    }
    printLogs("File not found.\n");
    return false;
}

uint16_t MjolnFileSystem::checkFileExistence(const char *filename)
{
    return findFileFromCache(filename);
}

void MjolnFileSystem::listFiles()
{
    if (!isFileSystemInitialized())
        return;

    bool logState = logEnabled;
    showLogs(true);

    uint16_t *fileCount = new uint16_t(_fatEntryCount - (uint16_t)_bootSector.deleted);

    printLogs(String(*fileCount) + " files found.\n\n");

    printLogs("FILES LIST\n");
    printLogs("root\\\n\n");

    const uint8_t indexWidth = 10;
    const uint8_t nameWidth = 24;
    const uint8_t sizeWidth = 12;

    printLogs(
        padRight("Index", indexWidth) +
        padRight("File Name", nameWidth) +
        padRight("Size", sizeWidth) +
        "\n");

    printLogs(
        padRight("-----", indexWidth) +
        padRight("---------", nameWidth) +
        padRight("----", sizeWidth) +
        "\n");

    for (uint16_t i = 1; i <= _fatEntryCount; i++)
    {
        tempFatEntry = readFATEntry(i);

        if (tempFatEntry.status == MJOLN_FILE_SYSTEM_FAT_UNAVAILABLE)
            continue;

        printLogs(
            padRight(String(i), indexWidth) +
            padRight(String(tempFatEntry.filename), nameWidth) +
            padRight(String(tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16)) +
                         (tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16) > 1023 ? "B" : "KB"),
                     sizeWidth) +
            "\n");
    }

    printLogs("\n");
    delete fileCount;
    showLogs(logState);
}

String MjolnFileSystem::padRight(const String &value, uint8_t width)
{
    String result = value;

    if (result.length() >= width)
        return result.substring(0, width);

    while (result.length() < width)
        result += " ";

    return result;
}

void MjolnFileSystem::listAllFiles()
{
    if (!isFileSystemInitialized())
        return;

    bool logState = logEnabled;
    showLogs(true);

    uint16_t *fileCount = new uint16_t(_fatEntryCount - (uint16_t)_bootSector.deleted);

    printLogs(String(*fileCount) + " files found.\n\n");

    printLogs("FILES LIST\n");
    printLogs("root\\\n\n");

    const uint8_t indexWidth = 10;
    const uint8_t nameWidth = 24;
    const uint8_t sizeWidth = 12;
    const uint8_t statusWidth = 12;

    printLogs(
        padRight("Index", indexWidth) +
        padRight("File Name", nameWidth) +
        padRight("Size", sizeWidth) +
        padRight("Status", statusWidth) +
        "\n");

    printLogs(
        padRight("-----", indexWidth) +
        padRight("---------", nameWidth) +
        padRight("----", sizeWidth) +
        padRight("------", statusWidth) +
        "\n");

    for (uint16_t i = 1; i <= _fatEntryCount; i++)
    {
        tempFatEntry = readFATEntry(i);

        printLogs(
            padRight(String(i), indexWidth) +
            padRight(String(tempFatEntry.filename), nameWidth) +
            padRight(String(tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16)) +
                         (tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16) > 1023 ? "B" : "KB"),
                     sizeWidth) +
            padRight(tempFatEntry.status == MJOLN_FILE_SYSTEM_FAT_AVAILABLE ? "AVAILABLE" : "DELETED", statusWidth) +
            "\n");
    }

    printLogs("\n");
    delete fileCount;
    showLogs(logState);
}

void MjolnFileSystem::showLogs(bool show)
{
    logEnabled = show;
}

uint8_t MjolnFileSystem::getPageSize()
{
    switch (_eepromType)
    {
    case AT24C04:
    case AT24C08:
    case AT24C16:
        return 16;
    case AT24C32:
    case AT24C64:
        return 32;
    case AT24C128:
    case AT24C256:
        return 64;
    case AT24C512:
        return 128;
    default:
        return 0;
    }
}

uint16_t MjolnFileSystem::getUsableSize()
{
    return _eepromSize - sizeof(FS_BootSector) - (_fatEntryCount * sizeof(FS_FATEntry));
}

uint16_t MjolnFileSystem::getReservedSize()
{
    switch (_eepromType)
    {
    case AT24C04:
        return 136;
    case AT24C08:
        return 256;
    case AT24C16:
        return 496;
    case AT24C32:
        return 916;
    case AT24C64:
        return 1816;
    case AT24C128:
        return 3016;
    case AT24C256:
        return 4216;
    case AT24C512:
        return 6016;
    default:
        return 0;
    }
}

AT24CX_ADDR_SIZE MjolnFileSystem::getAddressSize()
{
    return (_eepromType == AT24C04 || _eepromType == AT24C08 || _eepromType == AT24C16) ? AT24CX_8Bit : AT24CX_16Bit;
}

float MjolnFileSystem::getStorageUsage()
{
    if (!isFileSystemInitialized())
        return -1;

    printLogs("\nSTORAGE USAGE\n-------------\n");
    float usage = (_bootSector.bytesInUse * 100) / (pow(2, (uint8_t)_eepromType));
    printLogs(String(usage) + "\% used from available space.\n");
    printLogs("Total: " + String((uint32_t)pow(2, (uint8_t)_eepromType)) + " bytes, " + String(getReservedSize()) + " bytes reserved by file system.\n\n");
    return usage;
}

uint32_t MjolnFileSystem::getBytesUsed()
{
    if (!isFileSystemInitialized())
        return 0;

    printLogs("\nSTORAGE USAGE\n-------------\n");
    printLogs(String(_bootSector.bytesInUse) + " bytes used from available space.\n");
    printLogs("Total: " + String((uint32_t)pow(2, (uint8_t)_eepromType)) + " bytes, " + String(getReservedSize()) + " bytes reserved by file system.\n\n");
    return _bootSector.bytesInUse;
}

void MjolnFileSystem::printFileInfo(const char *filename)
{
    if (!isFileSystemInitialized())
        return;

    bool logState = logEnabled;
    showLogs(true);

    if (checkFileExistence(filename))
    {
        uint32_t length = tempFatEntry.size[0] | (tempFatEntry.size[1] << 8) | (tempFatEntry.size[2] << 16);
        uint32_t startAddr = tempFatEntry.startAddr[0] | (tempFatEntry.startAddr[1] << 8) | (tempFatEntry.startAddr[2] << 16);
        printLogs("\nFILE INFORMATION\n");
        printLogs("----------------\n");
        printLogs("File name: " + String(tempFatEntry.filename) + "\n");
        printLogs("File size: " + String(length) + " Bytes\n");
        printLogs("File start address: " + String(startAddr) + "\n");
        printLogs("\n\n");
    }
    else
        printLogs("File not found!\n");

    showLogs(logState);
}

void MjolnFileSystem::printFileSystemInfo()
{
    if (!isFileSystemInitialized())
        return;

    bool logState = logEnabled;
    showLogs(true);

    printLogs("\nMjoln File System\n-----------------\n");
    printLogs("EEPROM type: " + String(_eepromType) + "\n");
    printLogs("File system version: " + String(_bootSector.version) + "\n");
    printLogs("File system signature: " + String(_bootSector.signature) + "\n");
    printLogs("File count: " + String((_bootSector.fileCount[0] | _bootSector.fileCount[1] << 8) - (uint16_t)_bootSector.deleted) + "\n");
    printLogs("Total size: " + String((uint32_t)pow(2, (uint8_t)_eepromType)) + " Bytes\n");
    printLogs("Available size: " + String((uint32_t)pow(2, (uint8_t)_eepromType) - _bootSector.bytesInUse - getReservedSize()) + " Bytes\n");
    printLogs("Reserved size: " + String(getReservedSize()) + " Bytes\n");
    printLogs("Address size: " + String(getAddressSize() ? "16 bit\n" : "8 bit\n"));
    printLogs("Page size: " + String(getPageSize()) + " Bytes\n");
    printLogs("Storage use: " + String((_bootSector.bytesInUse * 100) / (pow(2, (uint8_t)_eepromType))) + "%\n\n");

    showLogs(logState);
}

void MjolnFileSystem::terminal()
{
    if (!isFileSystemInitialized())
        return;

    String inputString = "";
    Serial.println("MJOLN FILE SYSTEM TERMINAL");
    Serial.print("\nmjolnFS@v1> ");
    showLogs(false);

    while (true)
    {
        if (Serial.available() > 0)
        {
            char inputChar = Serial.read();
            yield();

            if (inputChar == '\n')
            {
                inputString.trim();

                if (inputString.equals("exit"))
                {
                    Serial.println("Exiting...");
                    break;
                }
                Serial.println(inputString);
                processCommand(inputString);
                inputString = "";
                Serial.print("\nmjolnFS@v1> ");
            }
            else
            {
                inputString += inputChar;
            }
        }
    }
    showLogs(true);
}

bool MjolnFileSystem::isFileSystemInitialized()
{
    if (!isInit)
        printLogs("File system is not initialized. Possible reasons could be:\n-> Incompatible EEPROM\n-> Skipped mount method\n-> Connection failure to EEPROM\n\n");
    return isInit;
}

uint16_t MjolnFileSystem::findFileFromCache(const char *filename)
{
    uint16_t pos = MJOLN_FILE_NOT_FOUND;

    if (_fatEntryCount > 0)
    {
        char filesBuffer[fileLookupList.length() + 1];
        strcpy(filesBuffer, fileLookupList.c_str());

        char *token = strtok(filesBuffer, ",");
        uint16_t index = loadBalancingState ? _fatEntryCount / 2 : 1;

        while (token != NULL)
        {
            if (strcmp(token, filename) == 0)
            {
                tempFatEntry = readFATEntry(index);
                if (tempFatEntry.status)
                {
                    pos = index;
                    break;
                }
            }
            token = strtok(NULL, ",");
            index++;
        }
    }

    if (loadBalancingState && pos == MJOLN_FILE_NOT_FOUND)
        for (uint16_t i = 1; i <= _fatEntryCount / 2; i++)
        {
            tempFatEntry = readFATEntry(i);
            if (tempFatEntry.status == MJOLN_FILE_SYSTEM_FAT_UNAVAILABLE)
                continue;
            if (strcmp(tempFatEntry.filename, filename) == 0)
                pos = i;
        }

    return pos;
}

void MjolnFileSystem::runInitialIndexingAndStore()
{
    if (_fatEntryCount == 0)
        return;
    loadBalancingState = _fatEntryCount >= MJOLN_FILE_SYSTEM_CACHING_LIMIT;
    for (uint16_t i = (_fatEntryCount < MJOLN_FILE_SYSTEM_CACHING_LIMIT ? 1 : (_fatEntryCount / 2)); i <= _fatEntryCount; i++)
    {
        tempFatEntry = readFATEntry(i);
        fileLookupList += String(tempFatEntry.filename) + (i < _fatEntryCount ? "," : "");
    }
}

void MjolnFileSystem::defragment()
{
    if (_bootSector.deleted == 0)
        return;

    uint32_t offset = 0;

    printLogs("Defragmenting...\n");

    uint16_t fileCount = _bootSector.fileCount[0] | (_bootSector.fileCount[1] << 8);
    uint32_t prevEndAddr = getReservedSize();
    uint32_t prevVoidEndAddr = 0;
    uint16_t updatedFileCount = fileCount;

    for (uint16_t i = 1; i <= fileCount; i++)
    {
        FS_FATEntry entry = readFATEntry(i);
        if (entry.status == MJOLN_FILE_SYSTEM_FAT_AVAILABLE)
        {
            uint32_t fileSize = entry.size[0] | (entry.size[1] << 8) | (entry.size[2] << 16);
            uint32_t startAddr = entry.startAddr[0] | (entry.startAddr[1] << 8) | (entry.startAddr[2] << 16);
            uint32_t endAddr = startAddr + fileSize;

            if (prevEndAddr + 1 != startAddr && startAddr != getReservedSize())
            {
                moveData(startAddr, prevEndAddr, fileSize);
                entry.startAddr[0] = prevEndAddr & 0xFF;
                entry.startAddr[1] = (prevEndAddr >> 8) & 0xFF;
                entry.startAddr[2] = (prevEndAddr >> 16) & 0xFF;
                updateFATEntry(i, entry);
            }

            prevEndAddr += fileSize;
        }
    }

    _bootSector.lastDataAddr[0] = prevEndAddr & 0xFF;
    _bootSector.lastDataAddr[1] = (prevEndAddr >> 8) & 0xFF;
    _bootSector.lastDataAddr[2] = (prevEndAddr >> 16) & 0xFF;

    writeBootSector(_bootSector);
    delay(5);
    _bootSector = readBootSector();

    fileLookupList = "";
    runInitialIndexingAndStore();
    printLogs("Defragmentation complete.\n");
}

void MjolnFileSystem::showDump(uint32_t start, uint32_t end)
{
    showMemoryDump(MJOLN_STORAGE_DEVICE_ADDRESS, start, end, getAddressSize(), getPageSize());
}

void MjolnFileSystem::moveData(uint32_t srcAddr, uint32_t dstAddr, uint32_t length)
{
    uint8_t ps = getPageSize();
    uint8_t pageBuf[128];
    while (length > 0)
    {
        uint16_t chunk = (length > ps) ? ps : (uint16_t)length;
        eepromReadBytes(MJOLN_STORAGE_DEVICE_ADDRESS, srcAddr, getAddressSize(), pageBuf, chunk, ps);
        eepromWriteBytes(MJOLN_STORAGE_DEVICE_ADDRESS, dstAddr, getAddressSize(), pageBuf, chunk, ps);
        srcAddr += chunk;
        dstAddr += chunk;
        length -= chunk;
    }
}

uint16_t MjolnFileSystem::getNextAvailableFATEntryIndex()
{
    for (uint16_t i = 1; i <= _fatEntryCount; i++)
    {
        FS_FATEntry entry = readFATEntry(i);
        if (entry.status == MJOLN_FILE_SYSTEM_FAT_UNAVAILABLE)
            return i;
    }
    return MJOLN_FILE_NOT_FOUND;
}

void MjolnFileSystem::setPowerMode(AT24CXPowerMode mode)
{
    currentPowerMode = mode;
    eepromSetPowerMode(mode);
}
