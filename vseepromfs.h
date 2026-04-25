/*
This is some simple fileSytem library to work with internal EEPROM's using EEPROM.h lib from arduino
Supports Format,Read,Write,Delete,List Files and others
All directory are virtual and part of the filename.
The eeprom space will be divided in slots, if some file was deleted, that slot becomes disabled
If new file will be created, checks if that file can get in some free space, yes then the slot is reused, if not new slot created
In format os init, first it alloc space to file names FSFSMAX_FILES*FSFSNAME_SIZE, then all remain free space are data

https://github.com/vslinuxdotnet/VSEEPROMFS

VSEEPROMFS V0.6
By Vasco Santos 2026
*/


/*
Some example!!!!

#include "vseepromfs.h"

VSEEPROMFS fs2;

void setup() {

  Serial.begin(9600);
  fs2.begin();

  char meuBuffer[64];
  if (fs2.readFile("ver.txt", meuBuffer, sizeof(meuBuffer))) {
    Serial.println(meuBuffer);
  }else{
    fs2.writeFile("ver.txt", "FS EEPROM v1.0");
  }

  if (!fs2.exists("/sys/version.txt"))
    fs2.writeFile("/sys/version.txt", "0.4");


  fs2.listAllFiles();
  
  fs2.hexdump();
  
  }
*/

#include <Arduino.h>
#include <EEPROM.h>

#define FS_NAME_SIZE 20 //max file name len
#define FS_MAX_FILES 15 //max files

class VSEEPROMFS {

public:
  struct FileEntry {
  char name[FS_NAME_SIZE];
  int startAddress;
  int fileSize;
  bool active;
};

private:
  int nextAddress;
  const int TABLE_START = 1;
  int EEPROM_SIZE = (int) EEPROM.length();
  int FSMAX_FILES = FS_MAX_FILES; //max files
  int FSNAME_SIZE = FS_NAME_SIZE; //max file name len
  byte MAGIC_BYTE_ADDR = 0; //address if magic value
  byte MAGIC_VALUE = 0x2A; //format ok magic value


public:
  VSEEPROMFS() {
    nextAddress = TABLE_START + (sizeof(FileEntry) * FSMAX_FILES);
  }

  
  void begin(void) {
    if (!isFormatted()) format();
  
     //next slot free, search all slots
      int lastReserved = TABLE_START + (sizeof(FileEntry) * FSMAX_FILES);
      for (int i = 0; i < FSMAX_FILES; i++) {
        FileEntry entry;
        EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
        if (entry.active) {
          int endOfFile = entry.startAddress + entry.fileSize;
          if (endOfFile > lastReserved) lastReserved = endOfFile;
        }
      }
      nextAddress = lastReserved;
        //Serial.print(F("next addr: "));
        //Serial.println(nextAddress);
  }

  bool isFormatted(void) {
      return EEPROM.read(MAGIC_BYTE_ADDR) == MAGIC_VALUE;
  }
  
  void format(void) {
    Serial.println(F("EEPROM Formating..."));
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    
     EEPROM.write(MAGIC_BYTE_ADDR, MAGIC_VALUE);//save signature after format
     nextAddress = TABLE_START + (sizeof(FileEntry) * FSMAX_FILES);
     Serial.println(F("EEPROM Formated!"));
  }

bool writeFile(const char* name, const char* content) {
  if (!isFormatted()) return false;
  
  int sizeNeeded = strlen(content);
  int slotToUse = -1;
  int targetAddr = -1;

  //find disabled slot that can handle the space
  for (int i = 0; i < FSMAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
    
    // if slot is disabled and size get into the old space slot
    if (!entry.active && entry.fileSize >= sizeNeeded && entry.startAddress > 0) {
      slotToUse = i;
      targetAddr = entry.startAddress;
      //Serial.print(F("Reuse slot ")); Serial.println(i);
      break; 
    }
  }

  // if not found a disabled slot get next free slot
  if (slotToUse == -1) {
    // check is free space exists
    if (freeSpace() < sizeNeeded) {
      Serial.println(F("Error, no free space!"));
      return false;
    }
    
    // get the first free slot
    for (int i = 0; i < FSMAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      if (!entry.active) {
        slotToUse = i;
        targetAddr = nextAddress;
        nextAddress += sizeNeeded;
        break;
      }
    }
  }

  // save data in new or old slot
  if (slotToUse != -1) {
    FileEntry newFile;
    memset(newFile.name, 0, FSNAME_SIZE);
    strncpy(newFile.name, name, FSNAME_SIZE - 1);
    newFile.startAddress = targetAddr;
    newFile.fileSize = sizeNeeded;
    newFile.active = true;

    // save data
    for (int j = 0; j < sizeNeeded; j++) {
      EEPROM.write(targetAddr + j, content[j]);
    }

    //update table
    EEPROM.put(TABLE_START + (slotToUse * sizeof(FileEntry)), newFile);
    return true;
  }

  Serial.println(F("Error table Full!"));
  return false;
}
  /*
  bool writeFileold(const char* name, const char* content) {
    if (!isFormatted()) return false;
    
    int size = strlen(content);
    int targetIdx = -1;
    FileEntry temp;

    // find if free slot exists
    for (int i = 0; i < FSMAX_FILES; i++) {
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), temp);
      if (temp.active && strcmp(temp.name, name) == 0) {
        targetIdx = i;
        break;
      }
      if (!temp.active && targetIdx == -1) targetIdx = i;
    }

    if (targetIdx != -1) {
      FileEntry newFile;
      memset(newFile.name, 0, FSNAME_SIZE);
      strncpy(newFile.name, name, FSNAME_SIZE - 1);
      
      // rewrite file, if same size or less mantain address
      if (temp.active && strcmp(temp.name, name) == 0 && size <= temp.fileSize) {
          newFile.startAddress = temp.startAddress;
      } else {
          newFile.startAddress = nextAddress;
          nextAddress += size;
      }
      
      newFile.fileSize = size;
      newFile.active = true;

      // save file
      for (int j = 0; j < size; j++) {
        EEPROM.write(newFile.startAddress + j, content[j]);
      }

      // save metadata is adress
      EEPROM.put(TABLE_START + (targetIdx * sizeof(FileEntry)), newFile);
      return true;
    }
    return false;
  }
*/
  bool readFile(const char* name, char* buffer, int bufferSize) {
    if (!isFormatted()) return false;

    for (int i = 0; i < FSMAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);

      if (entry.active && strcmp(entry.name, name) == 0) {
        if (bufferSize < entry.fileSize + 1) return false;
        
        for (int j = 0; j < entry.fileSize; j++) {
          buffer[j] = (char)EEPROM.read(entry.startAddress + j);
        }
        buffer[entry.fileSize] = '\0';
        return true;
      }
    }
    return false;
  }

bool deleteFile(const char* name) {
  for (int i = 0; i < FSMAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
    
    if (entry.active && strcmp(entry.name, name) == 0) {
      entry.active = false; // mark as inactive slot
      EEPROM.put(TABLE_START + (i * sizeof(FileEntry)), entry);
      Serial.print(F("File deleted: ")); Serial.println(name);
      return true;
    }
  }
  return false;
}

 void listAllFiles(void) {
    if (!isFormatted()) return;
    Serial.println(F("\n--- FS LIST ---"));
    for (int i = 0; i < FSMAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      if (entry.active) {
        Serial.print(F("File: ")); Serial.print(entry.name);
        Serial.print(F(" | Size: ")); Serial.println(entry.fileSize);
      }
    }
  }

  bool exists(const char* name) {
    for (int i = 0; i < FSMAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      
      // file exists and slot is on
      if (entry.active && strcmp(entry.name, name) == 0) {
        return true;
      }
    }
    return false;
  }

  int freeSpace(void) {
    if (!isFormatted()) return 0;
    
    int free = EEPROM_SIZE - nextAddress;  
    //do it safe
    return (free > 0) ? free : 0;
  }

  //get free splots
  int freeSlots(void) {
    int count = 0;
    for (int i = 0; i < FSMAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      if (!entry.active) count++;
    }
    return count;
  }

  bool safeWrite(const char* name, const char* content) {
    int sizeNeeded = strlen(content);
    
    if (freeSpace() < sizeNeeded) {
      Serial.println(F("No free EEPROM!"));
      return false;
    }
    return writeFile(name, content);
  }

//get disable slots count
int getFreeSlotsCount(void) {
  if (!isFormatted()) return 0;
  
  int count = 0;
  for (int i = 0; i < FSMAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
    
    if (!entry.active) {
      count++;
    }
  }
  return count;
}

void listFreeSlotIndexes(void) {
  if (!isFormatted()) return;
  
  Serial.print(F("Free Slots: "));
  bool first = true;
  for (int i = 0; i < FSMAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
    
    if (!entry.active) {
      if (!first) Serial.print(F(", "));
      Serial.print(i);
      first = false;
    }
  }
  Serial.println();
}

bool isDirectory(const char* path) {
  if (!isFormatted()) return false;

  // search is path end with '/'
  String searchPath = String(path);
  if (!searchPath.endsWith("/")) {
    searchPath += "/";
  }

  for (int i = 0; i < FSMAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);

    // if active and starts with path
    if (entry.active && strstr(entry.name, searchPath.c_str()) == entry.name) {
      return true;
    }
  }
  
  return false;
}

void hexdump(void) {
  
  Serial.println(F("\n--- EEPROM HEXDUMP ---"));
  // Serial.println(F("Addr      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  |ASCII|"));
  Serial.println(F("------------------------------------------------------------------------"));

  for (int i = 0; i < EEPROM_SIZE; i += 16) {
    // print adress Hex (Ex: 0000, 0010...)
    if (i < 0x100) Serial.print('0');
    if (i < 0x10)  Serial.print('0');
    Serial.print(i, HEX);
    Serial.print(F("  "));

    char asciiBuffer[17]; // Buffer for ASCII
    asciiBuffer[16] = '\0';

    //first 16 bytes of line in hex
    for (int j = 0; j < 16; j++) {
      int addr = i + j;
      if (addr < EEPROM_SIZE) {
        byte val = EEPROM.read(addr);
        
        if (val < 0x10) Serial.print('0');
        Serial.print(val, HEX);
        Serial.print(' ');

        // save ASCII colum
        if (val >= 32 && val <= 126) {
          asciiBuffer[j] = (char)val;
        } else {
          asciiBuffer[j] = '.'; // hidden chars!
        }
      } else {
        // if no multiple of 16, write some spaces
        Serial.print(F("   "));
        asciiBuffer[j] = ' ';
      }
    }

    // print ASCII colum
    Serial.print(F(" |"));
    Serial.print(asciiBuffer);
    Serial.println('|');
    
    // little break to buffer dont frezes serial at low rate
    if (i % 128 == 0) delay(10);
  }
  Serial.println(F("------------------------------------------------------------------------"));
}

};
