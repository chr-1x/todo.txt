#if !defined(PLATFORM_TODO_H)

#include "todo.h"

internal void* 
PlatformAllocMemory(size_t BytesToAlloc, bool32 ZeroTheMemory=false);

internal bool32
PlatformFreeMemory(void* Memory);

internal bool32
PlatformFileExists(char* Filename);

internal string
PlatformGetUserDir();

internal read_file_result
PlatformReadEntireFile(char* Filename);

internal bool32
PlatformAppendToFile(char* Filename, size_t StringSize, char* StringToAppend);

internal bool32
PlatformWriteEntireFile(char* Filename, size_t StringSize, char* StringToWrite);

internal bool32
PlatformColorPrint(size_t Length, char* String);

internal bool32
PlatformColorPrintFormatted(char* FormatString, ...);

#define PrintFC PlatformColorPrintFormatted

internal bool32
PlatformError(char* ErrorMessage);

#define PLATFORM_TODO_H
#endif