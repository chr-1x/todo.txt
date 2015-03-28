

void* Alloc(size_t BytesToAlloc, bool32 ZeroTheMemory=false);
bool32 Free(void* Memory);

struct string
{
    size_t Length;
    char* Value;
};

string 
CatStrings(string String1, string String2)
{
    string Result;
    Result.Length = String1.Length + String2.Length;
    Result.Value = (char*)Alloc(Result.Length + 1);
    CatStrings(String1.Length, String1.Value, String2.Length, String2.Value, Result.Length, Result.Value);
    return Result;
}

string
CopyString(string Source)
{
    string Dest;
    Dest.Length = Source.Length;
    Dest.Value = (char*)Alloc(Dest.Length);
    CopyString(Source.Length, Source.Value, Dest.Length, Dest.Value);
    return Dest;
}

#define STR LoftCString
//NOTE(chronister): As the name implies, this assumes your string is null-terminated!
string
LoftCString(char* CString)
{
    string Result;
    Result.Length = StringLength(CString);
    Result.Value = CString;
    return Result;
}

void
FreeString(string* Str)
{
    Free(Str->Value);
    Str->Length = 0;
    Str->Value = 0;
}


int64 
StringIndexOf(string Haystack, string Needle, int64 StartIndex = 0)
{
    return StringIndexOf(Haystack.Length, Haystack.Value, Needle.Length, Needle.Value, StartIndex);
}

int 
StringOccurrences(string Source, string Search)
{
    return StringOccurrences(Source.Length, Source.Value, Search.Length, Search.Value);
}

int 
StringReplace(string* Source, string Token, string Replacement)
{
    int OccurrenceCount = StringOccurrences(*Source, Token);
    if (OccurrenceCount > 0)
    {
        size_t Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        size_t NewLength = Source->Length + Delta;

        size_t DestLength = 0;
        char* DestValue = (char*)Alloc(NewLength);

        StringReplace(Source->Length, Source->Value,
                      DestLength, DestValue,
                      Token.Length, Token.Value,
                      Replacement.Length, Replacement.Value);
    }

    return OccurrenceCount;
}
