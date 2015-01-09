#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "chr.h"
#include "todo.cpp"


#include "cstdlib"
//TODO(chronister): Low level linux allocation?
internal void* 
PlatformAllocMemory(size_t BytesToAlloc, bool32 ZeroTheMemory)
{
	if (ZeroTheMemory)
	{
		return calloc(BytesToAlloc, 1);
	}
	else
	{
		return malloc(BytesToAlloc);
	}
}
internal void* PlatformAllocMemory(size_t BytesToAlloc) { return PlatformAllocMemory(BytesToAlloc, true); }

internal void
PlatformFreeMemory(void* Memory)
{
	free(Memory);
}

internal read_file_result
PlatformReadEntireFile(char* Filename)
{
	read_file_result Result = {};
	int fd = open(Filename, O_RDONLY);
	struct stat file_info;
	fstat (fd, &file_info);
	Result.ContentsSize = file_info.st_size;
	Result.Contents = (char*)PlatformAllocMemory(Result.ContentsSize);
	read(fd, Result.Contents, Result.ContentsSize);
	close(fd);
	return Result;
}

internal bool32
PlatformAppendToFile(char* Filename, size_t StringSize, char* StringToAppend)
{
	int fd = open (Filename, O_WRONLY|O_CREAT|O_APPEND, 0666);
	int written = write(fd, StringToAppend, StringSize);
	close(fd);
	return (written == StringSize);
}

internal bool32
PlatformWriteEntireFile(char* Filename, size_t StringSize, char* StringToWrite)
{
	int fd = open (Filename, O_WRONLY|O_CREAT, 0666);
	int written = write(fd, StringToWrite, StringSize);
	close(fd);
	return (written == StringSize);
}

int main(int argc, char* argv[])
{
	return RunFromArguments(argc, argv);
}