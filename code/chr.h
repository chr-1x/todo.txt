/*
    An experiment in creating a personal standard library.

    Created by Andrew Chronister for personal use. Mileage may vary.
    Please properly attribute, etc, etc
*/
#if !defined(CHR)

//TODO(chronister): Find reliable ways to get these without stdint.h
#include "stdint.h"

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

//TODO(chronister): Find platform-specific ways to do output (for educational purposes?)
#include "stdio.h"
#define print printf

//NOTE(chronister): I'll selectively use some of the alternative tokens because I also code in python
#ifdef _WIN32
    #define and &&
    #define or ||
    #define xor ^
#endif
//TODO(chronister): GCC seems to have these tokens already?

#if !defined(null)
    #define null 0
#endif

#define global_variable static
#define internal static
#define local_persist static

//TODO(chronister): Fully fledged assertion macro that also checks Debug vs Release
#define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }

#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define Swap(A, B, T) { T temp##T = A; A = B; B = temp##T; }

#define foreach(T, iter, ArrayCount, Array) T iter = Array[0]; for (int i = 0; i < (int64)ArrayCount; ++i, iter = Array[i])

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    //TODO(chronister): Defines for max values
    Assert(Value <= UINT32_MAX);
    return((uint32)Value);
}

/* =====================
      Math Operations 
   ===================== */

//TODO(chronister): Generalize these to all number types!

//TODO(chronister): Overflow?
//TODO(chronister): Intrinsics?
int32
Pow(int32 Value, int32 Power)
{
    int32 Result = 1;
    while(Power-- > 0)
    {
        Result *= Value;
    }
    return Result;
}

#define Log10(Value) Log(Value, 10)

int32
Log(int32 Value, int32 Base)
{
    if (Value < Base) { return 0; }
    int32 Result = 0;
    while (Value > Base - 1)
    {
        Value /= Base;
        Result++;
    }
    return Result;
}

/* ==========================
      Memory Operations
   ========================== */

#if !defined(CopyMemory)
    void 
    CopyMemory(void* Dest, void* Source, size_t Count)
    {
        uint8* S = (uint8*)Source;
        uint8* D = (uint8*)Dest;

        for (int Index = 0;
            Index < Count;
            ++Index)
        {
            *D++ = *S++;
        }
    }
#endif 

/* ==========================
      String Operations
   ========================== */

bool32
CompareStrings(char* String1, char* String2)
{
    int i;
    for (i = 0;
        String1[i] and String2[i];
        ++i)
    {
        if (String1[i] != String2[i]) { return false; }
    }
    if (String1[i] != String2[i]) { return false; } // For strings of different lengths
    return true;
}

int
StringLength(char* String)
{
    int Count = 0;
    while (*(String++))
    {
        ++Count;
    }
    return Count;
}

void
CatStrings(size_t SourceACount, char* SourceA, size_t SourceBCount, char* SourceB, size_t DestCount, char* Dest)
{
    
    for (int Index = 0;
        Index < Min(SourceACount, DestCount);
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (int Index = 0;
        Index < Min(SourceBCount, DestCount - SourceACount);
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

void 
CopyString(size_t SourceCount, char* Source, size_t DestCount, char* Dest)
{
    for (int Index = 0;
        Index < Min(SourceCount, DestCount);
        ++Index)
    {
        *Dest++ = *Source++;
    }

    *Dest++ = 0;
}

/*
int32
StringIndexOf(char* Haystack, char* Needle)
{
    char* Start = null;
    for (int i = 0;
        Haystack[i];
        ++i)
    {
        if (String1[i] != String2[i]) { return false; }
    }
    return -1;
}*/

/* =====================
      Number Parsing 
   ===================== */

//TODO(chronister): Generalize these?

struct parse_int_result
{
    int64 Value;
    bool32 Valid;
};

parse_int_result
ParseInteger(int32 TextLength, char* Text)
{
    parse_int_result Result = {};
    Result.Valid = true;

    int sign = 1;
    if (*Text == '-')
    { 
        sign = -1; 
        ++Text;
        --TextLength;
    }
    for (int i = 0;
        i < TextLength;
        ++i)
    {
        char C = Text[i];
        if (C >= 48 && C <= 57)
        {
            Result.Value += (C - 48) * Pow(10, TextLength - i - 1);
        }
        else 
        {
            Result.Valid = false;
            Result.Value = 0;
            return Result;
        }
    }

    return Result;
}

/* ===============================
    Standard, useful Win32 things
   =============================== */
/*
    Some standard terminology:
     - Filename: just a files's name (todo.txt)
     - Filepath|path: file name + full directory
     - Directory: full directory path
*/
#ifdef _WIN32

    #include "windows.h"

    #define WIN32_FILENAME_SIZE MAX_PATH
        
    inline FILETIME
    Win32GetLastWriteTime(char *Filename)
    {
        FILETIME LastWriteTime = {};

        WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
        if (GetFileAttributesExA(Filename, GetFileExInfoStandard, &FileAttributes))
        {
            LastWriteTime = FileAttributes.ftLastWriteTime;
        }

        return LastWriteTime;
    }

    void
    Win32GetEXEPath(char* FilepathOut)
    {
        //TODO(chronister): test this and figure out if its even necessary
        DWORD SizeOfFilename = GetModuleFileName(0, FilepathOut, WIN32_FILENAME_SIZE);
    }

#endif


#ifdef __linux__

    #include "fcntl.h"
    #include "sys/stat.h"
    #include "sys/types.h"
    #include "unistd.h"
    #include "pwd.h"
    

#endif

#define CHR
#endif