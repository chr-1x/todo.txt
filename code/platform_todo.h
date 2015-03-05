#if !defined(PLATFORM_TODO_H)

#include "todo.h"

enum print_color_value
{
	COLOR_DEFAULT,
	COLOR_RED,
	COLOR_YELLOW,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_BLACK,
	COLOR_WHITE
};
struct print_color
{
	print_color_value Color;
	bool32 IsIntense;
};

internal print_color
PrintColor(print_color_value Value, bool32 IsIntense=false)
{
	print_color Result = { Value, IsIntense };
	return Result;
}

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
PlatformPrint(print_color ForegroundColor, print_color BackgroundColor, size_t Length, char* String);

internal bool32
PlatformPrintFormatted(print_color ForegroundColor, print_color BackgroundColor, char* FormatString, ...);

#define PrintF(...) PlatformPrintFormatted(PrintColor(COLOR_WHITE), PrintColor(COLOR_BLACK), __VA_ARGS__)
#define PrintFC(FgColor, ...) PlatformPrintFormatted(FgColor, PrintColor(COLOR_BLACK), __VA_ARGS__)
#define PrintFCB PlatformPrintFormatted

internal bool32
PlatformError(char* ErrorMessage);

#define PLATFORM_TODO_H
#endif