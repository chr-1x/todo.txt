#if !defined(TODO_H)

//TODO(chronister): Move this out into a standard header?
struct string
{
	uint32 Length;
	char* Value;
};

struct todo_item
{
    uint32 LineNumber;
    bool32 Complete;
    string Body;
    char Priority;
};

struct todo_file
{
    uint32 NumberOfItems;
    todo_item* Items;
    string Filename;
};

//TODO(chronister): For this purpose could this just be a string?
struct read_file_result
{
    size_t ContentsSize;
    char* Contents;
};

enum command 
{
    CMD_UNKNOWN,
    CMD_LIST,
    CMD_ADD,
    CMD_EDIT,
    CMD_REMOVE,
    CMD_PRIORITIZE,
    CMD_DEPRIORITIZE,
    CMD_COMPLETE,
    CMD_ARCHIVE,
    CMD_HELP,
    CMD_ADD_KW,
    CMD_REMOVE_KW
};
#define FLAG_HELP 0x00000001
#define FLAG_REMOVE_BLANK_LINES 0x00000010

struct parse_args_result
{
    command Command;
    uint32 Flags;

    uint32 StringArgCount;
    char* StringArgs[10];

    uint32 NumericArgCount;
    int32 NumericArgs[10];    
};


#define TODO_H
#endif