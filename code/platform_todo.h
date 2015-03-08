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

/* NOTE(Chronister):
  The syntax for color printing is a custom defined format that works as follows:
	  "This is |rg_RGB`Yellow text on bright white background` Normal text"
  Essentially, when a '`' is found, it looks back for a | and a _. r, g, and b found
    after each enable red, green, and blue flags on the text. If one or more of these
    are capitalized, an intensity flag is set. Characters other than r, g, and b are
    ignored, so you can use placeholders for colors you want to explicitly show as 
    not being present, for example |---_rgb` for black text on white background.
  Note that if a background or foreground color is not specified and the format string
    is present directly after a pipe | or an underscore _, the pipe or underscore may
    be interpreted as an empty format specifier and skipped.
*/
internal bool32
PlatformColorPrint(size_t Length, char* String);

internal bool32
PlatformColorPrintFormatted(char* FormatString, ...);

#define PrintFC PlatformColorPrintFormatted

internal string
PlatformReadLine();

internal bool32
PlatformError(char* ErrorMessage);

#define PLATFORM_TODO_H
#endif