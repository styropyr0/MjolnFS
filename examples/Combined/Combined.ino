#include <MjolnFS.h>

MjolnFileSystem fs(AT24C512);

void setup()
{
    Serial.begin(115200);
    fs.showLogs(false);
    fs.setPowerMode(HIGH_PERFORMANCE);
    delay(5000);

    // Initialize and mount the file system
    if (!fs.mount())
    {
        Serial.println("EEPROM not recognized, formatting...");
        fs.format(); // Format EEPROM if unrecognized
    }

    // File operations
    fs.writeFile("config", "settings123"); // Write data to a file
    char *buffer = fs.readFile("config");  // Read from the file
    Serial.println(buffer);                // Print file contents

    // List available files
    fs.listFiles();

    // Display File System Info
    fs.printFileSystemInfo();
    fs.terminal(); // Enable serial-based file system commands
}

void loop()
{
}
