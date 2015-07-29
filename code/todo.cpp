#include "chr.h"
#include "chr_string.h"
#include "chr_array.h"
#include "todo.h"


global_variable char* TodoBasename = "todo.txt";
global_variable char* DoneBasename = "done.txt";

array<void*> AllocatedStringList;

//NOTE(chronister): This function returns whether or not Filename was found
// in either pwd or user home dir. If it wasn't (returns false), the program
// should probably query if it should be created, and create it.
// Default creation folder should probably be the home dir.
internal astring
ConstructLocalFilepath(bstring Filename)
{
    if (plat::FileExists(Filename.Value))
    {
        //TODO(chronister): Do we want to expand the current directory to the full path?
        return BStringCopyToAString(Filename);
    }

    // We assume this is a well formed, existing directory
    // that looks like "C:/Users/Steve/" or "/home/steve/"
    astring UserDir = plat::GetUserDir();
    astring ConstructedPath = CatStrings(A2BSTR(UserDir), Filename);
    FreeString(&UserDir);
    return ConstructedPath;
}

internal astring
ReplaceFilenameInFilepath(bstring Filepath, bstring ReplacementName)
{
    char* OnePastLastSlash = Filepath.Value;
    for (char* Scan = Filepath.Value;
        *Scan;
        ++Scan)
    {
        if (*Scan == '/' || *Scan == '\\')
        {
            OnePastLastSlash = Scan + 1;
        }
    }
    if (*OnePastLastSlash == '\0'
        && !(*(OnePastLastSlash - 1) == '/' || *(OnePastLastSlash - 1) == '\\'))
    {
        OnePastLastSlash = Filepath.Value;
    }
    uint32 DirLen = (uint32)(OnePastLastSlash - Filepath.Value);
    astring Result;
    Result.Length = (uint32)DirLen + ReplacementName.Length;
    Result.Value = (char*)plat::Alloc(Result.Length, false);
    CatStrings(DirLen, Filepath.Value, ReplacementName.Length, ReplacementName.Value, Result.Length, Result.Value);
    return Result;
}

internal astring
GetTodoFilename()
{
    bstring TodoStr = BSTR(TodoBasename);
    return ConstructLocalFilepath(TodoStr);
}

internal astring
GetDoneFilename(astring TodoFilename)
{
    //TODO(chronister): This should be in the same place as the todo.txt given the same context
    // So it needs a way to know which todo.txt is active and use the same directory as that.
    astring DoneStr = ReplaceFilenameInFilepath(A2BSTR(TodoFilename), BSTR(DoneBasename));
    return DoneStr;
}

inline bool32
IsValidPriority(char Test)
{
    return (Test >= 'A' && Test <= 'Z');
}

//TODO(chronister): Generalize this out into a generic array sort function
//TODO(chronister): Use something other than a bubble sort?
internal void
SortTodoItemList(uint32 Length, todo_item* List, int32 (*Compare)(todo_item, todo_item))
{
    int32 Swaps = 0;
    uint32 N = Length;
    uint32 NewN = 0;
    do
    {
        NewN = 0;
        for (uint32 i = 1;
            i < N;
            ++i)
        {
            if (Compare(List[i - 1], List[i]) >= 0)
            {
                Swap(List[i], List[i - 1], todo_item);
                NewN = i;
            }
        }
        N = NewN;
    } while(N > 0);
}

int32
CompareStringValues(bstring A, bstring B)
{
    int i;
    char* String1 = A.Value;
    char* String2 = B.Value;
    for (i = 0;
        String1[i] && String2[i];
        ++i)
    {
        if (String1[i] > String2[i]) { return 1; }
        if (String1[i] < String2[i]) { return -1; }
    }
    //Check one past the last character, as well, in case they were different lengths
    if (String1[i] > String2[i]) { return 1; }
    if (String1[i] < String2[i]) { return -1; }
    return 0;
}

int32
CompareTodoItemPriority(todo_item A, todo_item B)
{
    if (A.Priority == 0 && B.Priority > 0) { return 1; }
    if (B.Priority == 0 && A.Priority > 0) { return -1; }
    if (A.Priority > B.Priority) { return 1; }
    if (A.Priority < B.Priority) { return -1; }

    if (A.Complete && !B.Complete) { return 1; }
    if (B.Complete && !A.Complete) { return -1; }

    return CompareStrings(A.Body, B.Body);
}

int32
CompareTodoItemLineNum(todo_item A, todo_item B)
{
    if (A.LineNumber < B.LineNumber) { return -1; }
    if (A.LineNumber > B.LineNumber) { return 1; }
    return 0;
}

bool32
PredicateItemComplete(todo_item* Item, void* Unused)
{
	return Item->Complete;
}

internal int32
GetNumberOfLines(plat::read_file_result File)
{
    uint32 LineCount = 0;
    char* Begin = (char*)File.Contents;
    for (uint32 i = 0;
        i < File.ContentsSize;
        ++i)
    {
        if (*(Begin + i) == '\n') { ++LineCount; }
    }
    //NOTE(chronister): The commented out condition would be if trailing newlines were not supported
    //if (*(Begin + File.ContentsSize - 1) != '\n')
    //{
    LineCount += 1;
    //}
    return LineCount;
}

bool32
IsNonTrivial(bstring String)
{
	foreach(char, C, String.Length, String.Value)
	{
		if (!(IsWhitespace(*C) || *C == '\0'))
		{
			return true;
		}
	}
	return false;
}

internal todo_item
ParseTodoLine(int32 LineNum, bstring Line)
{
    todo_item Item = {};
    Item.Priority = 0;
    Item.LineNumber = LineNum;
    Item.Body = Line;

    if (Line.Value[0] == '(' && Line.Value[2] == ')' && Line.Value[3] == ' ' && IsValidPriority(Line.Value[1]))
    {
        Item.Priority = Line.Value[1];
        Item.Body.Length -= 4;
        Item.Body.Value += 4;
    }
    if (Line.Value[0] == 'x' && Line.Value[1] == ' ')
    {
        Item.Complete = true;
        Item.Body.Length -= 2;
        Item.Body.Value += 2;
    }

    if (Item.Body.Length > 0 && Item.Body.Value[Item.Body.Length - 1] == '\r')
    {
        Item.Body.Length -= 1;
    }

    return Item;
}

internal todo_file
ParseTodoFile(plat::read_file_result File, bool32 RemoveEmptyLines = false)
{
    todo_file Todo = {0};

	uint32 MaximumItems = GetNumberOfLines(File);
	Todo.Items = AllocateArray<todo_item>(MaximumItems);

    uint32 LineNum = 0;
    char* Begin = (char*)File.Contents;
    char* Start = Begin;
	bool32 OnlyWhitespace = true;
    for (uint32 i = 0;
        i <= File.ContentsSize;
        ++i)
    {
        char* End = Begin + i;
		if (!(IsWhitespace(*End) || *End == '\0')) { OnlyWhitespace = false; }
        if (*End == '\n' || i == (File.ContentsSize))
        {
			if (!OnlyWhitespace)
			{
				Assert(LineNum < MaximumItems);

				bstring Line = {};
				Line.Length = (uint32)(End - Start);
				Line.Value = Start;

				todo_item Item = ParseTodoLine(LineNum+1, Line);
				ArrayPush<todo_item>(&Todo.Items, Item);

				OnlyWhitespace = true;
			}
            LineNum += 1;
			Start = End + 1;
        }
    }

    return Todo;
}

internal uint32
GetItemStringSize(todo_item Item)
{
    uint32 ItemLength = Item.Body.Length;

    if (Item.Complete)
    {
        ItemLength += 2;
    }
    else if (IsValidPriority(Item.Priority))
    {
        ItemLength += 4;
    }

    //NOTE(chronister): If this method ends up being used somewhere other than file serialization,
    // Make sure its still a good idea to have this here.
    ItemLength+=1; // For the newline.
    return ItemLength;
}

internal bool32
StringIsEmpty(uint32 StrLength, char* Str)
{
	for (uint32 i = 0; i < StrLength; ++i)
	{
		if (!IsWhitespace(Str[i])) { return false; }
	}
	return true;
}

internal bool32
SerializeTodoItem(todo_item Item, bstring* Buffer)
{
	//Note: Buffer is an unusual bstring in that we'll be appending straight into it. For our purposes, we'll treat it
	//as an astring, since we *know* that we'll be able to write to it.
	astring Result = BStringToAString(*Buffer);
	// Length needs to be zero for the append functions to work properly. 
	// Also, The loft method above will ensure the capacity is set correctly.
	Result.Length = 0; 

    if (StringIsEmpty((uint32)Item.Body.Length, Item.Body.Value)) { return false; }
	uint32 ItemLength = GetItemStringSize(Item);
    Assert(Result.Capacity >= ItemLength);

    if (Item.Complete)
    {
		FormatIntoString(&Result, "x %.*s\n", Item.Body.Length, Item.Body.Value);
    }
    else if (Item.Priority)
    {
		FormatIntoString(&Result, "(%c) %.*s\n", Item.Priority, Item.Body.Length, Item.Body.Value);
    }
    else
    {
		FormatIntoString(&Result, "%.*s\n", Item.Body.Length + 1, Item.Body.Value);
    }

	return true;
}

internal plat::read_file_result
SerializeTodoFile(todo_file Todo, bool32 RemoveEmptyLines)
{
    plat::read_file_result Result = {};
    if (Todo.Items.Length > 0)
    {
		ArraySortBubble<todo_item>(&Todo.Items, &CompareTodoItemLineNum);

        uint32 TotalSize = 0;
		{foreach(todo_item, Item, Todo.Items.Length, Todo.Items.Values)
        {
            TotalSize += GetItemStringSize(*Item);
        }}
		//TODO(chronister): Do we really need to null-terminate?
		TotalSize += 1; // For that sweet null-termination

        Result.ContentsSize = TotalSize;
        Result.Contents = plat::Alloc(Result.ContentsSize, false);
        char* ResultContents = (char*)Result.Contents;

        uint32 RunningSize = 0;
        foreach(todo_item, Item, Todo.Items.Length, Todo.Items.Values)
        {
			bstring Buffer;
			// We need to set the length to the size + 1 so that vsnprintf can leave room for the null terminator
			// without truncating the format. 
			Buffer.Length = GetItemStringSize(*Item) + 1;
			Buffer.Value = ResultContents + RunningSize;
            Assert((RunningSize + Buffer.Length) <= TotalSize);

            if (SerializeTodoItem(*Item, &Buffer))
			{
				RunningSize += Buffer.Length - 1;
				// We use the item size fetched above (minus the +1 added above for null terminator) because we don't need, or
				// want, items to be separated by nulls!
			}
        }

        //Eliminate trailing newlines
        while (ResultContents[Result.ContentsSize - 1] == '\n')
        {
            ResultContents[Result.ContentsSize - 1] = 0;
            Result.ContentsSize -= 1;
        }
    }

    return Result;
}

todo_file
GetTodoFile(bool32 RemoveEmptyLines = false)
{
    astring Filename = GetTodoFilename();
    plat::read_file_result Result = plat::ReadEntireFile(Filename.Value);
    todo_file Todo = {};
    if (Result.ContentsSize > 0)
    {
        Todo = ParseTodoFile(Result, RemoveEmptyLines);
    }
    Todo.Filename = Filename;
    return Todo;
}

todo_file
GetDoneFile(astring TodoFilename)
{
    astring Filename = GetDoneFilename(TodoFilename);
    plat::read_file_result Result = plat::ReadEntireFile(Filename.Value);
    todo_file Todo = {0};
    if (Result.ContentsSize > 0)
    {
		Todo = ParseTodoFile(Result, true);
    }
    Todo.Filename = Filename;
    return Todo;
}

void
FreeTodoFile(todo_file* Todo)
{
	FreeArray<todo_item>(&Todo->Items);
	FreeFile(Todo->Raw);
}

bool32
SaveTodoFile(todo_file Todo, bool32 RemoveEmptyLines = false)
{
    plat::read_file_result Serialized = SerializeTodoFile(Todo, RemoveEmptyLines);
    return plat::WriteEntireFile(Todo.Filename.Value, Serialized.ContentsSize, (char*)Serialized.Contents);
}

bool32
SaveDoneFile(todo_file Done, bool32 RemoveEmptyLines = false)
{
    plat::read_file_result Serialized = SerializeTodoFile(Done, RemoveEmptyLines);
    return plat::WriteEntireFile(Done.Filename.Value, Serialized.ContentsSize, (char*)Serialized.Contents);
}

bool32 ItemMatchesQuery(todo_item* Item, bstring* Query)
{
    return StringIndexOf(Item->Body, *Query) >= 0;
}

void
ListTodoItems(todo_file Todo, bstring* Query=0)
{
    if (Todo.Items.Length <= 0)
    {
        PrintFC("|R`No items to do!\n");
        return;
    }
    int32 MaxWidth = Log10(Todo.Items.Length) + 1;
    foreach(todo_item, Line, Todo.Items.Length, Todo.Items.Values)
    {
        if (Line->Body.Length <= 1) { continue; } // Don't display blank lines

        if (Query)
        {
            if (!ItemMatchesQuery(Line, Query))
            {
                continue;
            }
        }

        int32 LineWidth = Log10(Line->LineNumber);
        for (int i = 0;
             i < MaxWidth - LineWidth;
             ++i)
        {
            PrintFC(" ");
        }

        astring ColoredBody = BStringToAString(Line->Body);

        int NumProjects = StringOccurrences(Line->Body, BSTR("+"));
        if (NumProjects > 0)
        {
            // _rgb`  ...   ` -- 6
			// Allocates a new string with space for the line's text PLUS the characters in the color format string.
            astring Temp1 = AllocateString(Line->Body.Length + 6 * NumProjects + 1 + 1);
            CopyString(Line->Body, &Temp1);

            astring Temp2 = AllocateString(Temp1.Capacity);

            bstring KeywordColor = BSTR("|RGB`+");

            // Deal with a + in the first spot
            if (Temp1.Value[0] == '+')
            {
                StringReplace(BSTR(Temp1.Value), &Temp2,
                    BSTR("+"), KeywordColor, 0, 1);
                CopyString(A2BSTR(Temp2), &Temp1);
            }

            int LastKeychar = -1;
            int LastSpace = 0;
            for (int i = 0;
                i < NumProjects;
                ++i)
            {
                LastKeychar = StringIndexOf(A2BSTR(Temp1), BSTR(" +"), (uint32)(LastSpace));
                if (LastKeychar > 0)
                {
					StringReplace(A2BSTR(Temp1), &Temp2,
							BSTR("+"), KeywordColor, (int)LastSpace, 1);
                    CopyString(Temp2, &Temp1);

					StringReplace(A2BSTR(Temp1), &Temp2, BSTR(" "), BSTR("` "), (int)LastKeychar + 1, 1);
                    CopyString(Temp2, &Temp1);

                    LastSpace = StringIndexOf(A2BSTR(Temp1), BSTR(" "), (uint32)(LastKeychar + 1));
                    if (LastSpace < 0)
					{
						break;
                    }
                }
                else
                {
                    break;
                }
            }

            ColoredBody = Temp1;
            FreeString(&Temp2);
        }

        PrintFC("|G`%d:` ", Line->LineNumber);
        if (Line->Complete)
        {
            PrintFC("x %.*s\n", ColoredBody.Length, ColoredBody.Value);
        }
        else if (Line->Priority)
        {
			PrintFC("|RG`(%c)` %.*s\n", Line->Priority, ColoredBody.Length, ColoredBody.Value);
        }
        else
        {
            PrintFC("%.*s\n", ColoredBody.Length, ColoredBody.Value);
        }

        if (ColoredBody.Value != Line->Body.Value)
        {
            FreeString(&ColoredBody);
        }
    }
}
void
ListTodoItems(bstring* Query=0)
{
    todo_file Todo = GetTodoFile();
	ArraySortBubble(&Todo.Items, &CompareTodoItemPriority);
    ListTodoItems(Todo, Query);
    FreeTodoFile(&Todo);
}

int64
GetTodoItemIndexFromLineNumber(uint32 Number, todo_file Todo)
{
    todo_item* Result = 0;
    //TODO(chronister): Make some kind of standard, somewhat efficient
    //  array search
    for (uint32 i = 0;
		i < Todo.Items.Length;
        ++i)
    {
        if (Todo.Items.Values[i].LineNumber == Number)
        {
            return i;
        }
    }
    return -1;
}

void
AddTodoItem(todo_file* Todo, todo_item Item)
{
	//TODO(chronister): NumberOfItems is not actually related to line number. Need to find the highest line number
	//somehow.
	// (2015-07-23) in what circumstances?
    Item.LineNumber = Todo->Items.Length+1;
    ArrayPush<todo_item>(&Todo->Items, Item);

	PrintFC("Added |B`\"%s\"` to todo.txt on line |G`#%d`.\n", Item.Body.Value, Item.LineNumber);
}

void
AddTodoItem(todo_file* Todo, char* It)
{
	bstring ItemStr = BSTR(It);
	todo_item Item = ParseTodoLine(0, ItemStr);
	AddTodoItem(Todo, Item);
}

void
RemoveTodoItem(todo_file* Todo, int ItemNum)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
    if (ItemIndex < 0) 
	{ 
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return; 
	}

    RemoveItemByIndex<todo_item>(&Todo->Items, (uint32)ItemIndex);
	PrintFC("Removed item |G`#%d`.\n", ItemNum);
    return;
}

void
PrioritizeTodoItem(todo_file* Todo, int32 ItemNum, char Priority)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
	if (ItemIndex < 0)
	{
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return;
	}
    todo_item* Item = Get<todo_item>(Todo->Items, (uint32)ItemIndex);
    Item->Priority = Priority;

    if (Priority)
    {
        PrintFC("Set the priority of item |G`#%d` to |RG`(%c)`.\n", Item->LineNumber, Item->Priority);
    }
    else
    {
        PrintFC("Deprioritized item |G`#%d`.\n", Item->LineNumber, Item->Priority);
    }
    return;
}

void
SetTodoItemCompletion(todo_file* Todo, int32 ItemNum, bool32 Complete)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
	if (ItemIndex < 0)
	{
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return;
	}

    todo_item* Item = Get<todo_item>(Todo->Items, (uint32)ItemIndex);
    Item->Complete = Complete;

    PrintFC("Completed item |G`#%d`.\n", Item->LineNumber, Item->Priority);
}

void
EditTodoItem(todo_file* Todo, int32 ItemNum, bstring NewText)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
	if (ItemIndex < 0)
	{
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return;
	}

    todo_item* Item = Get<todo_item>(Todo->Items, (uint32)ItemIndex);
    Item->Body = NewText;

    PrintFC("Edited item |G`#%d`.\n", Item->LineNumber);
}

uint32
CountCompletedItems(todo_file Todo)
{
	return ArrayOccurrences<todo_item>(Todo.Items, PredicateItemComplete, (void*)(null));
}

void
ArchiveCompletedItems(todo_file* Todo, todo_file* Done)
{
	array<todo_item*> CompletedItems = ArrayFilter<todo_item>(Todo->Items, PredicateItemComplete, (void*)(null));
	ArrayPushAllReferences<todo_item>(&Done->Items, CompletedItems);
	FreeArray(&CompletedItems);
	
    if (SaveDoneFile(*Done))
    {
		PrintFC("Archived the completed items.\n");
    }
}

void
AddKeyword(todo_file* Todo, int32 ItemNum, bstring Keyword)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
	if (ItemIndex < 0)
	{
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return;
	}

    todo_item* Item = Get<todo_item>(Todo->Items, (uint32)ItemIndex);

	astring BodyBuffer = AllocateString(Item->Body.Length + 1 + Keyword.Length);
	AppendToString(&BodyBuffer, Item->Body);
	AppendToString(&BodyBuffer, BSTR(" "));
    AppendToString(&BodyBuffer, Keyword);
	Item->Body = A2BSTR(BodyBuffer);

    PrintFC("Added |---_rgb`%s` item |G`#%d`.\n", Keyword.Value, Item->LineNumber);
}

void
RemoveKeyword(todo_file* Todo, int32 ItemNum, bstring Keyword)
{
	int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, *Todo);
	if (ItemIndex < 0)
	{
		PrintFC("|R`No item at line %d!`\n", ItemNum);
		return;
	}

    todo_item* Item = Get<todo_item>(Todo->Items, (uint32)ItemIndex);

    if (*(Keyword.Value) == null) {
        PrintFC("Item #%d not updated, invalid keyword.\n", ItemNum);
        return;
    }

    if (Keyword.Length == 1)
    {
        if (*Keyword.Value == '+' || *Keyword.Value == '@')
        {
            int64 StartIndex = StringIndexOf(Item->Body.Value, Keyword.Value);
			int64 EndIndex = -1;
            if (StartIndex >= 0) { EndIndex = StringIndexOf(Item->Body.Value, " ", (uint32)StartIndex); }

            if (StartIndex >= 0 && EndIndex >= 0)
            {
                Assert(EndIndex > StartIndex);

                Keyword.Length = (uint32)(EndIndex - StartIndex);
				Keyword.Value = Item->Body.Value + StartIndex;
            }
        }
    }

    astring KeywordToken = CatStrings(BSTR(" "), Keyword);

	astring NewBody = BStringToAString(Item->Body);
    int Replacements = StringReplace(&NewBody, A2BSTR(KeywordToken), BSTR(""));
	
    astring Confirmation = FormatString("Item |G`#%d` now reads |B`%s`, is this correct?",
                                            Item->LineNumber, Item->Body.Value);
    if (!plat::ConfirmAction(Confirmation.Value)) {
        PrintFC("Removal aborted.\n");
		goto cleanup;
    }

    if (Replacements <= 0) {
        PrintFC("Couldn't find %s in item #%d.\n", Keyword.Value+1, Item->LineNumber);
		goto cleanup;
    }

	Item->Body = A2BSTR(NewBody);

    if (Replacements == 1)
    {
        PrintFC("Removed |---_rgb`%s` from item |G`#%d`.\n", Keyword.Value+1, Item->LineNumber);
    }
    else
    {
        PrintFC("Removed %d instances of _rgb`%s` from item |G`#%d`.\n", Replacements, Keyword.Value, Item->LineNumber);
    }

	cleanup:
	FreeString(&Confirmation);
	FreeString(&KeywordToken);
	return;
}

internal void
InitializeTodoFile()
{
    astring CurrentDirectory = plat::GetCurrentDirectory();

    astring TodoFilename = FormatString("%s/%s", CurrentDirectory.Value, TodoBasename);

    if (plat::FileExists(TodoFilename.Value))
    {
        PrintFC("|R`todo.txt already exists here!");
    }
    else
    {
        if (plat::WriteEntireFile(TodoFilename.Value, 0, ""))
        {
            PrintFC("|G`Created todo.txt in current directory.");
        }
        else
        {
            PrintFC("|R`Unable to create todo.txt!");
        }
    }

    FreeString(&TodoFilename);
    FreeString(&CurrentDirectory);
}

internal parse_args_result
ParseArgs(int argc, char* argv[])
{
    parse_args_result Result = {};
    int32 CommandIndex = -1;
    for (int32 i = 1;
        i < argc;
        ++i)
    {
        char* Arg = argv[i];

        if (Arg[0] == '-' || Arg[0] == '/')
        {
            if (Arg[1] == 'n') { Result.Flags |= FLAG_REMOVE_BLANK_LINES; }
            if (Arg[1] == 'h') { Result.Flags |= FLAG_HELP; }

            if (Arg[1] == '-')
            {
                char* Word = (Arg + 2);
                if (CompareStrings(Word, "help")) { Result.Flags |= FLAG_HELP; }
            }
        }
        else
        {
            // Doesn't start with " or /, so it's either a command or an arg to a command
            if (CommandIndex < 0)
            {
                if (CompareStrings(Arg, "ls")
                 || CompareStrings(Arg, "list"))
                {
                    Result.Command = CMD_LIST;
                }
                else if (CompareStrings(Arg, "a")
                      || CompareStrings(Arg, "add"))
                {
                    Result.Command = CMD_ADD;
                }
                else if (CompareStrings(Arg, "edit")
                      || CompareStrings(Arg, "replace"))
                {
                    Result.Command = CMD_EDIT;
                }
                else if (CompareStrings(Arg, "rm")
                      || CompareStrings(Arg, "del")
                      || CompareStrings(Arg, "remove"))
                {
                    Result.Command = CMD_REMOVE;
                }
                else if (CompareStrings(Arg, "p")
                      || CompareStrings(Arg, "pri")
                      || CompareStrings(Arg, "prioritize"))
                {
                    Result.Command = CMD_PRIORITIZE;
                }
                else if (CompareStrings(Arg, "dp")
                      || CompareStrings(Arg, "depri")
                      || CompareStrings(Arg, "deprioritize"))
                {
                    Result.Command = CMD_DEPRIORITIZE;
                }
                else if (CompareStrings(Arg, "do")
                      || CompareStrings(Arg, "did")
                      || CompareStrings(Arg, "complete")
                      || CompareStrings(Arg, "finish"))
                {
                    Result.Command = CMD_COMPLETE;
                }
                else if (CompareStrings(Arg, "ar")
                      || CompareStrings(Arg, "archive"))
                {
                    Result.Command = CMD_ARCHIVE;
                }
                else if (CompareStrings(Arg, "help")
                      || CompareStrings(Arg, "h"))
                {
                    Result.Command = CMD_HELP;
                }
                else if (CompareStrings(Arg, "addkeyword")
                      || CompareStrings(Arg, "addkw")
                      || CompareStrings(Arg, "akw")
                      || CompareStrings(Arg, "append")
                      || CompareStrings(Arg, "app"))
                {
                    Result.Command = CMD_ADD_KW;
                }
                else if (CompareStrings(Arg, "addproject")
                      || CompareStrings(Arg, "addproj")
                      || CompareStrings(Arg, "ap"))
                {
                    Result.Command = CMD_ADD_PROJ;
                }
                else if (CompareStrings(Arg, "addcontext")
                      || CompareStrings(Arg, "addcon")
                      || CompareStrings(Arg, "ac"))
                {
                    Result.Command = CMD_ADD_CTX;
                }
                else if (CompareStrings(Arg, "removekeyword")
                      || CompareStrings(Arg, "rmkw")
                      || CompareStrings(Arg, "rkw"))
                {
                    Result.Command = CMD_REMOVE_KW;
                }
                else if (CompareStrings(Arg, "removeproject")
                      || CompareStrings(Arg, "rmproj")
                      || CompareStrings(Arg, "rp"))
                {
                    Result.Command = CMD_REMOVE_PROJ;
                }
                else if (CompareStrings(Arg, "removecontext")
                      || CompareStrings(Arg, "rmcon")
                      || CompareStrings(Arg, "rc"))
                {
                    Result.Command = CMD_REMOVE_CTX;
                }
                else if (CompareStrings(Arg, "init")
                      || CompareStrings(Arg, "in"))
                {
                    Result.Command = CMD_INIT;
                }
                else
                {
                    Result.Command = CMD_UNKNOWN;
                }

                CommandIndex = i;
            }
            else
            {
                parse_int_result IntegerParse = ParseInteger(StringLength(Arg), Arg);
                if (IntegerParse.Valid)
                {
                    Assert(Result.NumericArgCount < 10);
                    Result.NumericArgs[Result.NumericArgCount] = (int32)IntegerParse.Value;
                    Result.NumericArgCount += 1;
                }
                else
                {
                    //TODO(chronister) Should the string args array be dynamic?
                    Assert(Result.StringArgCount < 10);
                    Result.StringArgs[Result.StringArgCount] = Arg;
                    Result.StringArgCount += 1;
                }
            }
        }
    }

    return Result;
}

void* AllocateTracked(size_t BytesToAlloc, bool32 ZeroMemory)
{
	void* Result = plat::Alloc(BytesToAlloc, ZeroMemory);
	ArrayPush<void*>(&AllocatedStringList, Result);
	return Result;
}

bool32 FreeTracked(void* Memory)
{
	RemoveItemByValue<void*>(&AllocatedStringList, Memory);
	return plat::Free(Memory);
}

internal int
RunFromArguments(parse_args_result Args)
{
	//win32::PrintMemoryUsage("START");
	ArrayAllocFunc = plat::Alloc;
	ArrayFreeFunc = plat::Free;
	AllocatedStringList = AllocateArray<void*>(1024);
	StringAllocFunc = AllocateTracked;
	StringFreeFunc = FreeTracked;

    char* Usage = "Usage: todo action [task_number] [task_description]";
    char* Help = "Usage: todo action [task_number] [task_description]\n"
                 "Keep track of items you need to accomplish, organized by +project and @context\n"
                  "\n"
                  "Actions:\n"
                  "  add|a \"Sleep on large pile of gold +DragonThings @home\"\n"
                  "  archive\n"
                  "  del|rm #[ # # ...]\n"
                  "  depri|dp #[ # # ...]\n"
                  "  do|complete #[ # # ...]\n"
                  "  help\n"
                  "  list|ls [query]\n"
                  "  pri|p # [A-Z]\n"
                  "  replace|edit # \"Sleep on even larger pile of gold +DragonThings @home\"\n";

    todo_file Todo = GetTodoFile();
	bool32 Modified = false;
	if (Args.Flags & FLAG_REMOVE_BLANK_LINES) { Modified = true; }

    switch (Args.Command)
    {
        case CMD_LIST:
        {
            if (Args.StringArgCount > 0)
            {
                bstring Query = BSTR(Args.StringArgs[0]);
                ListTodoItems(&Query);
            }
            else
            {
                ListTodoItems();
            }
        } break;

        case CMD_ADD:
        {
            foreach(char*, It, Args.StringArgCount, Args.StringArgs)
            {
                AddTodoItem(&Todo, *It);
            }
            if (Args.StringArgCount == 0)
            {
                PrintFC("Please specify a valid thing to add.\n");
            }
			else { Modified = true; }
        } break;

        case CMD_EDIT:
        {
            if (Args.NumericArgCount == 1)
            {
                if (Args.StringArgCount == 1)
                {
                    EditTodoItem(&Todo, Args.NumericArgs[0], BSTR(Args.StringArgs[0]));
					Modified = true;
                }
                else
                {
                    PrintFC("Please supply exactly one string with an edited description.\n");
                }
            }
            else
            {
                PrintFC("Please supply exactly one item number to edit.\n");
            }
        } break;

        case CMD_REMOVE:
        {
            foreach(int32, It, Args.NumericArgCount, Args.NumericArgs)
            {
                RemoveTodoItem(&Todo, *It);
				Modified = true;
            }
            if (Args.NumericArgCount == 0)
            {
                PrintFC("Please supply at least one item number to remove.\n");
            }
        } break;

        case CMD_PRIORITIZE:
        {
            if (Args.NumericArgCount == 1)
            {
                if (Args.StringArgCount == 1 && IsValidPriority(*(Args.StringArgs[0])))
                {
                    PrioritizeTodoItem(&Todo, Args.NumericArgs[0], *(Args.StringArgs[0]));
					Modified = true;
                }
                else
                {
                    PrintFC("Please supply exactly one item priority (A-Z).\n");
                }
            }
            else
            {
                PrintFC("Please supply exactly one item number to prioritize.\n");
            }
        } break;

        case CMD_DEPRIORITIZE:
        {
            foreach(int32, It, Args.NumericArgCount, Args.NumericArgs)
            {
                PrioritizeTodoItem(&Todo, *It, 0);
            }
            if (Args.NumericArgCount == 0)
            {
                PrintFC("Please supply at least one item number to deprioritize.\n");
            }
			else { Modified = true; }
        } break;

        case CMD_COMPLETE:
        {
            foreach(int32, It, Args.NumericArgCount, Args.NumericArgs)
            {
                SetTodoItemCompletion(&Todo, *It, true);
            }
            if (Args.NumericArgCount == 0)
            {
                PrintFC("Please supply at least one item number to complete.\n");
            }
			else { Modified = true; }
        } break;

        case CMD_ARCHIVE:
        {
			todo_file Done = GetDoneFile(Todo.Filename);
            ArchiveCompletedItems(&Todo, &Done);
			Modified = true;
        } break;

        case CMD_ADD_KW:
        case CMD_ADD_PROJ:
        case CMD_ADD_CTX:
        {
            if (Args.NumericArgCount >= 1 && Args.StringArgCount >= 1)
            {
                foreach (int, ItemNum, Args.NumericArgCount, Args.NumericArgs)
                {
                    foreach(char*, K, Args.StringArgCount, Args.StringArgs)
                    {
                        astring Keyword = ASTR(*K);
						bool32 KwModified = false;
                        if (Args.Command == CMD_ADD_PROJ && Keyword.Value[0] != '+')
                        {
                            Keyword = CatStrings(BSTR("+"), A2BSTR(Keyword));
							KwModified = true;
                        }
                        else if (Args.Command == CMD_ADD_CTX && Keyword.Value[0] != '@')
                        {
                            Keyword = CatStrings(BSTR("@"), A2BSTR(Keyword));
							KwModified = true;
                        }

                        AddKeyword(&Todo, *ItemNum, A2BSTR(Keyword));
						if (KwModified) FreeString(&Keyword);
                    }
                }
				Modified = true;
            }
            else
            {
                PrintFC("Please specify at least one item number and at least one thing to add.\n");
            }
        } break;

        case CMD_REMOVE_KW:
        case CMD_REMOVE_PROJ:
        case CMD_REMOVE_CTX:
        {
            if (Args.NumericArgCount >= 1)
            {
                if (Args.StringArgCount == 0)
                {
                    char K = 0;
                    if (Args.Command == CMD_REMOVE_PROJ)
                    {
                        K = '+';
                    }
                    else if (Args.Command == CMD_REMOVE_CTX)
                    {
                        K = '@';
                    }
                    RemoveKeyword(&Todo, Args.NumericArgs[0], BSTR(&K));
                }
                else
                {
                    foreach (int32, N, Args.NumericArgCount, Args.NumericArgs)
                    {
                        foreach(char*, K, Args.StringArgCount, Args.StringArgs)
                        {
                            astring Keyword = ASTR(*K);
							bool32 KwModified = false;
                            if (Args.Command == CMD_ADD_PROJ && Keyword.Value[0] != '+')
                            {
                                Keyword = CatStrings(BSTR("+"), A2BSTR(Keyword));
								KwModified = true;
                            }
                            else if (Args.Command == CMD_ADD_CTX && Keyword.Value[0] != '@')
                            {
                                Keyword = CatStrings(BSTR("@"), A2BSTR(Keyword));
								KwModified = true;
                            }

                            RemoveKeyword(&Todo, *N, A2BSTR(Keyword));
							if (KwModified) FreeString(&Keyword);
                        }
                    }
                }
				Modified = true;
            }
            else
            {
                PrintFC("Please specify exactly one item number.\n");
            }
        } break;

        case CMD_HELP:
        {
            PrintFC(Help);
        } break;

        case CMD_INIT:
        {
            InitializeTodoFile();
        } break;

        default:
        {
            if (Args.Flags & FLAG_HELP)
            {
                PrintFC(Help);
            }
            else
            {
                PrintFC("%s\n"
                      "Try todo -h for more information.\n", Usage);
            }
        } break;
    }

    if (Modified && !SaveTodoFile(Todo, Args.Flags & FLAG_REMOVE_BLANK_LINES)) {
        PrintFC("Unable to save todo file!\n");
        return 1;
    }

    //win32::PrintMemoryUsage("END");
    return 0;
}
