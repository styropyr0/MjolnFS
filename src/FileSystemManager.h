#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#ifdef __cplusplus

#include <Arduino.h>
#include <Wire.h>
#include "Logger.h"
#include "MjolnConst.h"

enum AT24CX_ADDR_SIZE
{
    AT24CX_8Bit = 0x00, // 8-bit address size
    AT24CX_16Bit = 0x01 // 16-bit address size
};

enum AT24CXPowerMode
{
    // Low-Power mode. Uses 100KHz clock. Suitable for most operations with reduced power consumption.
    LOW_POWER = 0x00,
    // Balanced mode. Uses 400KHz clock. Offers a good balance between performance and power consumption.
    BALANCED = 0x01,
    // High-Performance mode. Uses 1MHz clock. Provides the best performance at the cost of increased power consumption.
    HIGH_PERFORMANCE = 0x02
};

/**
 * @brief Reads a specified number of bytes from the EEPROM.
 * @param eepromAddr The I2C address of the EEPROM.
 * @param storeAddr The starting address in the EEPROM to read from.
 * @param addressSize The size of the address in bytes.
 * @param buffer Pointer to the buffer where the read data will be stored.
 * @param length The number of bytes to read.
 * @param pageSize The size of the EEPROM page.
 * @note The pageSize is used to determine the maximum number of bytes that can be read in a single operation.
 * @return true if the read operation was successful, false otherwise.
 */
bool eepromReadBytes(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, uint8_t *buffer, uint16_t length, uint8_t pageSize);

/**
 * @brief Writes a specified number of bytes to the EEPROM.
 * @param eepromAddr The I2C address of the EEPROM.
 * @param storeAddr The starting address in the EEPROM to write to.
 * @param addressSize The size of the address in bytes.
 * @param data Pointer to the data to be written.
 * @param length The number of bytes to write.
 * @param pageSize The size of the EEPROM page.
 * @note The pageSize is used to determine the maximum number of bytes that can be written in a single operation.
 * @return true if the write operation was successful, false otherwise.
 */
bool eepromWriteBytes(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, const uint8_t *data, uint16_t length, uint8_t pageSize);

/**
 * @brief Deletes a specified number of bytes from the EEPROM.
 * @param eepromAddr The I2C address of the EEPROM.
 * @param storeAddr The starting address in the EEPROM to delete.
 * @param addressSize The size of the address in bytes.
 * @param length The number of bytes to delete.
 * @param pageSize The size of the EEPROM page.
 * @note The pageSize is used to determine the maximum number of bytes that can be written in a single operation.
 * @return true if the delete operation was successful, false otherwise.
 */
bool eepromDeleteMemoryRange(uint8_t eepromAddr, uint32_t storeAddr, AT24CX_ADDR_SIZE addressSize, uint16_t length, uint8_t pageSize);

/**
 * @brief Deletes all data in the EEPROM.
 * @param eepromAddr The I2C address of the EEPROM.
 * @param length The number of bytes to delete.
 * @return true if the delete operation was successful, false otherwise.
 */
bool deletePartition(uint8_t eepromAddr, uint16_t length);

/**
 * @brief Prints the raw data in the eeprom over the start and end addresses specified.
 * @param eepromAddr The I2C address of the EEPROM.
 * @param start The starting address in the EEPROM.
 * @param end The address to which memory dump should be printed.
 * @param addressSize The size of the address in bytes.
 * @param pageSize The size of the EEPROM page.
 * @note The pageSize is used to determine the maximum number of bytes that can be written in a single operation.
 */
void showMemoryDump(uint8_t eepromAddr, uint16_t start, uint16_t end, AT24CX_ADDR_SIZE addressSize, uint8_t pageSize);

/**
 * @brief Sets the power mode of the EEPROM.
 * @param mode The power mode to set.
 * @note Different power modes may affect the performance and power consumption of the EEPROM. Refer to the EEPROM datasheet for details on the effects of each mode.
 */
void eepromSetPowerMode(AT24CXPowerMode mode);

#endif // __cplusplus
#endif // FILESYSTEM_MANAGER_H