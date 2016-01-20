/*
    An experiment in creating a personal standard library.

    Created by Andrew Chronister for personal use. Mileage may vary.
    Please properly attribute, etc, etc
*/
#if !defined(CHR)

//TODO(chronister): Find reliable ways to get these without stdint.h
#include "stdint.h"

typedef int8_t int8;
typedef int8_t s8;
typedef int16_t int16;
typedef int16_t s16;
typedef int32_t int32;
typedef int32_t s32;
typedef int64_t int64;
typedef int64_t s64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint8_t u8;
typedef uint16_t uint16;
typedef uint16_t u16;
typedef uint32_t uint32;
typedef uint32_t u32;
typedef uint64_t uint64;
typedef uint64_t u64;

typedef unsigned int uint;

typedef float float32;
typedef float f32;
typedef double float64;
typedef double f64;

//TODO(chronister): Build custom output functions (For educational purposes?)
#include "stdio.h"

#define global_variable static
#define internal static
#define local_persist static

#if !defined(null)
    #define null 0
#endif

//TODO(chronister): Fully fledged assertion macro
#if INTERNAL
    #define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
	#define AssertDebug(Expression) Assert(Expression)
#else
    #define Assert(Expression) /* */
    #define AssertDebug(Expression) /* */
    //TODO(chronister): Logging these by default?
#endif


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B))
#define Swap(A, B, T) { T temp##T = A; A = B; B = temp##T; }

#define foreach(T, iter, ArrayCount, Array) T* iter = 0; if (Array) { iter = &((Array)[0]); } for (int i##__LINE__ = 0; i##__LINE__ < (int64)ArrayCount; ++i##__LINE__, iter = &((Array)[i##__LINE__]))
#define whilecount(Expression, MaxIterations) uint64 _LoopCount = 0; while((Expression) && _LoopCount++ < MaxIterations)

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#include "chr_math.h"

/* ==========================
      Memory Operations
   ========================== */

#if !defined(CopyMemory)
    void 
    CopyMemory(void* Dest, void* Source, uint32 Count)
    {
        uint8* S = (uint8*)Source;
        uint8* D = (uint8*)Dest;

        for (uint Index = 0;
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
    ZeroMemory(void* MemPtr, uint32 BytesToClear)
    {
        uint8* M = (uint8*)MemPtr;
        for (uint Index = 0;
            Index < BytesToClear;
            ++Index)
        {
            *M++ = 0;
        }
    }
#endif

bool32
CompareMemory(void* Ptr1, void* Ptr2, uint32 Length)
{
    uint8 *Block1 = (uint8*)Ptr1, *Block2 = (uint8*)Ptr2;
    for (uint32 i = 0;
        i < Length;
        ++i)
    {
        if (Block1[i] != Block2[i]) { return false; }
    }
    return true;
}

/* ==========================
      Character Operations
   ========================== */

inline bool32 
IsNumber(char C)
{
    return C >= '0' && C <= '9';
}

inline bool32
IsSimpleLetter(char C)
{
    return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
}

inline bool32
IsWhitespace(char C)
{
	return (C == ' ' || C == '\t' || C == '\n' || C == '\r' || C == '\f');
}

inline bool32
CharIsLowercase(char C)
{
	return (C >= 'a' && C <= 'z');
}

inline bool32
CharIsUppercase(char C)
{
	return (C >= 'A' && C <= 'Z');
}

inline char
CharToLower(char C)
{
    if (CharIsUppercase(C))
    {
        return (C -  'A') + 'a';
    }
    return C;
}

inline char
CharToUpper(char C)
{
    if (CharIsLowercase(C))
    {
        return (C -  'a') + 'A';
    }
    return C;
}

void
LowercaseString(char* String)
{
    for (char* C = String;
        *C != '\0';
        ++C)
    {
        *C = CharToLower(*C);
    }
}

void
UppercaseString(char* String)
{
    for (char* C = String;
        *C != '\0';
        ++C)
    {
        *C = CharToUpper(*C);
    }
}

void
UppercaseString(size_t StringLength, char* String)
{
    for (char* C = String;
        (size_t)(C - String) < StringLength;
        ++C)
    {
        *C = CharToUpper(*C);
    }
}

inline char
DigitToChar(uint8 D)
{
    return D % 10 + '0';
}

/* ==========================
      String Operations
   ========================== */

bool32
IsNotJustWhitespace(uint32 Length, char* String)
{
	foreach(char, C, Length, String)
	{
		if (!(IsWhitespace(*C) || *C == '\0'))
		{
			return true;
		}
	}
	return false;
}

bool32
CompareStrings(char* String1, char* String2)
{
    int i;
    for (i = 0;
        String1[i] && String2[i];
        ++i)
    {
        if (String1[i] != String2[i]) 
		   return false;
    }
	
	// For strings of different lengths..?
    if (String1[i] != String2[i]) 
		return false; 

    return true;
}

bool32
CompareStringsInsensitive(char* String1, char* String2)
{
    int i;
    for (i = 0;
        String1[i] && String2[i];
        ++i)
    {
        if (CharToLower(String1[i]) != CharToLower(String2[i])) { return false; }
    }
    if (CharToLower(String1[i]) != CharToLower(String2[i])) { return false; } // For strings of different lengths
    return true;
}

bool32
CompareStringSegments(uint32 CompareLength, char* String1, char* String2)
{
	for (uint i = 0;
		i < CompareLength;
		++i)
	{
		if (String1[i] != String2[i]) { return false; }
	}

	return true;
}

bool32
CompareStringSegmentsInsensitive(uint32 CompareLength, char* String1, char* String2)
{
	for (uint i = 0;
		i < CompareLength;
		++i)
	{
		if (CharToLower(String1[i]) != CharToLower(String2[i])) { return false; }
	}

	return true;
}

uint32
StringLength(char* String)
{
    uint Count = 0;
    while (*(String++))
    {
        ++Count;
    }
    return Count;
}

void
CatStrings(uint32 SourceACount, char* SourceA, uint32 SourceBCount, char* SourceB, uint32 DestCount, char* Dest)
{
    
    for (uint32 Index = 0;
        Index < Min(SourceACount, DestCount);
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (uint32 Index = 0;
        Index < Min(SourceBCount, DestCount - SourceACount);
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

uint32
CopyString(int32 SourceCount, char* Source, int32 DestCount, char* Dest, bool32 TerminateDest = true)
{
	if (SourceCount == 0 || DestCount == 0) return 0;
	int32 Write = Min(SourceCount, DestCount);
	if (TerminateDest) { --Write; }

    for (int32 Index = 0;
        Index < Write && Index >= 0; 
        ++Index)
    {
        *Dest++ = *Source++;
    }

	if (TerminateDest) { *Dest++ = 0; }
	return Write + (uint32)(TerminateDest != 0);
}
uint32
CopyString(char* Dest, char* Source, uint32 Count)
{
	return CopyString(Count, Source, Count, Dest);
}

int
StringIndexOf(uint32 HaystackCount, char* Haystack, uint32 NeedleCount, char* Needle, uint32 LowerBound = 0)
{
    uint32 NeedleIndex = 0;

    for (uint32 i = LowerBound;
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
int 
StringIndexOf(char* Haystack, char* Needle, uint32 LowerBound = 0)
{
    return StringIndexOf(StringLength(Haystack), Haystack, StringLength(Needle), Needle, LowerBound);
}

bool32
StringContains(uint32 HaystackCount, char* Haystack, uint32 NeedleCount, char* Needle)
{
	return StringIndexOf(HaystackCount, Haystack, NeedleCount, Needle) >= 0;
}

bool32
StringContains(char* Haystack, char* Needle)
{
	return StringIndexOf(Haystack, Needle) >= 0;
}

int
StringLastIndexOf(uint32 HaystackCount, char* Haystack, uint32 NeedleCount, char* Needle, int UpperBound = -1)
{
    int NeedleIndex = NeedleCount - 1;
    if (UpperBound < 0) { UpperBound = HaystackCount - 1; }

    for (int i = UpperBound;
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

int StringLastIndexOf(char* Haystack, char* Needle, int UpperBound = -1)
{
    return StringLastIndexOf(StringLength(Haystack), Haystack, StringLength(Needle), Needle, UpperBound);
}

void
ZeroString(uint32 StringLength, char* String)
{
    for (uint32 i = 0; i < StringLength; ++i)
    {
        String[i] = 0;
    }
}

uint32 
StringOccurrences(uint32 SourceCount, char* Source, uint32 SearchCount, char* Search, uint32 StartIndex = 0)
{
    int64 Index = StartIndex - 1;
    uint32 Occurrences = 0;
    while ((Index = StringIndexOf(SourceCount, Source, SearchCount, Search, (uint32)(Index+1))) >= 0)
    {   
        ++Occurrences;
    }
    
    return Occurrences;
}

uint32
StringOccurrences(char* Source, char* Search)
{
    return StringOccurrences(StringLength(Source), Source, StringLength(Search), Search);
}

int 
StringReplace(uint32 SourceCount, char* Source, 
              uint32 DestCount, char* Dest, 
              uint32 TokenCount, char* Token, 
              uint32 ReplacementCount, char* Replacement,
              int StartIndex = 0, int OccurrencesToReplace = -1)
{
    uint32 OccurrenceCount = Min(StringOccurrences(SourceCount, Source, TokenCount, Token, StartIndex), (uint32)OccurrencesToReplace);
    if (OccurrenceCount > 0)
    {
        int32 Delta = (ReplacementCount - TokenCount) * OccurrenceCount; // Remember, might be negative
        uint32 NewLength = SourceCount + Delta;

        uint32 LastIndex = StartIndex;
        int32 NextIndex = StartIndex - 1;
        uint32 NewStringIndex = StartIndex;
        while ((NextIndex = StringIndexOf(SourceCount, Source, TokenCount, Token, (int)NextIndex+1)) >= 0 && OccurrenceCount-- > 0)
        {
            //Copy in from the original string, up to the next token occurrence
            CopyString(NextIndex - LastIndex, Source + LastIndex, NextIndex - LastIndex, Dest + NewStringIndex, false);
            NewStringIndex += (NextIndex - LastIndex);

            //Copy in from the replacement instead of copying the token from the original
            CopyString(ReplacementCount, Replacement, ReplacementCount, Dest + NewStringIndex, false);
            NewStringIndex += ReplacementCount;

            // Account for the token before moving on
            LastIndex = NextIndex + TokenCount; 
        }

        CopyString(SourceCount - LastIndex, Source + LastIndex, NewLength - NewStringIndex, Dest + NewStringIndex, false);

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
ParseInteger(uint32 TextLength, char* Text)
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
    for (uint32 i = 0;
        i < TextLength;
        ++i)
    {
        char C = Text[i];
        if (IsNumber(C))
        {
            Result.Value += (C - 48) * integer::Pow(10, (int32)(TextLength - i - 1));
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
