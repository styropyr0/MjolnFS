#include <MjolnFS.h>

MjolnFileSystem fs(AT24C32);

void setup()
{
    Serial.begin(9600);
    delay(5000);
    fs.showLogs(false);
    if (!fs.mount()) // If mounting fails, it means the file system is not found or invalid
        fs.format(); // Initially, the eeprom shall be formatted with MjolnFS File System's Boot sector
    fs.writeFile("file1", "Maupertuis set about generalising his earlier mathematical work, proposing the principle of least action as a metaphysical principle that underlies all the laws of mechanics. He also expanded into the biological realm, anonymously publishing a book that was part popular science, part philosophy, and part erotica: Venus physique.");
    char *fileBuffer = fs.readFile("file1"); // Or dynamically allocate the space
    if (fileBuffer)
    {
        Serial.print("File content: ");
        Serial.println(fileBuffer);
    }
    else
        Serial.println("Failed to read the file.");
}

void loop()
{
}
