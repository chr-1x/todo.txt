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
/*
    "This is a |Rxx_xgb`test` %d"
    
    (do printf substitution)

    (get first index of |):
        for character in | to |+3
            if (character != '|' or '_' or '`')
                color |= determinecolor(character, true)
    (get first index of _):
        ''
            ''
                color |= '', false)


*/

internal int16
WinColorFromCharacter(char Character, bool32 Foreground=true)
{
    switch (Character)
    {
        case 'r':
            return Foreground ? FOREGROUND_RED : BACKGROUND_RED;
        case 'R':
            return Foreground ? FOREGROUND_RED | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_INTENSITY;
        case 'g':
            return Foreground ? FOREGROUND_GREEN : BACKGROUND_GREEN;
        case 'G':
            return Foreground ? FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_INTENSITY;\
        case 'b':
            return Foreground ? FOREGROUND_BLUE : BACKGROUND_BLUE;
        case 'B':
            return Foreground ? FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        default:
            return 0;
    }
}

internal bool32
PlatformPrintColored(size_t Length, char* String)
{
    bool32 Result;
    int16 WinColor;
    uint32 CharsWritten;
    uint32 CharsToWrite;

    int32 i = 0;
    for (char* C = String; *C != '\0' && i < Length; ++C, ++i)
    {
        if (*C == '`')
        {
            char ColorString[9] = {};
			char* StringEnd = Max(C - 8, String);
            CopyString(C - StringEnd, StringEnd, 8, ColorString);

            WinColor = 0;
            int32 CharsToSkip = 0;

            int32 LastForeground = (int32)StringLastIndexOf(ColorString, "|");
            if (LastForeground >= 0)
            {   
                for (char* Col = ColorString + LastForeground; *Col != '\0' && *Col != '_'; ++Col)
                {
                    WinColor |= WinColorFromCharacter(*Col, true);
                }
            }

            int32 LastBackground = (int32)StringLastIndexOf(ColorString, "_");
            if (LastBackground >= 0)
            {
                for (char* Col = ColorString + LastBackground; *Col != '\0' && *Col != '|'; ++Col)
                {
                    WinColor |= WinColorFromCharacter(*Col, false);
                }
            }

            if (LastBackground >= 0 || LastForeground >= 0)
            {
                CharsToSkip = (int32)(C - StringEnd + 1) - (int32)Min((uint32)LastBackground, (uint32)LastForeground);
            }
            else
            {
                WinColor = FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
            }

            uint32 CharsToWrite = (uint32)Max(((C - String)) - (int64)CharsToSkip, 0);
            Result &= WriteConsole(ConsoleOut, String, (uint32)CharsToWrite, (LPDWORD)&CharsWritten, NULL);
            Result &= (CharsWritten == CharsToWrite);

            String = ++C;

            SetConsoleTextAttribute(ConsoleOut, WinColor);
        }
    }

    CharsToWrite = StringLength(String);
    Result &= WriteConsole(ConsoleOut, String, (uint32)CharsToWrite, (LPDWORD)&CharsWritten, NULL);
    Result &= (CharsWritten == CharsToWrite);

    SetConsoleTextAttribute(ConsoleOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
    return Result;
}

internal bool32
PlatformPrintFormatted(char* FormatString, ...)
{
    size_t Length = StringLength(FormatString);
    size_t FormattedLength = Max(Length*2, 256); // TODO(Chronister): This allocation
    char* ScratchBuffer = (char*)PlatformAllocMemory(FormattedLength, false);
    
    va_list args;
    va_start(args, FormatString);
    vsprintf_s(ScratchBuffer, FormattedLength, FormatString, args);
    va_end(args);

    bool32 Result = PlatformPrintColored(FormattedLength, ScratchBuffer);
    PlatformFreeMemory(ScratchBuffer);

    return Result;
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