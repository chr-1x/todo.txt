#if !defined(TODO_H)

struct todo_item
{
	uint32 LineNumber;
	bool32 Complete;
    size_t BodyLength;
    char* Body;
	char Priority;
};

struct todo_file
{
    uint32 NumberOfItems;
    todo_item* Items;
    char* Filename;
};

//TODO(chronister): Move this out into a standard header?
struct string
{
	uint32 Length;
	char* Value;
};
//TODO(chronister): For this purpose could this just be a string?
struct read_file_result
{
    size_t ContentsSize;
    char* Contents;
};


#define TODO_H
#endif