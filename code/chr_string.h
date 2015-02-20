

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
    Result.Value = (char*)Alloc(Result.Length);
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
    int64 Index = 0;
    int Occurrences = 0;
    while ((Index = StringIndexOf(Source, Search, Index+1)) >= 0)
    {   
        ++Occurrences;
    }
    
    return Occurrences;
}

int 
StringReplace(string* Source, string Token, string Replacement)
{
    int OccurrenceCount = StringOccurrences(*Source, Token);
    if (OccurrenceCount > 0)
    {
        size_t Delta = (Replacement.Length - Token.Length) * OccurrenceCount; // Remember, might be negative
        size_t NewLength = Source->Length + Delta;

        char* NewString = (char*)Alloc(NewLength);
        size_t LastIndex = 0;
        int64 NextIndex = 0;
        size_t NewStringIndex = 0;
        while ((NextIndex = StringIndexOf(*Source, Token, (int)NextIndex + 1)) >= 0)
        {
            //Copy in from the original string, up to the next token occurrence
            CopyString(NextIndex - LastIndex, Source->Value + LastIndex, NextIndex - LastIndex, NewString + NewStringIndex);
            NewStringIndex += (NextIndex - LastIndex);

            //Copy in from the replacement instead of copying the token from the original
            CopyString(Replacement.Length, Replacement.Value, Replacement.Length, NewString + NewStringIndex);
            NewStringIndex += Replacement.Length;

            // Account for the token before moving on
            LastIndex = NextIndex + Token.Length; 
        }

        CopyString(Source->Length - LastIndex, Source->Value + LastIndex, NewLength - NewStringIndex, NewString + NewStringIndex);

        Assert(NewStringIndex + (Source->Length - LastIndex) == NewLength);
        Source->Value = NewString;
        Source->Length = NewLength;

    }

    return OccurrenceCount;
}
