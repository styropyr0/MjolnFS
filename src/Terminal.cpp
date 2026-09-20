#include "MjolnFS.h"

void MjolnFileSystem::processCommand(String command)
{
    if (command.startsWith("mk "))
    {
        String filename, data;
        extractArgs(command, filename, data);
        if (!filename.isEmpty() && !data.isEmpty())
        {
            if (writeFile(filename.c_str(), data.c_str()))
                Serial.println("File created.");
            else
                Serial.println("ERR: File already exists!");
        }
        else
            Serial.println("Usage: mk <filename> <data>");
    }
    else if (command.startsWith("update "))
    {
        String filename, data;
        extractArgs(command, filename, data);
        if (!filename.isEmpty() && !data.isEmpty())
        {
            if (updateFile(filename.c_str(), data.c_str()))
                Serial.println("File updated.");
            else
                Serial.println("ERR: File not found!");
        }
        else
            Serial.println("Usage: mk <filename> <data>");
    }
    else if (command.startsWith("rm "))
    {
        String filename = command.substring(3);
        filename.trim();
        if (!filename.isEmpty())
        {
            deleteFile(filename.c_str());
            Serial.println("File deleted.");
        }
        else
            Serial.println("Usage: rm <filename>");
    }
    else if (command.equals("ls"))
        listFiles();
    else if (command.equals("clear"))
        Serial.println("\033[2J\033[H");
    else if (command.equals("ls -a"))
        listAllFiles();
    else if (command.startsWith("read "))
    {
        String filename = command.substring(5);
        filename.trim();

        if (!filename.isEmpty())
        {
            char *buffer = new char[256];
            if (buffer)
            {
                if (readFile(filename.c_str(), buffer))
                    Serial.println(buffer);
                else
                    Serial.println("ERR: File not found.");

                delete[] buffer;
            }
            else
            {
                Serial.println("ERR: Memory allocation failed.");
            }
        }
        else
            Serial.println("Usage: read <filename>");
    }
    else if (command.equals("info"))
        printFileSystemInfo();
    else if (command.startsWith("info "))
    {
        String filename = command.substring(5);
        filename.trim();
        if (!filename.isEmpty())
            printFileInfo(filename.c_str());
        else
            Serial.println("Usage: info <filename>");
    }
    else if (command.equals("delpart"))
    {
        format();
        Serial.println("Partition deleted.");
    }
    else if (command.equals("storeuse"))
    {
        Serial.print("Storage Usage: ");
        Serial.print(getStorageUsage(), 2);
        Serial.println("%");
    }
    else if (command.equals("storeusebytes"))
    {
        Serial.print("Bytes Used: ");
        Serial.println(getBytesUsed());
    }
    else if (command.equals("defrag"))
    {
        defragment();
        Serial.println("Defragmentation complete.");
    }
    else if (command.startsWith("dump "))
    {
        bool logState = logEnabled;
        showLogs(true);
        String args = command.substring(5);
        args.trim();
        int spaceIndex = args.indexOf(' ');
        if (spaceIndex != -1)
        {
            String startStr = args.substring(0, spaceIndex);
            String endStr = args.substring(spaceIndex + 1);
            startStr.trim();
            endStr.trim();
            uint32_t startAddr = strtoul(startStr.c_str(), nullptr, 0);
            uint32_t endAddr = strtoul(endStr.c_str(), nullptr, 0);
            showDump(startAddr, endAddr);
        }
        else
            Serial.println("Usage: dump <start_address> <end_address>");
        showLogs(logState);
    }
    else if (command.equals("exit"))
    {
        Serial.println("Exiting...");
    }
    else if (command.equals("sysinfo"))
    {
        Serial.println("\nSYSTEM INFORMATION\n------------------\n");
        Serial.println("EEPROM type: " + String(_eepromType));
        Serial.println("File system version: " + String(_bootSector.version));
        Serial.println("File system signature: " + String(_bootSector.signature));
        uint16_t lastDataAddr = _bootSector.lastDataAddr[0] | (_bootSector.lastDataAddr[1] << 8) | (_bootSector.lastDataAddr[2] << 16);
        Serial.println("Last data address: " + String(lastDataAddr));
        Serial.println("File count: " + String(_fatEntryCount) + "\n");
    }
    else if (command.equals("help"))
    {
        Serial.println("\nAvailable commands:");
        Serial.println("mk <filename> <data> - Create a new file with the specified name and data.");
        Serial.println("update <filename> <data> - Update an existing file with new data.");
        Serial.println("rm <filename> - Delete the specified file.");
        Serial.println("ls - List all files in the system.");
        Serial.println("read <filename> - Read and display the contents of a file.");
        Serial.println("info - Display information about the file system.");
        Serial.println("delpart - Format the file system, erasing all data.");
        Serial.println("storeuse - Show storage usage as a percentage.");
        Serial.println("storeusebytes - Show storage usage in bytes.");
        Serial.println("defrag - Defragment the file system to optimize storage.\n");
    }
    else
        Serial.println("Unknown command.");
}

void MjolnFileSystem::extractArgs(String command, String &filename, String &data)
{
    int firstSpace = command.indexOf(' ');
    int secondSpace = command.indexOf(' ', firstSpace + 1);

    if (firstSpace != -1 && secondSpace != -1)
    {
        filename = command.substring(firstSpace + 1, secondSpace);
        filename.trim();
        data = command.substring(secondSpace + 1);
        data.trim();
    }
}
