# VSEEPROMFS
Simple File System to use with internal EEPROM in Arduino

This is some simple fileSytem library to work with internal EEPROM's using EEPROM.h lib from arduino
Supports Format,Read,Write,Delete,List Files and others
All directory are virtual and part of the filename.
The eeprom space will be divided in slots, if some file was deleted, that slot becomes disabled
If new file will be created, checks if that file can get in some free space, yes then the slot is reused, if not new slot created
In format os init, first it alloc space to file names MAX_FILES*NAME_SIZE, then all remain free space are data
