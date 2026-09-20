# MjolnFileSystem  
**Author:** Saurav Sajeev  
**Description:** A file system designed for EEPROM storage management using AT24C series chips.  
**Latest version:** 1.0.1

---

## **Table of Contents**  
1. [Overview](#overview)  
2. [Supported EEPROM Models](#supported-eeprom-models)  
3. [Features](#features)  
4. [Installation](#installation)  
5. [Usage](#usage)  
6. [Command Set](#command-set)  
7. [API Reference](#api-reference)  
   - Initialization and Mounting  
   - File Operations  
   - File System Information  
   - Power Management  
   - Terminal Interaction  
8. [Notes](#notes)  
9. [Warnings](#warnings)  
10. [License](#license)  

---

## Overview
MjolnFileSystem is a lightweight, structured file system built for AT24C-series EEPROM chips. It provides functionality for file storage, retrieval, deletion, renaming, and system monitoring while optimizing memory usage. Designed for embedded systems, it allows efficient data management in constrained memory environments.

On-disk format version is **2**. Mounting rejects unsupported boot sectors and prompts for `format()` when a version upgrade is required.

---

## Supported EEPROM Models

The file system supports various AT24C EEPROM types, each with different storage capacities:

```cpp
enum AT24CXType {
    AT24C04 = 0x09,   // 4 KB EEPROM
    AT24C08 = 0x0A,   // 8 KB EEPROM
    AT24C16 = 0x0B,   // 16 KB EEPROM
    AT24C32 = 0x0C,   // 32 KB EEPROM
    AT24C64 = 0x0D,   // 64 KB EEPROM
    AT24C128 = 0x0E,  // 128 KB EEPROM
    AT24C256 = 0x0F,  // 256 KB EEPROM
    AT24C512 = 0x10   // 512 KB EEPROM
};
```

The correct EEPROM model must be selected when initializing the file system.

---

## Features

* **File Management:** Create, read, update, rename, delete, and list files stored in EEPROM.
* **Filename Validation:** Names are checked for length and allowed characters before create/update/rename.
* **FAT Reuse:** Unclaimed (deleted) FAT entries are reused when writing new files.
* **Storage Monitoring:** Retrieve storage usage information, including percentage and bytes used.
* **System Control:** Print file system details, format EEPROM, and manage logs.
* **EEPROM Formatting:**

  * `format()`: Resets the boot sector and erases all data (also updates on-disk FS version).
  * `cleanFormat()`: Erases all data without reflashing the boot sector.
* **Defragmentation:** Compacts file data after deletions and size-increasing updates.
* **Power Modes:** Switch EEPROM I2C clock speed between low power, balanced, and high performance.
* **Version-Aware Mounting:** Boot sector verification returns specific error codes for invalid signature, unsupported version, or outdated-but-mountable layouts.
* **Serial Terminal Interaction:** Execute commands via the serial interface for real-time file system management.
* **Performance Optimization:**

  * Efficient file caching.
  * Boot-time one-time indexing which creates a lookup table for faster access.

---

## Installation

1. **Clone the Repository**

```bash
git clone https://github.com/styropyr0/MjolnFS.git
cd MjolnFS
```

2. **Initialize and Mount the File System**

```cpp
MjolnFileSystem fs(AT24C32);  // Initialize with chosen EEPROM model
if (!fs.mount()) {
    fs.format();  // Format EEPROM if unrecognized or version-incompatible
}
```

Ensure the correct EEPROM type is passed as a parameter.

---

## Usage

**Writing Data to a File**

```cpp
fs.writeFile("config", "settings123");
```

**Reading Data from a File**

```cpp
char *buffer = fs.readFile("config");
if (buffer) {
    Serial.println(buffer);
    delete[] buffer;  // Caller must free the returned buffer
}
```

**Updating File Data**

```cpp
fs.updateFile("config", "newsettings456");
```

**Renaming a File**

```cpp
fs.renameFile("config", "settings");
```

**Deleting a File**

```cpp
fs.deleteFile("config");
```

**Listing Stored Files**

```cpp
fs.listFiles();
```

**Defragmenting**

```cpp
fs.defragment();
```

**Setting Power Mode**

```cpp
fs.setPowerMode(BALANCED);         // 400 kHz
fs.setPowerMode(HIGH_PERFORMANCE); // 1 MHz
fs.setPowerMode(LOW_POWER);        // 100 kHz (default)
```

---

## Command Set

The `terminal()` function supports various commands for file system interactions.

| Command | Description | Example |
| ------------------------ | ------------------------------- | --------------------------- |
| `mk <filename> <data>` | Create a file and write data | `mk config settings123` |
| `update <filename> <data>` | Update a file's contents | `update config newsettings` |
| `rename <old> <new>` | Rename an existing file | `rename config settings` |
| `rm <filename>` | Delete a specified file | `rm config` |
| `ls` | List all available files | `ls` |
| `ls -a` | List all files, including deleted ones | `ls -a` |
| `read <filename>` | Read a file's contents | `read config` |
| `info` | Display file system information | `info` |
| `info <filename>` | Display information about a specific file | `info config` |
| `delpart` | Format EEPROM and erase data | `delpart` |
| `storeuse` | Show storage usage % | `storeuse` |
| `storeusebytes` | Show total used bytes | `storeusebytes` |
| `defrag` | Defragment the file system | `defrag` |
| `dump <start> <end>` | Dump EEPROM bytes in a range | `dump 0 64` |
| `sysinfo` | Show system / FS version info | `sysinfo` |
| `clear` | Clear the serial terminal view | `clear` |
| `help` | Show available commands | `help` |
| `exit` | Exit the terminal session | `exit` |

---

## API Reference

### Initialization and Mounting

```cpp
MjolnFileSystem(AT24CXType eepromModel);
bool mount();
```

* Initializes the file system with the selected EEPROM model.
* `mount()` verifies the boot sector signature and version:
  * Compatible version → mounts successfully.
  * Older-but-supported layout → mounts and advises updating via `format()`.
  * Unsupported / newer / invalid signature → fails; call `format()` before use.

---

## File Operations

```cpp
bool writeFile(const char *filename, const char *data);
char *readFile(const char *filename);
bool deleteFile(const char *filename);
bool updateFile(const char *filename, const char *data);
bool renameFile(const char *oldFilename, const char *newFilename);
void listFiles();
void defragment();
```

* **writeFile()**: Creates and writes data to a file (reuses free FAT slots when available).
* **readFile()**: Allocates and returns a null-terminated buffer with the file contents, or `nullptr` if not found. Caller must free it with `delete[]`.
* **deleteFile()**: Deletes the specified file.
* **updateFile()**: Updates the contents of a file by replacing existing data (delete + rewrite).
* **renameFile()**: Renames a file. The new name must be valid and must not already exist.
* **defragment()**: Compacts file data to remove holes left by deletions and growing updates.

```cpp
/**
 * @brief Updates data in a file.
 * @param filename Name of the file to write to.
 * @param data Data to be written.
 * @return True if update operation is successful, false otherwise.
 * @note Overwrites existing content if the file already exists.
 */
bool updateFile(const char *filename, const char *data);
```

---

## File System Information

```cpp
void printFileSystemInfo();
void printFileInfo(const char *filename);
bool format();
bool cleanFormat();
float getStorageUsage();
uint32_t getBytesUsed();
void showLogs(bool show);
```

* Print file system and file-specific information.
* `format()` and `cleanFormat()` clear EEPROM data. Prefer `format()` when upgrading FS version.
* `getStorageUsage()` returns the usage percentage.
* `showLogs()` enables or disables debug logs.

---

## Power Management

```cpp
enum AT24CXPowerMode {
    LOW_POWER = 0x00,         // 100 kHz
    BALANCED = 0x01,          // 400 kHz
    HIGH_PERFORMANCE = 0x02   // 1 MHz
};

void setPowerMode(AT24CXPowerMode mode);
```

* Adjusts the I2C clock used for EEPROM access.
* Default mode is `LOW_POWER`.

---

## Terminal Interaction

```cpp
void terminal();
```

* Launches an interactive serial terminal for file operations and system management.

---

## Performance Optimization

### Internal File Lookup Cache

To enhance lookup speed, the following mechanism is used:

```cpp
uint16_t MjolnFileSystem::findFileFromCache(const char *filename)
```

* Searches filenames cached during initial boot.
* Reduces the need for repeated FAT scans.
* Applies a load-balancing strategy for efficiency.

### Initial FAT Indexing

```cpp
void MjolnFileSystem::runInitialIndexingAndStore();
```

* Runs during boot to cache filenames into a list.
* Enables faster `findFileFromCache()` execution.
* Balances memory usage vs. lookup performance.

---

## Notes

* **Memory Management**: `readFile()` allocates the result on the heap. Always free it when done:

```cpp
char *buffer = fs.readFile("config");
if (buffer) {
    Serial.println(buffer);
    delete[] buffer;
}
```

* **File System Formatting**: Use `delpart` or `format()` with caution; all data will be erased. Use `format()` to upgrade an outdated on-disk FS version.

* **Serial Communication**: Ensure commands sent to `terminal()` are well-formatted. Use `help` for the full command list.

---

## Warnings

* **Filename Limits**: Filenames are limited to **8 characters**. Allowed characters are alphanumeric, underscore (`_`), hyphen (`-`), and period (`.`). Invalid names are rejected on create, update, and rename.
* **Version Compatibility**: EEPROMs formatted with an unsupported FS version will fail to mount until reformatted with `format()`.
* **Data Loss**: Formatting and `delpart` permanently erase all stored files.

---

## License

This project is licensed under the **MIT License**. You are free to use, modify, and distribute it for personal or commercial purposes.
