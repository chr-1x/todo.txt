#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "chr.h"
#include "todo.cpp"


#include "cstdlib"
//TODO(chronister): Low level linux allocation?`
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

internal bool32
PlatformFreeMemory(void* Memory)
{
	free(Memory);
	return true; //TODO(chronister): Is there any way to tell if this worked?
}

internal bool32
PlatformFileExists(char* Filename)
{
	return (open(Filename, O_RDONLY) >= 0);
}

internal string
PlatformGetUserDir()
{
	string Result;
	struct passwd* pw = getpwuid(getuid());
	
	size_t PathLen = StringLength(pw->pw_dir);
	Result.Length = PathLen + 1;
	Result.Value = (char*)PlatformAllocMemory(Result.Length);
	CopyString(PathLen, pw->pw_dir, PathLen, Result.Value);
	CatStrings(PathLen, Result.Value, 1, "/", Result.Length, Result.Value);
	return Result;
}

internal read_file_result
PlatformReadEntireFile(char* Filename)
{
	read_file_result Result = {0};
	int fd = open(Filename, O_RDONLY|O_CREAT);
	struct stat file_info = {0};
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
	return RunFromArguments(ParseArgs(argc, argv));
}