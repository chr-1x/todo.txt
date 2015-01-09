#include "platform_todo.h"

/* Example usage:
    t add|a "task [+category] [@context]"
    t addm "Multi line things
            add the things"
    t list|ls [search]
    t pri|p itemnumber priority
    t depri|dp itemnumber
    t replace itemnumber "taskalt"
*/

global_variable char* TodoBasename = "todo.txt";
global_variable char* DoneBasename = "done.txt";

//TODO(chronister): Some kind of context, also, we want to use user dir
internal char* 
GetTodoFilename()
{
    return TodoBasename;
}

internal char*
GetDoneFilename()
{
    //NOTE(chronister): This should be in the same place as the todo.txt given the same context
    return DoneBasename;
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
CompareTodoItemPriority(todo_item A, todo_item B)
{
    if (A.Priority == 0 && B.Priority > 0) { return 1; }
    if (B.Priority == 0 && A.Priority > 0) { return -1; }
    if (A.Priority > B.Priority) { return 1; }
    if (A.Priority < B.Priority) { return -1; }

    if (A.Complete && !B.Complete) { return 1; }  
    if (B.Complete && !A.Complete) { return -1; }  
    
    //TODO(chronister): Sort by the whole string
    if (*A.Body > *B.Body) { return 1; }
    if (*A.Body < *B.Body) { return -1; }

    return 0;
}

int32
CompareTodoItemLineNum(todo_item A, todo_item B)
{
    if (A.LineNumber < B.LineNumber) { return -1; }
    if (A.LineNumber > B.LineNumber) { return 1; }
    return 0; 
}

internal int32
GetNumberOfLines(read_file_result File)
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

internal todo_item
ParseTodoLine(int32 LineNum, size_t Length, char* Line)
{
    todo_item Item = {};
    Item.Priority = 0;
    Item.LineNumber = LineNum;
    Item.BodyLength = Length;
    Item.Body = Line;

    if (Line[0] == '(' && Line[2] == ')' && Line[3] == ' ' && IsValidPriority(Line[1]))
    {
        Item.Priority = Line[1];
        Item.BodyLength -= 4;
        Item.Body += 4;
    }
    if (Line[0] == 'x' && Line[1] == ' ')
    {
        Item.Complete = true;
        Item.BodyLength -= 2;
        Item.Body += 2;
    }

    return Item;
}

internal todo_file
ParseTodoFile(read_file_result File)
{
    todo_file Todo = {0};

    Todo.NumberOfItems = GetNumberOfLines(File);
    Todo.Items = (todo_item*)PlatformAllocMemory(sizeof(todo_item)*Todo.NumberOfItems);

    uint32 LineNum = 0;
    char* Begin = (char*)File.Contents;
    char* Start = Begin;
    for (uint32 i = 0;
        i <= File.ContentsSize;
        ++i)
    {
        char* End = Begin + i;
        if (*End == '\n' or i == (File.ContentsSize))
        {
            
            size_t Length = End - Start;
            Assert(LineNum < Todo.NumberOfItems);
            *End = '\0';

            Todo.Items[LineNum] = ParseTodoLine(LineNum+1, Length, Start);
            
            LineNum += 1;


            Start = End + 1;
        }
    }

    return Todo;
}

internal size_t 
GetItemStringSize(todo_item Item)
{
    size_t ItemLength = Item.BodyLength;

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

internal void
SerializeTodoItem(todo_item Item, size_t* ResultLength, char* Result)
{
    size_t ItemLength = GetItemStringSize(Item);
    Assert(*ResultLength >= ItemLength);

    if (Item.Complete)
    {
        CatStrings(2, "x ", Item.BodyLength, Item.Body, ItemLength-1, Result);
        CatStrings(ItemLength-1, Result, 1, "\n", ItemLength, Result);
    }
    else if (Item.Priority)
    {
        CatStrings(1, "(", 1, &Item.Priority, 2, Result);
        CatStrings(2, Result, 2, ") ", 4, Result);
        CatStrings(4, Result, Item.BodyLength, Item.Body, ItemLength-1, Result);
        CatStrings(ItemLength-1, Result, 1, "\n", ItemLength, Result);
    }
    else 
    {
        CatStrings(Item.BodyLength, Item.Body, 1, "\n", ItemLength, Result);
    }
}

internal read_file_result
SerializeTodoFile(todo_file Todo)
{
    read_file_result Result = {};
    if (Todo.NumberOfItems > 0)
    {
        SortTodoItemList(Todo.NumberOfItems, Todo.Items, &CompareTodoItemLineNum);

        size_t TotalSize = 0;
        for (uint32 i = 0;
            i < Todo.NumberOfItems;
            ++i)
        {
            todo_item Item = Todo.Items[i];
            TotalSize += GetItemStringSize(Item);
        }

        Result.ContentsSize = TotalSize;
        Result.Contents = (char*)PlatformAllocMemory(TotalSize);

        size_t RunningSize = 0;
        for (uint32 i = 0;
            i < Todo.NumberOfItems;
            ++i)
        {
            todo_item Item = Todo.Items[i];

            size_t ItemLength = GetItemStringSize(Item);
            char* Serial = (char*)PlatformAllocMemory(ItemLength);
            
            SerializeTodoItem(Item, &ItemLength, Serial);

            Assert((RunningSize + ItemLength) <= TotalSize);
            CatStrings(RunningSize, Result.Contents, ItemLength, Serial, TotalSize, Result.Contents);
            RunningSize += ItemLength;
        }

        //Eliminate trailing newlines
        if (Result.Contents[Result.ContentsSize - 1] == '\n')
        {
            Result.Contents[Result.ContentsSize - 1] = 0;
            Result.ContentsSize -= 1;
        }
    }

    return Result;
}

todo_file
GetTodoFile()
{
    char* Filename = GetTodoFilename();
    read_file_result Result = PlatformReadEntireFile(Filename);
    todo_file Todo = {};
    if (Result.ContentsSize > 0)
    {
        Todo = ParseTodoFile(Result);   
    }
    Todo.Filename = Filename;
    return Todo;
}

todo_file
GetDoneFile()
{
    char* Filename = GetDoneFilename();
    read_file_result Result = PlatformReadEntireFile(Filename);
    todo_file Todo = {0};
    if (Result.ContentsSize > 0)
    {
        Todo = ParseTodoFile(Result);   
    }
    Todo.Filename = Filename;
    return Todo;
}

bool32
SaveTodoFile(todo_file Todo)
{
    read_file_result Serialized = SerializeTodoFile(Todo);
    return PlatformWriteEntireFile(Todo.Filename, Serialized.ContentsSize, Serialized.Contents);
}

bool32
SaveDoneFile(todo_file Done)
{
    read_file_result Serialized = SerializeTodoFile(Done);
    return PlatformWriteEntireFile(Done.Filename, Serialized.ContentsSize, Serialized.Contents);
}

void
ListTodoItems(todo_file Todo)
{
    int32 MaxWidth = Log10(Todo.NumberOfItems) + 1;
    for (uint32 i = 0;
        i < Todo.NumberOfItems;
        ++i)
    {
        todo_item Line = Todo.Items[i];
        if (Line.BodyLength <= 1) { continue; } // Don't display blank lines

        int32 LineWidth = Log10(Line.LineNumber);
        for (int i = 0;
             i < MaxWidth - LineWidth;
             ++i)
        {
            print(" ");
        }

        if (Line.Complete)
        {
            print("%d: x %s\n", Line.LineNumber, Line.Body);
        }
        else if (Line.Priority) 
        {
            print("%d: (%c) %s\n", Line.LineNumber, Line.Priority, Line.Body);
        }
        else
        {
            print("%d: %s\n", Line.LineNumber, Line.Body);
        }
    }
}
void
ListTodoItems()
{
    todo_file Todo = GetTodoFile();
    SortTodoItemList(Todo.NumberOfItems, Todo.Items, &CompareTodoItemPriority);
    ListTodoItems(Todo);
}

int64
GetTodoItemIndexFromLineNumber(uint32 Number, todo_file Todo)
{
    todo_item* Result = null;
    //TODO(chronister): Make some kind of standard, somewhat efficient
    //  array search
    for (uint32 i = 0;
        i < Todo.NumberOfItems;
        ++i)
    {
        if (Todo.Items[i].LineNumber == Number)
        {
            return i;
        }
    }
    return -1;
}

void
AddTodoItem(todo_item* Item)
{
    //TODO(chronister): Implement this for later
}

void
AddTodoItem(char* Line)
{
    char* Filename = GetTodoFilename();

    int LineLength = StringLength(Line);
    char* Dest = (char*)PlatformAllocMemory(LineLength + 1);
    CatStrings(1, "\n", LineLength, Line, LineLength + 1, Dest);
    if (PlatformAppendToFile(Filename, LineLength + 1, Dest))
    {
        print("Added \"%s\" to todo.txt", Line);
    }
}

//TODO(chronister): Generalize this out into a generic array remove function
internal bool32
UnorderedRemoveTodoItemFromList(uint32* ListLength, todo_item* List, uint32 ItemIndex)
{
    for (uint32 i = 0;
        i < *ListLength;
        ++i)
    {
        if (i == ItemIndex)
        {
            List[i] = List[*ListLength - 1];
            *ListLength -= 1;
            return true;
        }
    }
    return false;
}

void
RemoveTodoItem(int32 ItemNum)
{
    todo_file Todo = GetTodoFile();

    int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, Todo);
    if (ItemIndex >= 0)
    {
        todo_item* Item = &Todo.Items[ItemIndex];
        if (UnorderedRemoveTodoItemFromList(&Todo.NumberOfItems, Todo.Items, (uint32)ItemIndex)) 
        {
            if (SaveTodoFile(Todo))
            {
                print("Removed item #%d.", Item->LineNumber);
            }
        }
        else
        {
            print("Unable to remove item #%d for some reason. Please debug.", Item->LineNumber);
        }
    }
    else
    {
        print("Unable to find item #%d.\n", ItemNum);
    }
}

void
PrioritizeTodoItem(int32 ItemNum, char Priority)
{
    todo_file Todo = GetTodoFile();

    int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, Todo);
    if (ItemIndex >= 0)
    {  
        todo_item* Item = &Todo.Items[ItemIndex];
        Item->Priority = Priority;

        if (SaveTodoFile(Todo))
        {
            if (Priority)
            {
                print("Set the priority of item #%d to %c.", Item->LineNumber, Item->Priority);
            }
            else
            {
                print("Deprioritized item #%d.", Item->LineNumber, Item->Priority);
            }
        }
    }
    else
    {
        print("Unable to find item #%d.", ItemNum);
    }
}

//TODO(chronister): This is the third time this pattern has appeared! Compress it!
void
SetTodoItemCompletion(int32 ItemNum, bool32 Complete)
{
    todo_file Todo = GetTodoFile();

    int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, Todo);
    if (ItemIndex >= 0)
    {  
        todo_item* Item = &Todo.Items[ItemIndex];
        Item->Complete = Complete;

        if (SaveTodoFile(Todo))
        {
            print("Completed item #%d.", Item->LineNumber, Item->Priority);
        }
    }
    else
    {
        print("Unable to find item #%d.", ItemNum);
    }
}

void
EditTodoItem(int32 ItemNum, size_t NewTextLength, char* NewText)
{
    todo_file Todo = GetTodoFile();

    int64 ItemIndex = GetTodoItemIndexFromLineNumber(ItemNum, Todo);
    if (ItemIndex >= 0)
    {  
        todo_item* Item = &Todo.Items[ItemIndex];
        Item->Body = NewText;
        Item->BodyLength = NewTextLength;

        if (SaveTodoFile(Todo))
        {
            print("Edited item #%d.", Item->LineNumber, Item->Body, Item->Priority);
        }
    }
    else
    {
        print("Unable to find item #%d.", ItemNum);
    }
}

uint32
CountCompletedItems(todo_file Todo)
{
    uint32 Result = 0;
    for (uint32 i = 0;
        i < Todo.NumberOfItems;
        ++i)
    {
        if (Todo.Items[i].Complete) { ++Result; }
    }
    return Result;
}

void 
ArchiveCompletedItems()
{
    todo_file Todo = GetTodoFile();
    todo_file Done = GetDoneFile();

    uint32 AllCompletedItems = Done.NumberOfItems + CountCompletedItems(Todo);
    todo_item* CompletedItemList = (todo_item*)PlatformAllocMemory(AllCompletedItems * sizeof(todo_item));

    size_t DoneItemsBytes = Done.NumberOfItems*sizeof(todo_item);

    CopyMemory((void*)CompletedItemList, (void*)Done.Items, DoneItemsBytes);

    uint32 RunningNumCompletedItems = Done.NumberOfItems;
    for (uint32 i = 0;
        i < Todo.NumberOfItems;
        ++i)
    {
        todo_item* Item = &Todo.Items[i];
        if (Item->Complete)
        {
            CompletedItemList[RunningNumCompletedItems] = Todo.Items[i];

            Todo.Items[i] = Todo.Items[Todo.NumberOfItems - 1];
            Todo.NumberOfItems -= 1;
            ++RunningNumCompletedItems;
        }
    }

    Done.Items = CompletedItemList;
    Done.NumberOfItems = AllCompletedItems;

    if (SaveDoneFile(Done))
    {
        if (SaveTodoFile(Todo))
        {
            print("Archived the completed items.");
        }
    }
}

internal int 
ParseArgs(int argc, char* argv[])
{
    foreach(argc, argv, char*, it)
    {
        print(it);
    }
}


internal int 
RunFromArguments(int argc, char* argv[])
{
    if (argc < 2) { return 0; }

    // char* call = argv[0]; // name of the program that was called. Not really important (for now)
    char* Command = argv[1]; // name of command passed
    char* CommandArg1 = "";
    char* CommandArg2 = "";
    if (argc > 2) {
        CommandArg1 = argv[2];
    } 
    if (argc > 3) {
        CommandArg2 = argv[3];
    }

    if (CompareStrings(Command, "ls") or CompareStrings(Command, "list"))
    {
        ListTodoItems();
        return 0;
    }

    if (CompareStrings(Command, "a") or CompareStrings(Command, "add"))
    {
        if (CommandArg1)
        {
            AddTodoItem(CommandArg1);
            return 0;
        }
        
        //Default:
        print("Please specify a valid thing to add.");
        return 0;
    }

    if (CompareStrings(Command, "edit") or CompareStrings(Command, "replace"))
    {
        if (CommandArg1)
        {
            integer_parse_result Parsed = ParseInteger(StringLength(CommandArg1), CommandArg1);
            if (Parsed.Valid) 
            {
                if (CommandArg2)
                {
                    EditTodoItem((int32)Parsed.Value, StringLength(CommandArg2), CommandArg2);
                    return 0;
                }
            }
        }
        
        //Default:
        print("Please specify a valid thing to edit.");
        return 0;
    }

    if (CompareStrings(Command, "rm") or CompareStrings(Command, "del") or CompareStrings(Command, "remove"))
    {
        if (CommandArg1)
        {
            integer_parse_result Parsed = ParseInteger(StringLength(CommandArg1), CommandArg1);
            if (Parsed.Valid) 
            {
                RemoveTodoItem((int32)Parsed.Value);
                return 0;
            }
        }
        
        //Default:
        print("Please specify a valid item number to remove.");
        return 0;
    }

    if (CompareStrings(Command, "p") or CompareStrings(Command, "pri") or CompareStrings(Command, "prioritize"))
    {        
        if (CommandArg1)
        {
            integer_parse_result Parsed = ParseInteger(StringLength(CommandArg1), CommandArg1);
            if (Parsed.Valid) 
            {
                if (CommandArg2 and IsValidPriority(*CommandArg2))
                {
                    PrioritizeTodoItem((int32)Parsed.Value, *CommandArg2);
                    return 0;
                }
                else
                {
                    print("Please enter a valid item priority (A-Z).");
                    return 0;
                }
            }
        }
        
        //Default:
        print("Please specify a valid item number to prioritize.");
        return 0;
    }

    if (CompareStrings(Command, "dp") or CompareStrings(Command, "depri") or CompareStrings(Command, "deprioritize"))
    {        
        if (CommandArg1)
        {
            integer_parse_result Parsed = ParseInteger(StringLength(CommandArg1), CommandArg1);
            if (Parsed.Valid) 
            {
                PrioritizeTodoItem((int32)Parsed.Value, 0);
                return 0;
            }
        }
        
        //Default:
        print("Please specify a valid item number to deprioritize.");
        return 0;
    }

    if (CompareStrings(Command, "do") or CompareStrings(Command, "did") or 
        CompareStrings(Command, "complete") or CompareStrings(Command, "finish"))
    {
        for (int i = 2;
            i < argc;
            ++i)
        {
            integer_parse_result Parsed = ParseInteger(StringLength(argv[i]), argv[i]);
            if (Parsed.Valid) 
            {
                SetTodoItemCompletion((int32)Parsed.Value, true);
                return 0;
            }
            else 
            {
                print("Couldn't figure out %s.\n", argv[i]);
            }
        }
        return 0;
    }

    if (CompareStrings(Command, "ar") or CompareStrings(Command, "archive"))
    {
        ArchiveCompletedItems();
        return 0;
    }

    print("This will be a help message, later! :D");
    return 0;
}
