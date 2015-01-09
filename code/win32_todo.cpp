#include "chr.h"
#include "todo.cpp"

global_variable HANDLE MyHeap;

internal void* 
PlatformAllocMemory(size_t BytesToAlloc, bool32 ZeroTheMemory)
{
    return HeapAlloc(MyHeap, ZeroTheMemory ? HEAP_ZERO_MEMORY : 0, BytesToAlloc);
}
//NOTE(chronister): Default behavior: Don't zero it.
internal void* PlatformAllocMemory(size_t BytesToAlloc) { return PlatformAllocMemory(BytesToAlloc, false); }

internal void
PlatformFreeMemory(void* Memory)
{
    if (Memory) {
        HeapFree(MyHeap, 0, Memory);
    }
}

internal read_file_result
PlatformReadEntireFile(char* Filename)
{
    read_file_result Result = {};

    HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
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
        print("Unable to open %s", Filename);
    }

    return Result;
}

internal bool32
PlatformAppendToFile(char* Filename, size_t StringSize, char* StringToAppend)
{
	bool32 Result = false;

    HANDLE FileHandle = CreateFile(Filename, FILE_APPEND_DATA, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
PlatformWriteEntireFile(char* Filename, size_t StringSize, char* StringToWrite)
{
    bool32 Result = false;

    Assert(StringSize < UINT32_MAX);

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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


int main(int argc, char* argv[])
{
    MyHeap = GetProcessHeap();
    RunFromArguments(argc, argv);
}