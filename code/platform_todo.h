#if !defined(PLATFORM_TODO_H)

#include "todo.h"


internal void* 
PlatformAllocMemory(size_t BytesToAlloc, bool32 ZeroTheMemory);
internal void* PlatformAllocMemory(size_t BytesToAlloc);

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

#define PLATFORM_TODO_H
#endif