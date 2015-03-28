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


int64
StringIndexOf(size_t HaystackCount, char* Haystack, size_t NeedleCount, char* Needle, int64 LowerBound = 0)
{
    size_t NeedleIndex = 0;

    for (size_t i = LowerBound;
        i < HaystackCount;
        ++i)
    {
        if (Haystack[i] == Needle[NeedleIndex])
        {
            ++NeedleIndex;
            if (NeedleIndex >= NeedleCount)
            {
                return (i+1) - NeedleCount;
            }
        }
        else 
        {
            if (NeedleIndex > 0)
            {
                NeedleIndex = 0;
            }
        }
    }
    return -1;
}

// Automates the finding of the length of C-style strings. Better to call the other if you already know the lengths.
int64 
StringIndexOf(char* Haystack, char* Needle, int64 LowerBound = 0)
{
    return StringIndexOf(StringLength(Haystack), Haystack, StringLength(Needle), Needle, LowerBound);
}

int64
StringLastIndexOf(size_t HaystackCount, char* Haystack, size_t NeedleCount, char* Needle, int64 UpperBound = -1)
{
    int64 NeedleIndex = NeedleCount - 1;
    if (UpperBound < 0) { UpperBound = HaystackCount - 1; }

    for (int64 i = UpperBound;
        i >= 0;
        --i)
    {
        if (Haystack[i] == Needle[NeedleIndex])
        {
            --NeedleIndex;
            if (NeedleIndex < 0)
            {
                return (i+1);
            }
        }
        else 
        {
            if (NeedleIndex < (int64)(NeedleCount) - 1)
            {
                NeedleIndex = NeedleCount - 1;
            }
        }
    }
    return -1;
}

int64 StringLastIndexOf(char* Haystack, char* Needle, int64 UpperBound = -1)
{
    return StringLastIndexOf(StringLength(Haystack), Haystack, StringLength(Needle), Needle, UpperBound);
}

int32 
StringOccurrences(size_t SourceCount, char* Source, size_t SearchCount, char* Search)
{
    int64 Index = 0;
    int Occurrences = 0;
    while ((Index = StringIndexOf(SourceCount, Source, SearchCount, Search, Index+1)) >= 0)
    {   
        ++Occurrences;
    }
    
    return Occurrences;
}

int32 StringOccurrences(char* Source, char* Search)
{
    return StringOccurrences(StringLength(Source), Source, StringLength(Search), Search);
}

int 
StringReplace(size_t SourceCount, char* Source, 
              size_t DestCount, char* Dest, 
              size_t TokenCount, char* Token, 
              size_t ReplacementCount, char* Replacement)
{
    int OccurrenceCount = StringOccurrences(SourceCount, Source, TokenCount, Token);
    if (OccurrenceCount > 0)
    {
        size_t Delta = (ReplacementCount - TokenCount) * OccurrenceCount; // Remember, might be negative
        size_t NewLength = SourceCount + Delta;

        Assert(DestCount >= NewLength);

        size_t LastIndex = 0;
        int64 NextIndex = 0;
        size_t NewStringIndex = 0;
        while ((NextIndex = StringIndexOf(SourceCount, Source, TokenCount, Token, (int)NextIndex + 1)) >= 0)
        {
            //Copy in from the original string, up to the next token occurrence
            CopyString(NextIndex - LastIndex, Source + LastIndex, NextIndex - LastIndex, Dest + NewStringIndex);
            NewStringIndex += (NextIndex - LastIndex);

            //Copy in from the replacement instead of copying the token from the original
            CopyString(ReplacementCount, Replacement, ReplacementCount, Dest + NewStringIndex);
            NewStringIndex += ReplacementCount;

            // Account for the token before moving on
            LastIndex = NextIndex + TokenCount; 
        }

        CopyString(SourceCount - LastIndex, Source + LastIndex, NewLength - NewStringIndex, Dest + NewStringIndex);

        Assert(NewStringIndex + (SourceCount - LastIndex) == NewLength);
    }

    return OccurrenceCount;
}

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