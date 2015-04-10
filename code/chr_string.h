

void* Alloc(size_t BytesToAlloc, bool32 ZeroTheMemory=false);
bool32 Free(void* Memory);

struct string
{
    size_t Length;
    size_t Capacity;
    char* Value;
};

string 
CatStrings(string String1, string String2)
{
    string Result;
    Result.Length = String1.Length + String2.Length;
    Result.Capacity = Result.Length + 1;
    Result.Value = (char*)Alloc(Result.Capacity);
    CatStrings(String1.Length, String1.Value, String2.Length, String2.Value, Result.Length, Result.Value);
    return Result;
}

void 
CopyString(string Source, string* Dest)
{
    Assert(Dest->Capacity >= Source.Capacity);
    Assert(Dest->Capacity > Source.Length);
    CopyString(Source.Capacity-1, Source.Value, Dest->Capacity-1, Dest->Value);
    Dest->Length = Source.Length;
}

string
CopyString(string Source)
{
    string Dest;
    Dest.Length = Source.Length;
    Dest.Capacity = Source.Length + 1;
    Dest.Value = (char*)Alloc(Dest.Capacity);
    CopyString(Source.Length, Source.Value, Dest.Capacity, Dest.Value);
    return Dest;
}

#define STR LoftCString

//NOTE(chronister): As the name implies, this assumes your string is null-terminated!
inline string 
LoftCString(char* CString, size_t Capacity, size_t Length)
{
    string Result;
    Result.Length = Length;
    Result.Capacity = Capacity;
    Result.Value = CString;
    return Result;
}

inline string 
LoftCString(char* CString, size_t Capacity)
{
    return LoftCString(CString, Capacity, StringLength(CString));
}

inline string
LoftCString(char* CString)
{
    size_t Length = StringLength(CString);
    return LoftCString(CString, Length + 1, Length);
}

void
FreeString(string* Str)
{
    Free(Str->Value);
    Str->Length = 0;
    Str->Capacity = 0;
    Str->Value = 0;
}


int64 
StringIndexOf(string Haystack, string Needle, int64 LowerBound = 0)
{
    return StringIndexOf(Haystack.Length, Haystack.Value, Needle.Length, Needle.Value, LowerBound);
}

int64 
StringLastIndexOf(string Haystack, string Needle, int64 UpperBound = 0)
{
    return StringLastIndexOf(Haystack.Length, Haystack.Value, Needle.Length, Needle.Value, UpperBound);
}

int 
StringOccurrences(string Source, string Search, uint32 StartIndex = 0)
{
    return StringOccurrences(Source.Length, Source.Value, Search.Length, Search.Value, StartIndex);
}

int StringReplace(string Source, string* Dest, string Token, string Replacement, int StartIndex, int OccurrencesToReplace = -1){
    int OccurrenceCount = Min(StringOccurrences(Source, Token, StartIndex), OccurrencesToReplace);
    if (OccurrenceCount > 0)
    {
        size_t Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        size_t NewLength = Source.Length + Delta;

        Assert(Dest->Capacity > NewLength);

        size_t LastIndex = StartIndex;
        int64 NextIndex = StartIndex - 1;
        size_t NewStringIndex = StartIndex;
        while ((NextIndex = StringIndexOf(Source.Length, Source.Value, Token.Length, Token.Value, (int)NextIndex+1)) >= 0 && OccurrencesToReplace-- > 0)
        {
            //Copy in from the original string, up to the next token occurrence
            CopyString(NextIndex - LastIndex, Source.Value + LastIndex, NextIndex - LastIndex, Dest->Value + NewStringIndex);
            NewStringIndex += (NextIndex - LastIndex);

            //Copy in from the replacement instead of copying the token from the original
            CopyString(Replacement.Length, Replacement.Value, Replacement.Length, Dest->Value + NewStringIndex);
            NewStringIndex += Replacement.Length;

            // Account for the token before moving on
            LastIndex = NextIndex + Token.Length; 
        }

        CopyString(Source.Length - LastIndex, Source.Value + LastIndex, NewLength - NewStringIndex, Dest->Value + NewStringIndex);

        Assert(NewStringIndex + (Source.Length - LastIndex) == NewLength);
        Dest->Length = NewLength;
    }

    return OccurrenceCount;
}


int 
StringReplace(string* Source, string Token, string Replacement, 
            int StartIndex = 0, int OccurrencesToReplace = -1)
{
    int OccurrenceCount = StringOccurrences(*Source, Token);
    if (OccurrenceCount > 0)
    {
        int64 Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        size_t NewLength = Source->Length + Delta;

        size_t DestLength = 0;
        size_t DestCapacity = NewLength + 1;
        char* DestValue = (char*)Alloc(DestCapacity);

        string Dest = STR(DestValue, DestCapacity, DestLength);
        StringReplace(*Source, &Dest,
                      Token, Replacement, 
                      StartIndex, OccurrencesToReplace);
        Source->Value = DestValue;
        Source->Capacity = DestCapacity;
        Source->Length = DestLength;
    }

    return OccurrenceCount;
}