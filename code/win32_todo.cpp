#include "chr.h"
#include "todo.cpp"
#include "stdarg.h"

global_variable HANDLE MyHeap;
global_variable HANDLE ConsoleOut;
global_variable HANDLE ConsoleIn;
global_variable HANDLE ConsoleError;

internal void* 
PlatformAllocMemory(size_t BytesToAlloc, bool32 ZeroTheMemory)
{
    return HeapAlloc(MyHeap, ZeroTheMemory ? HEAP_ZERO_MEMORY : 0, BytesToAlloc);
}

internal bool32
PlatformFreeMemory(void* Memory)
{
    bool32 Result = false;
    if (Memory) {
        Result = HeapFree(MyHeap, 0, Memory);
    }
    return Result;
}

internal bool32
PlatformFileExists(char* Filepath)
{
    WIN32_FIND_DATA Ignored;
    if (FindFirstFile(Filepath, &Ignored) != INVALID_HANDLE_VALUE)
    {
        return true;
    }
    return false;
}

#include "shlobj.h"

internal string
PlatformGetUserDir()
{
    char Path[WIN32_FILENAME_SIZE];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, Path))) 
    {
        string Result;
        uint32 PathLen = StringLength(Path);
        Result.Length = PathLen + 1;
        Result.Value = (char*)PlatformAllocMemory(Result.Length);
        CopyString(PathLen, Path, Result.Length, Result.Value);
        CatStrings(PathLen, Result.Value, 1, "/", Result.Length, Result.Value);

        return Result;
    }
    else
    {
        //TODO(chronister): Proper logging!
        string Nil = {0};
        return Nil;
    }
}

internal read_file_result
PlatformReadEntireFile(char* Filepath)
{
    read_file_result Result = {};

    HANDLE FileHandle = CreateFile(Filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = (char*)PlatformAllocMemory(FileSize32);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                    (FileSize32 == BytesRead)) 
                {
                    Result.ContentsSize = FileSize32;
                }
                else 
                {
                    //TODO(chronister): Logging
                    PlatformFreeMemory(Result.Contents);
                    Result.Contents = 0;
                }
                
            }
            else 
            {
                //TODO(chronister): Proper logging
                print("Unable to read file");
            }
        }
        else 
        {
            //TODO(chronister): Proper logging
            print("Unable to read file");
        }

        CloseHandle(FileHandle);
    }
    else 
    {
        //TODO(chronister): Proper logging
        print("Unable to open %s", Filepath);
    }

    return Result;
}

internal bool32
PlatformAppendToFile(char* Filepath, size_t StringSize, char* StringToAppend)
{
	bool32 Result = false;

    HANDLE FileHandle = CreateFile(Filepath, FILE_APPEND_DATA, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, StringToAppend, StringLength(StringToAppend), &BytesWritten, 0)) 
        {
            //NOTE(handmade): File appended successfully
            Result = (BytesWritten == StringSize);
        }
        else 
        {
            //TODO(handmade): Logging
            print("Unable to write to todo.txt");
        }

        CloseHandle(FileHandle);
    }
    else 
    {
        //TODO(chronister): Proper logging
        print("Unable to open todo.txt for appending");
    }

	return Result;
}

internal bool32
PlatformWriteEntireFile(char* Filepath, size_t StringSize, char* StringToWrite)
{
    bool32 Result = false;

    Assert(StringSize < UINT32_MAX);

    HANDLE FileHandle = CreateFileA(Filepath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {

        DWORD BytesWritten;
        if (WriteFile(FileHandle, StringToWrite, (DWORD)StringSize, &BytesWritten, 0)) 
        {
            //NOTE(chronister): File read successfully
            Result = (BytesWritten == StringSize);
        }
        else 
        {
            //TODO(chronister): Proper Logging
            print("Unable to write to todo.txt!");
        }

        CloseHandle(FileHandle);
    }
    else 
    {
        //TODO(chronister): Proper Logging
        print("Unable to open todo.txt for writing!");
    }

    return(Result);
}

internal int16
PrintColorToWinColor(print_color PrintColor, bool32 IsForeground)
{
    int16 ResultColor = 0;
    switch(PrintColor.Color)
    {
        case COLOR_RED: 
        case COLOR_YELLOW:
        case COLOR_MAGENTA:
        case COLOR_WHITE:
        {
            ResultColor |= (IsForeground ? FOREGROUND_RED : BACKGROUND_RED);
        }
    }
    switch(PrintColor.Color)
    {
        case COLOR_YELLOW: 
        case COLOR_GREEN:
        case COLOR_CYAN:
        case COLOR_WHITE:
        {
            ResultColor |= (IsForeground ? FOREGROUND_GREEN : BACKGROUND_GREEN);
        } break;
    }
    switch(PrintColor.Color)
    {
        case COLOR_CYAN: 
        case COLOR_BLUE:
        case COLOR_MAGENTA:
        case COLOR_WHITE:
        {
            ResultColor |= (IsForeground ? FOREGROUND_BLUE : BACKGROUND_BLUE);
        } break;
    }
    return ResultColor;
}

internal bool32
PlatformPrint(print_color ForegroundColor, print_color BackgroundColor, size_t Length, char* String)
{
    int16 WinFgColor = (ForegroundColor.IsIntense ? FOREGROUND_INTENSITY : 0);
    int16 WinBgColor = (BackgroundColor.IsIntense ? BACKGROUND_INTENSITY : 0);
    WinFgColor |= PrintColorToWinColor(ForegroundColor, true);
    WinBgColor |= PrintColorToWinColor(BackgroundColor, false);

    SetConsoleTextAttribute(ConsoleOut, WinFgColor | WinBgColor);
    uint32 CharsWritten;
    bool32 Result = WriteConsole(ConsoleOut, String, (uint32)Length, (LPDWORD)&CharsWritten, NULL);
    SetConsoleTextAttribute(ConsoleOut, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
    
    return Result && CharsWritten == Length;
}

internal bool32
PlatformPrintFormatted(print_color ForegroundColor, print_color BackgroundColor, char* FormatString, ...)
{
    size_t Length = StringLength(FormatString);
    char* ScratchBuffer = (char*)PlatformAllocMemory(Max(Length*2, 256), false);
    
    va_list args;
    va_start(args, FormatString);
    vsprintf_s(ScratchBuffer, Max(Length*2, 256), FormatString, args);
    va_end(args);

    PlatformFreeMemory(ScratchBuffer);

    return PlatformPrint(ForegroundColor, BackgroundColor, StringLength(ScratchBuffer), ScratchBuffer);
}

internal bool32
PlatformError(char* ErrorMessage)
{
    size_t ErrorLength = StringLength(ErrorMessage);
    SetConsoleTextAttribute(ConsoleError, FOREGROUND_RED);
    uint32 CharsWritten;
    bool32 Result = WriteConsole(ConsoleError, ErrorMessage, (uint32)ErrorLength, (LPDWORD)&CharsWritten, NULL);
    SetConsoleTextAttribute(ConsoleError, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
    
    return Result && CharsWritten == ErrorLength;
}


int main(int argc, char* argv[])
{
    MyHeap = GetProcessHeap();
    ConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    ConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleError = GetStdHandle(STD_ERROR_HANDLE);

    RunFromArguments(ParseArgs(argc, argv));
}