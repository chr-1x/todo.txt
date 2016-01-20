#if !defined(CHR_STRING)

//#include "chr.h"

#if !defined(CHR_H)
	#define global_variable static
	#define bool32 int
//todo...
#endif

//TODO(chronister): Making these function pointers complicates things somewhat, because they can be unset potentially.
//Need some kind of macro which every function that uses them can call to ensure that they're set, and if not, use a
//good default.
void* (*StringAllocFunc)(size_t BytesToAlloc, bool32 ZeroTheMemory);
bool32 (*StringFreeFunc)(void* Memory);

typedef char* cstring;

// allocated-strings, or strings that are operated under the assumption that they point to the start of an
// allocated block and can be reallocated at any time. 

struct astring
{
    cstring Value; // Value is expected to always point to an allocated memory block
    uint32 Length;
    uint32 Capacity; // Total capacity of the allocated block
};
global_variable astring NullAString = { null, 0, 0 };

// basic-strings, or strings that are just a char pointer and a length. They usually refer to memory
// somewhere in the middle of an allocated block or memory we don't own.

struct bstring
{
	cstring Value; // Value can point to anywhere in a block. It's up to the user to know what to free.
	uint32 Length;
};
global_variable bstring NullBString = { null, 0 };
global_variable bstring EmptyBString = { "", 0 };

// Usage note: bstring and astring should be memory-compatible for the first two struct members. To treat an astring* as
// a bstring, just cast the pointer to a bstring*. It is NOT SAFE to do this the other way around, though!

// =====================
// Conversion Functions:
// =====================

cstring
DuplicateCString(cstring String)
{
	uint32 Length = StringLength(String);
	cstring NewString = (cstring)StringAllocFunc(Length + 1, false);
	CopyString(Length, String, Length, NewString);
	NewString[Length] = 0;
	return NewString;
}

internal astring
DuplicateBString(bstring String)
{
    astring NewString = {};
    NewString.Length = NewString.Capacity = String.Length;
    NewString.Value = (char*)StringAllocFunc(NewString.Capacity, false);
    CopyString(String.Length, String.Value, NewString.Length, NewString.Value);
    return NewString;
}

internal astring
DuplicateAString(astring String)
{
    astring NewString = {};
    NewString.Length = NewString.Capacity = String.Length;
    NewString.Value = (char*)StringAllocFunc(NewString.Capacity, false);
    CopyString(String.Length, String.Value, NewString.Length, NewString.Value, false);
    return NewString;
}

#define A2BSTR AStringToBString

// See the usage note above if you are dealing with astring and bstring pointers.
inline bstring
AStringToBString(astring AStr) 
{
	bstring Result = { AStr.Value, AStr.Length };
	return Result;
}

inline astring
BStringToAString(bstring BStr)
{
	// It's not reliable to subscript past the end of the string (it might be protected memory) to check if its null
	// terminated, so we have to assume it's not. The AStr will thus have the same "capacity" as "length"
	// TODO(chronister): MOVE EVERYTHING OFF OF NULL TERMINATION (AStrs and BStrs anyway)

	astring Result = {};
	Result.Value = BStr.Value;
	Result.Length = Result.Capacity = BStr.Length;
	return Result;
}

inline astring
BStringCopyToAString(bstring BStr)
{
	astring Result;
	Result.Length = BStr.Length;
	Result.Capacity = Result.Length;
   	Result.Value = (cstring)StringAllocFunc(BStr.Length, false);
	CopyMemory(Result.Value, BStr.Value, Result.Capacity);
	Result.Value[BStr.Length] = '\0';
	return Result;
}

#define ASTR CStringToAString 

inline astring 
CStringToAString(cstring CString, uint32 Capacity, uint32 Length)
{
    astring Result;
    Result.Length = Length;
    Result.Capacity = Capacity;
    Result.Value = CString;
    return Result;
}

inline astring 
CStringToAString(cstring CString, uint32 Capacity)
{
	uint32 Length = StringLength(CString);
    return CStringToAString(CString, Capacity, Min(Length, Capacity));
}

inline astring
CStringToAString(cstring CString)
{
    uint32 Length = StringLength(CString);
    return CStringToAString(CString, Length + 1, Length);
}

cstring 
BStringToCString(bstring Str)
{
	cstring Result = (char*)StringAllocFunc(Str.Length + 1, false);
	CopyMemory(Result, Str.Value, Str.Length);	
	Result[Str.Length] = '\0';
	return Result;
}

#define BSTR CStringToBString

inline bstring
CStringToBString(cstring Begin, uint32 Length)
{
	bstring Result;
	Result.Value = Begin;
	Result.Length = Length;
	return Result;
}

inline bstring
CStringToBString(cstring Begin)
{
	return CStringToBString(Begin, StringLength(Begin));
}

// ==========================
// various utility functions
// ==========================

astring
AllocateString(uint32 Capacity, bool32 ZeroTheString = false)
{
    astring Result;
    Result.Capacity = Capacity;
    Result.Value = (cstring)StringAllocFunc(Result.Capacity, ZeroTheString);
    Result.Length = 0;
    return Result;
}

void
ExpandString(astring* String, uint32 NewCapacity)
{
    cstring NewValue = (cstring)StringAllocFunc(NewCapacity, false);
    
    CopyString(String->Capacity, String->Value, NewCapacity, NewValue, false);
    String->Capacity = NewCapacity;

    StringFreeFunc(String->Value);
    String->Value = NewValue;
}

void
AppendToString(astring* Main, bstring Addendum)
{
	Assert(Main->Length <= Main->Capacity);
	int MainSpace = Main->Capacity - Main->Length;
	if (MainSpace < 0) { MainSpace = 0; }
    CopyString(Addendum.Length, Addendum.Value, MainSpace, Main->Value + Main->Length, false);
    Main->Length += Addendum.Length;
}

void
AppendToString(astring* Main, char* Addendum)
{
    AppendToString(Main, BSTR(Addendum));
}

void
AppendFullToString(astring* Main, bstring Addendum)
{
    if (Main->Length + Addendum.Length >= Main->Capacity)
    {
        cstring MainOld = Main->Value;
        Main->Capacity = Max(Main->Length + Addendum.Length + 1, Main->Length * 2);
        Main->Value = (cstring)StringAllocFunc(Main->Capacity, false);
        CopyString(Main->Length, MainOld, Main->Length, Main->Value, false);
		StringFreeFunc(MainOld);
    }
    AppendToString(Main, Addendum);
}

void
CatStrings(bstring String1, bstring String2, bstring* Dest)
{
    CatStrings(String1.Length, String1.Value, String2.Length, String2.Value, Dest->Length, Dest->Value);
}

astring 
CatStrings(bstring String1, bstring String2)
{
    astring Result;
    Result.Length = String1.Length + String2.Length;
    Result.Capacity = Result.Length;
    Result.Value = (cstring)StringAllocFunc(Result.Capacity, false);
    CatStrings(String1.Length, String1.Value, String2.Length, String2.Value, Result.Capacity, Result.Value);
    return Result;
}

void 
CopyString(bstring Source, astring* Dest)
{
    Assert(Dest->Capacity >= Source.Length);
    Assert(Dest->Capacity > Source.Length);
    CopyString(Source.Length, Source.Value, Dest->Capacity-1, Dest->Value, false);
    Dest->Length = Source.Length;
}

void 
CopyString(astring Source, astring* Dest)
{
    Assert(Dest->Capacity >= Source.Capacity);
    Assert(Dest->Capacity > Source.Length);
    CopyString(Source.Capacity-1, Source.Value, Dest->Capacity-1, Dest->Value);
    Dest->Length = Source.Length;
}

astring
CopyString(bstring Source)
{
    astring Dest;
    Dest.Length = Source.Length;
    Dest.Capacity = Source.Length + 1;
    Dest.Value = (cstring)StringAllocFunc(Dest.Capacity, false);
    CopyString(Source.Length, Source.Value, Dest.Capacity, Dest.Value);
    return Dest;
}

bool32
CompareStrings(bstring Str1, bstring Str2)
{
	if (Str1.Length != Str2.Length) { return false; }
	return CompareStringSegments(Str1.Length, Str1.Value, Str2.Value);
}

bool32
CompareStringsInsensitive(bstring Str1, bstring Str2)
{
	if (Str1.Length != Str2.Length) { return false; }
	return CompareStringSegmentsInsensitive(Str1.Length, Str1.Value, Str2.Value);
}

void
ZeroString(bstring* Str)
{
	ZeroString(Str->Length, Str->Value);
	Str->Length = 0;
}

void
FreeString(astring* Str)
{
    StringFreeFunc(Str->Value);
    Str->Length = 0;
    Str->Capacity = 0;
    Str->Value = 0;
}

int 
StringIndexOf(bstring Haystack, bstring Needle, uint32 LowerBound = 0)
{
    return StringIndexOf(Haystack.Length, Haystack.Value, Needle.Length, Needle.Value, LowerBound);
}

int 
StringLastIndexOf(bstring Haystack, bstring Needle, int UpperBound = -1)
{
    return StringLastIndexOf(Haystack.Length, Haystack.Value, Needle.Length, Needle.Value, UpperBound);
}

uint32 
StringOccurrences(bstring Source, bstring Search, uint32 StartIndex = 0)
{
    return StringOccurrences(Source.Length, Source.Value, Search.Length, Search.Value, StartIndex);
}

uint32
StringReplace(bstring Source, astring* Dest, bstring Token, bstring Replacement, int StartIndex = 0, int OccurrencesToReplace = -1)
{
    uint32 OccurrenceCount = Min(StringOccurrences(Source, Token, StartIndex), (uint32)OccurrencesToReplace);
    if (OccurrenceCount > 0)
    {
        int Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        uint32 NewLength = Source.Length + Delta;

        Assert(Dest->Capacity > NewLength);

        StringReplace(Source.Length, Source.Value,
                      Dest->Length, Dest->Value,
                      Token.Length, Token.Value,
                      Replacement.Length, Replacement.Value
                      ,StartIndex, OccurrencesToReplace);

        Dest->Length = NewLength;
    }
	else { 
        Assert(Dest->Capacity > Source.Length);
		CopyMemory(Dest->Value, Source.Value, Source.Length + 1);
		Dest->Length = Source.Length;
	}
    return OccurrenceCount;
}

uint32 
StringReplace(astring* Source, bstring Token, bstring Replacement, 
            int StartIndex = 0, int OccurrencesToReplace = -1)
{
	bstring* SourceAsBstr = (bstring*)Source;
    uint32 OccurrenceCount = Min(StringOccurrences(*SourceAsBstr, Token, StartIndex), (uint32)OccurrencesToReplace);
    if (OccurrenceCount > 0)
    {
        int Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        uint32 NewLength = Source->Length + Delta;

        uint32 DestLength = 0;
        uint32 DestCapacity = NewLength + 1;
        cstring DestValue = (cstring)StringAllocFunc(DestCapacity, false);

        astring Dest = ASTR(DestValue, DestCapacity, DestLength);
        StringReplace(*SourceAsBstr, &Dest,
                      Token, Replacement, 
                      StartIndex, OccurrencesToReplace);
		*Source = Dest;
    }
	else
	{
		//People expect this function to have allocated new memory.
		*Source = DuplicateAString(*Source);
	}

    return OccurrenceCount;
}

uint32 
StringReplaceInPlace(astring* Source, astring Scratch, bstring Token, bstring Replacement, 
            int StartIndex = 0, int OccurrencesToReplace = -1)
{
	bstring* SourceAsBstr = (bstring*)Source;
    uint32 OccurrenceCount = Min(StringOccurrences(*SourceAsBstr, Token, StartIndex), (uint32)OccurrencesToReplace);
    if (OccurrenceCount > 0)
    {
        int Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        uint32 NewLength = Source->Length + Delta;

        uint32 DestLength = 0;
        uint32 DestCapacity = NewLength + 1;
        cstring DestValue = (cstring)StringAllocFunc(DestCapacity, false);

        astring Dest = ASTR(DestValue, DestCapacity, DestLength);
        StringReplace(*SourceAsBstr, &Dest,
                      Token, Replacement, 
                      StartIndex, OccurrencesToReplace);
		*Source = Dest;
    }
	else
	{
		//People expect this function to have allocated new memory.
		*Source = DuplicateAString(*Source);
	}

    return OccurrenceCount;
}

#define BoundedIndex(String, Index) \
	/* Normalize the indexes to positive values in the range */ \
	if (Index < 0) { Index = String.Length + Index; } \
	/* Above check caught valid negatives, this bounds it to the length of the array. */ \
	if (Index < 0) { Index = 0; } \
	if (Index >= (int)String.Length) { Index = String.Length - 1; }

bstring
StringSlice(bstring Original, int FirstIndex, int LastIndex)
{
	// Bound the indexes and translate from negative indices to normal indices in the usual way
	BoundedIndex(Original, FirstIndex);
	BoundedIndex(Original, LastIndex);
	if (FirstIndex >= LastIndex) { return EmptyBString; }
	bstring Result;
	Result.Length = LastIndex - FirstIndex;
	Result.Value = Original.Value + FirstIndex;
	return Result;
}

internal void
LeftTrimString(bstring* String)
{
	if (String->Length == 0) { return; }
	int FirstNonWhitespace = -1;
	for (uint i = 0;
		i < String->Length;
		++i)
	{
		if (!IsWhitespace(String->Value[i]))
		{
			FirstNonWhitespace = i;
			break;
		}
	}

	if (FirstNonWhitespace > 0)
	{
		CopyString(String->Value, 
				String->Value + FirstNonWhitespace, 
				String->Length - FirstNonWhitespace);
		String->Length -= FirstNonWhitespace;
	}
}

internal void
RightTrimString(bstring* String)
{
	if (String->Length == 0) { return; }
	int LastNonWhitespace = -1;
	int LastNonZero = -1;
	
	for(uint i = String->Length - 1;
		i >= 0;
		--i)
	{
		if (!IsWhitespace(String->Value[i]))
		{
			LastNonWhitespace = i;
			break;
		}
		if (LastNonZero < 0 && String->Value[i] != '\0')
		{
			LastNonZero = i;
		}
	}

	if (LastNonWhitespace >= 0)
	{
		++LastNonWhitespace; // First of the ending whitespace
		if (LastNonZero < 0) LastNonZero = String->Length;
		ZeroMemory(String->Value + LastNonWhitespace, (LastNonZero - LastNonWhitespace));
		String->Length = (uint32)LastNonWhitespace;
	}
}

internal void
TrimString(bstring* String)
{
	RightTrimString(String);
	LeftTrimString(String);
}

#define CHR_STRING
#endif
