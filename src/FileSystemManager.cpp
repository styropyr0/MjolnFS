#include "FileSystemManager.h"

bool eepromReadBytes(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, uint8_t *buffer, uint16_t length, uint8_t pageSize)
{
    uint16_t bytesRead = 0;
    while (length > 0)
    {
        uint16_t remainingPageSize = min(length, (uint16_t)(pageSize - (storeAddr % pageSize)));
        Wire.beginTransmission(eepromAddr);
        if (addressSize == AT24CX_16Bit)
            Wire.write((storeAddr >> 8) & 0xFF);
        Wire.write(storeAddr & 0xFF);
        Wire.endTransmission();
        Wire.requestFrom((int)eepromAddr, (int)remainingPageSize);
        for (uint16_t i = 0; i < remainingPageSize && Wire.available(); i++)
            buffer[bytesRead++] = Wire.read();
        length -= remainingPageSize;
        storeAddr += remainingPageSize;
    }
    return Wire.available() == 0;
}

bool eepromWriteBytes(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, const uint8_t *data, uint16_t length, uint8_t pageSize)
{
    uint16_t bytesWrote = 0;
    while (length > 0)
    {
        uint16_t remainingPageSize = min(length, (uint16_t)(pageSize - (storeAddr % pageSize)));
        delay(5);
        Wire.beginTransmission(eepromAddr);
        if (addressSize == AT24CX_16Bit)
            Wire.write((storeAddr >> 8) & 0xFF);
        Wire.write(storeAddr & 0xFF);
        for (uint16_t i = 0; i < remainingPageSize; i++)
            Wire.write(data[bytesWrote++]);
        length -= remainingPageSize;
        storeAddr += remainingPageSize;
        if (Wire.endTransmission() != 0)
            return 0;
    }
    return true;
}

void showMemoryDump(uint8_t eepromAddr, uint16_t start, uint16_t end, AT24CX_ADDR_SIZE addressSize, uint8_t pageSize)
{
    for (uint16_t addr = start; addr < end; addr += pageSize)
    {
        printLogs("Addr " + String(addr) + " - " + String(addr + pageSize) + ": ");
        Wire.beginTransmission(eepromAddr);
        if (addressSize == AT24CX_16Bit)
            Wire.write((addr >> 8) & 0xFF);
        Wire.write(addr & 0xFF);
        Wire.endTransmission();
        Wire.requestFrom((int)eepromAddr, end - start);
        for (uint16_t i = 0; i < pageSize; i++)
            printLogs("0x" + String(Wire.read(), HEX) + ", ");
        printLogs("\n");
    }
}

void eepromSetPowerMode(AT24CXPowerMode mode)
{
    switch (mode)
    {
    case LOW_POWER:
        Wire.setClock(100000);
        break;
    case BALANCED:
        Wire.setClock(400000);
        break;
    case HIGH_PERFORMANCE:
        Wire.setClock(1000000);
        break;
    }
}

bool eepromDeleteMemoryRange(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, uint16_t length, uint8_t pageSize)
{
    uint16_t bytesWrote = 0;
    while (length > 0)
    {
        uint16_t remainingPageSize = min(length, (uint16_t)(pageSize - (storeAddr % pageSize)));
        delay(5);
        Wire.beginTransmission(eepromAddr);
        if (addressSize == AT24CX_16Bit)
            Wire.write((storeAddr >> 8) & 0xFF);
        Wire.write(storeAddr & 0xFF);
        for (uint16_t i = 0; i < remainingPageSize; i++)
            Wire.write(0xFF);
        length -= remainingPageSize;
        storeAddr += remainingPageSize;
        if (Wire.endTransmission() != 0)
            return false;
    }
    return true;
}

bool deletePartition(uint8_t eepromAddr, uint16_t length)
{
    printLogs("Deleting partition...\n\n|--------------------|\n ");
    uint8_t progress = 0;
    for (uint16_t addr = 0; addr < length - 64; addr += 64)
    {
        Wire.beginTransmission(eepromAddr);
        Wire.write((addr >> 8) & 0xFF);
        Wire.write(addr & 0xFF);
        for (uint16_t i = 0; i < 64; i++)
        {
            Wire.write(0xFF);
        }
        yield();
        if ((addr * 100) / length > progress)
        {
            progress = (addr * 100) / length;
            if (progress % 5 == 0)
                printLogs("=");
        }
        if (Wire.endTransmission() != 0)
            return false;
        delay(5);
    }
    printLogs("=\n\n");
    printLogs("Partition deleted successfully.\n");
    return true;
}
