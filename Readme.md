# MjolnFileSystem  
**Author:** Saurav Sajeev  
**Description:** A file system designed for EEPROM storage management using AT24C series chips.
**Latest version:** 1.0.0

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
   - Terminal Interaction  
8. [Notes](#notes)  
9. [Warnings](#warnings)  
10. [License](#license)  

---

## Overview
MjolnFileSystem is a lightweight, structured file system built for AT24C-series EEPROM chips. It provides functionality for file storage, retrieval, deletion, and system monitoring while optimizing memory usage. Designed for embedded systems, it allows efficient data management in constrained memory environments.

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

* **File Management:** Create, read, update, delete, and list files stored in EEPROM.
* **Storage Monitoring:** Retrieve storage usage information, including percentage and bytes used.
* **System Control:** Print file system details, format EEPROM, and manage logs.
* **EEPROM Formatting:**

  * `format()`: Resets the boot sector and erases all data.
  * `cleanFormat()`: Erases all data without reflashing the boot sector.
* **Serial Terminal Interaction:** Execute commands via the serial interface for real-time file system management.
* **Performance Optimization:**

  * Efficient file caching.
  * Boot-time one time indexing which creates a lookup table for faster access.

---

## Installation

1. **Clone the Repository**

```bash
git clone https://github.com/YOUR_USERNAME/MjolnFileSystem.git
cd MjolnFileSystem
```

2. **Initialize and Mount the File System**

```cpp
MjolnFileSystem fs(AT24C32);  // Initialize with chosen EEPROM model
if (!fs.mount()) {
    fs.format();  // Format EEPROM if unrecognized
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
char buffer[256]; // Allocate buffer
uint32_t length = fs.readFile("config", buffer);
Serial.println(buffer);
delete[] buffer; // Free memory after use
```

**Updating File Data**

```cpp
fs.updateFile("config", "newsettings456");
```

**Deleting a File**

```cpp
fs.deleteFile("config.txt");
```

**Listing Stored Files**

```cpp
fs.listFiles();
```

---

## Command Set

The `terminal()` function supports various commands for file system interactions.

| Command                  | Description                     | Example                     |
| ------------------------ | ------------------------------- | --------------------------- |
| mk `<filename>` `<data>` | Create a file and write data    | `mk config.txt settings123` |
| rm `<filename>`          | Delete a specified file         | `rm config.txt`             |
| ls                       | List all available files        | `ls`                        |
| read `<filename>`        | Read a file's contents          | `read config.txt`           |
| update `<filename>` `<data>`   | Update a file's contents  | `update config.txt`         |
| info                     | Display file system information | `info`                      |
| delpart                  | Format EEPROM and erase data    | `delpart`                   |
| storeuse                 | Show storage usage %            | `storeuse`                  |
| storeusebytes            | Show total used bytes           | `storeusebytes`             |
| defrag                   | Defragment the file system      | `defrag`                    |
| exit                     | Exit the terminal session       | `exit`                      |

---

## API Reference

### Initialization and Mounting

```cpp
MjolnFileSystem(AT24CXType eepromModel);
bool mount();
```

* Initializes the file system with the selected EEPROM model.
* Use `format()` before mounting if the EEPROM is unrecognized.

---

## File Operations

```cpp
bool writeFile(const char *filename, const char *data);
uint32_t readFile(const char *filename, char *buffer);
bool deleteFile(const char *filename);
bool updateFile(const char *filename, const char *data);
void listFiles();
```

* **writeFile()**: Creates and writes data to a file.
* **readFile()**: Reads file contents into a dynamically allocated buffer. Caller must free it.
* **deleteFile()**: Deletes the specified file.
* **updateFile()**: Updates the contents of a file, replacing any existing data.

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
* `format()` and `cleanFormat()` clear EEPROM data.
* `getStorageUsage()` returns the usage percentage.
* `showLogs()` enables or disables debug logs.

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

* **Memory Management**: Caller is responsible for freeing memory returned by `readFile()`:

```cpp
char* buffer = new char[256];
uint32_t len = fs.readFile("config", buffer);
Serial.println(buffer);
delete[] buffer;
```

* **File System Formatting**: Use `delpart` or `format()` with caution; all data will be erased.

* **Serial Communication**: Ensure commands sent to `terminal()` are well-formatted.

---

## Warnings

* **FAT Entry Size Limitation**: Filenames are limited to 8 characters due to a 16-byte FAT entry size. Exceeding this may cause file errors.

---

## License

This project is licensed under the **MIT License**. You are free to use, modify, and distribute it for personal or commercial purposes.

---

```

Let me know if you'd like this converted into an actual `README.md` file or need further enhancements (e.g., diagram of internal architecture, example terminal session logs, etc.).
```
