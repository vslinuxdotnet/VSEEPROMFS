/*
Some Simple FileSytem Library to work with internal EEPROM's
Supports Format,Read,Write,Delete,List Files
All directory are virtual and part of filename.
The space will be divided in slots, if some file was deleted, that slot becomes disabled, then if new file can get in the free space the slot is reused.

VSEEPROMFS V0.3
*/
#include <Arduino.h>

#define MAX_FILES 10 //max files
#define NAME_SIZE 16 //max file name
#define EEPROM_SIZE 1024 //max size of internal eeprom
#define MAGIC_BYTE_ADDR 0 //address if magic value
#define MAGIC_VALUE 0x55 //format ok magic value

class VSEEPROMFS {
  
private:
  int nextAddress;
  const int TABLE_START = 1;

public:
  VSEEPROMFS() {
    nextAddress = TABLE_START + (sizeof(FileEntry) * MAX_FILES);
  }
  
  void begin() {
    if (!isFormatted()) format();
  
     //next slot free, search all slots
      int lastReserved = TABLE_START + (sizeof(FileEntry) * MAX_FILES);
      for (int i = 0; i < MAX_FILES; i++) {
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

  bool isFormatted() {
      return EEPROM.read(MAGIC_BYTE_ADDR) == MAGIC_VALUE;
  }
  
  void format() {
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    
     EEPROM.write(MAGIC_BYTE_ADDR, MAGIC_VALUE);//save signature after format
     nextAddress = TABLE_START + (sizeof(FileEntry) * MAX_FILES);
     Serial.println(F("EEPROM Formated!"));
  }

bool writeFile(const char* name, const char* content) {
  if (!isFormatted()) return false;
  
  int sizeNeeded = strlen(content);
  int slotToUse = -1;
  int targetAddr = -1;

  //find disabled slot that can handle the space
  for (int i = 0; i < MAX_FILES; i++) {
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
    for (int i = 0; i < MAX_FILES; i++) {
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
    memset(newFile.name, 0, NAME_SIZE);
    strncpy(newFile.name, name, NAME_SIZE - 1);
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
    for (int i = 0; i < MAX_FILES; i++) {
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), temp);
      if (temp.active && strcmp(temp.name, name) == 0) {
        targetIdx = i;
        break;
      }
      if (!temp.active && targetIdx == -1) targetIdx = i;
    }

    if (targetIdx != -1) {
      FileEntry newFile;
      memset(newFile.name, 0, NAME_SIZE);
      strncpy(newFile.name, name, NAME_SIZE - 1);
      
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

    for (int i = 0; i < MAX_FILES; i++) {
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
  for (int i = 0; i < MAX_FILES; i++) {
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

 void listAllFiles() {
    if (!isFormatted()) return;
    Serial.println(F("\n--- FS LIST ---"));
    for (int i = 0; i < MAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      if (entry.active) {
        Serial.print(F("File: ")); Serial.print(entry.name);
        Serial.print(F(" | Size: ")); Serial.println(entry.fileSize);
      }
    }
  }

  bool exists(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
      FileEntry entry;
      EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
      
      // file exists and slot is on
      if (entry.active && strcmp(entry.name, name) == 0) {
        return true;
      }
    }
    return false;
  }

  int freeSpace() {
    if (!isFormatted()) return 0;
    
    int free = EEPROM_SIZE - nextAddress;  
    //do it safe
    return (free > 0) ? free : 0;
  }

  //get free splots
  int freeSlots() {
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
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
int getFreeSlotsCount() {
  if (!isFormatted()) return 0;
  
  int count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);
    
    if (!entry.active) {
      count++;
    }
  }
  return count;
}

void listFreeSlotIndexes() {
  if (!isFormatted()) return;
  
  Serial.print(F("Free Slots: "));
  bool first = true;
  for (int i = 0; i < MAX_FILES; i++) {
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

  for (int i = 0; i < MAX_FILES; i++) {
    FileEntry entry;
    EEPROM.get(TABLE_START + (i * sizeof(FileEntry)), entry);

    // if active and starts with path
    if (entry.active && strstr(entry.name, searchPath.c_str()) == entry.name) {
      return true;
    }
  }
  
  return false;
}

};
