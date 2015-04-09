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

#define global_variable static
#define internal static
#define local_persist static

//TODO(chronister): Fully fledged assertion macro that also checks Debug vs Release
#if INTERNAL
    #define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
#else
    #define Assert(Expression) /* */
    //TODO(chronister): Logging
#endif
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define Swap(A, B, T) { T temp##T = A; A = B; B = temp##T; }

#define foreach(T, iter, ArrayCount, Array) T* iter = 0; if (Array) { iter = &((Array)[0]); } for (int i##iter = 0; i##iter < (int64)ArrayCount; ++i##iter, iter = &((Array)[i##iter]))
#define whilecount(Expression, MaxIterations) uint64 _LoopCount = 0; while((Expression) && _LoopCount++ < MaxIterations)

#define MS_PER_SEC(S) ((S) * 1000)
#define MS_PER_MIN(M) (MS_PER_SEC(M) * 60)
#define MS_PER_HOUR(H) (MS_PER_MIN(H) * 60)

#define S_PER_MIN(M) ((M) * 60)
#define S_PER_HOUR(H) (S_PER_MIN(H) * 60)
#define S_PER_DAY(D) (S_PER_HOUR(D) * 24)

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

#define CopyArray(T, Dest, Source, Count) CopyMemory((void*)Dest, (void*)Source, Count*sizeof(T))

#if !defined(ZeroMemory)
    void
    ZeroMemory(void* MemPtr, size_t BytesToClear)
    {
        uint8* M = (uint8*)MemPtr;
        for (int Index = 0;
            Index < BytesToClear;
            ++Index)
        {
            *M++ = 0;
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
        String1[i] && String2[i];
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
    
    for (size_t Index = 0;
        Index < Min(SourceACount, DestCount);
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (size_t Index = 0;
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
    for (size_t Index = 0;
        Index < Min(SourceCount, DestCount);
        ++Index)
    {
        *Dest++ = *Source++;
    }

    *Dest++ = 0;
}

int64
StringIndexOf(size_t HaystackCount, char* Haystack, size_t NeedleCount, char* Needle, size_t LowerBound = 0)
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
StringIndexOf(char* Haystack, char* Needle, size_t LowerBound = 0)
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

void
ZeroString(size_t StringLength, char* String)
{
    for (size_t i = 0; i < StringLength; ++i)
    {
        String[i] = 0;
    }
}

inline bool32 
IsNumber(char C)
{
    return C >= '0' && C <= '9';
}

inline char
DigitToChar(uint8 D)
{
    return D % 10 + '0';
}

int32 
StringOccurrences(size_t SourceCount, char* Source, size_t SearchCount, char* Search, uint32 StartIndex = 0)
{
    int64 Index = StartIndex;
    int Occurrences = 0;
    while ((Index = StringIndexOf(SourceCount, Source, SearchCount, Search, Index+1)) >= 0)
    {   
        ++Occurrences;
    }
    
    return Occurrences;
}

int32 StringOccurrences(char* Source, char* Search, uint32 StartIndex = 0)
{
    return StringOccurrences(StringLength(Source), Source, StringLength(Search), Search);
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
ParseInteger(size_t TextLength, char* Text)
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
    for (size_t i = 0;
        i < TextLength;
        ++i)
    {
        char C = Text[i];
        if (IsNumber(C))
        {
            Result.Value += (C - 48) * Pow(10, (int32)(TextLength - i - 1));
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

#define CHR
#endif