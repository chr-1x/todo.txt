#if !defined(TODO_H)

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
    CMD_ADD_PROJ,
    CMD_ADD_CTX,
    CMD_REMOVE_KW,
    CMD_REMOVE_PROJ,
    CMD_REMOVE_CTX
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
