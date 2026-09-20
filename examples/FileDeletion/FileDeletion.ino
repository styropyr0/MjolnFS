#include <MjolnFS.h>

MjolnFileSystem fs(AT24C32);

void setup()
{
    Serial.begin(9600);
    delay(5000);
    fs.showLogs(false);
    if (!fs.mount()) // If mounting fails, it means the file system is not found or invalid
        fs.format(); // Initially, the eeprom shall be formatted with MjolnFS File System's Boot sector
    fs.writeFile("test", "Maupertuis set about generalising his earlier mathematical work.");
    fs.deleteFile("test");
    char *fileBuffer = fs.readFile("test"); // Or dynamically allocate the space
    if (fileBuffer)
    {
        Serial.print("File content: ");
        Serial.println(fileBuffer);
    }
    else
        Serial.println("File not found!");
}

void loop()
{
}
