#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <pwd.h>

#include "chr.h"
#include "todo.cpp"


#include "cstdlib"
#include "stdarg.h"

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
	Result.Capacity = Result.Length + 1;
	Result.Value = (char*)PlatformAllocMemory(Result.Capacity);
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

/*
	On linux, we'll use the ANSI color codes. These have the format:
		\033[ff;bbm
	Where ff is a number between 30 and 37, and bb is a number between 40 and 47.
	The mapping (on the second digit) is as follows:
	 	0 --> Black		1 --> Red 		2 --> Green		3 --> Yellow
	 	4 --> Blue		5 --> Magenta 	6 --> Cyan		7 --> White
	So the mapping for my format to ANSI will look like:
	 ___ --> 0    000
	 r__ --> 1    001
	 _g_ --> 2    010
	 rg_ --> 3    011
	 __b --> 4    100
	 r_b --> 5    101
	 _gb --> 6    110
	 rgb --> 7    111
*/
internal bool32
PlatformColorPrint(size_t Length, char* String)
{
	//int Result = write(STDOUT_FILENO, String, Length);	
	//printf("\033[31mtest");
	write(STDOUT_FILENO, "\033[37m", 5);
	
	string Dest;
	int NumCodes = StringOccurrences(Length, String, 1, "`");

	// Worst case, my control character is just ` and the ANSI code is 8 digits. So expand by 7.
	Dest.Capacity = Length + NumCodes * 7; 
	Dest.Value = (char*)Alloc(Dest.Capacity);

	string Source = STR(String, Length+1, Length);

	int64 LastControlChar = -1;
	int64 NextControlChar = -1;
	while ((NextControlChar = StringIndexOf(Source, STR("`"), LastControlChar+1)) >= 0)
	{
		//LastControlChar = Max(LastControlChar, 0);
		int64 PrevFgCode = StringLastIndexOf(Source, STR("|"), NextControlChar);
		int64 PrevBgCode = StringLastIndexOf(Source, STR("_"), NextControlChar);

		uint32 ansiFg = 0;
		uint32 ansiBg = 0;
		bool32 FgIntense = false;
		bool32 BgIntense = false;

		if (PrevFgCode >= 0 && PrevFgCode > LastControlChar && NextControlChar - PrevFgCode <= 7)
		{ 
			if      (StringIndexOf(4, String + PrevFgCode, 1, "R") >= 0) { ansiFg |= 0x1; FgIntense = true; }
			else if (StringIndexOf(4, String + PrevFgCode, 1, "r") >= 0) { ansiFg |= 0x1; }
			if      (StringIndexOf(4, String + PrevFgCode, 1, "G") >= 0) { ansiFg |= 0x2; FgIntense = true; }
			else if (StringIndexOf(4, String + PrevFgCode, 1, "g") >= 0) { ansiFg |= 0x2; }
			if      (StringIndexOf(4, String + PrevFgCode, 1, "B") >= 0) { ansiFg |= 0x4; FgIntense = true; }
			else if (StringIndexOf(4, String + PrevFgCode, 1, "b") >= 0) { ansiFg |= 0x4; }
		}
		else
		{
			ansiFg = 7;
			PrevFgCode = -1; // So that StartOfSeq will be computed properly
		}

		bool32 WriteBack = false;
		if (PrevBgCode >= 0 && PrevBgCode > LastControlChar && LastControlChar - PrevBgCode <= 7)
		{ 
			WriteBack = true;
			if      (StringIndexOf(4, String + PrevBgCode, 1, "R") >= 0) { ansiBg |= 0x1; BgIntense = true; }
			else if (StringIndexOf(4, String + PrevBgCode, 1, "r") >= 0) { ansiBg |= 0x1; }
			if      (StringIndexOf(4, String + PrevBgCode, 1, "G") >= 0) { ansiBg |= 0x2; BgIntense = true; }
			else if (StringIndexOf(4, String + PrevBgCode, 1, "g") >= 0) { ansiBg |= 0x2; }
			if      (StringIndexOf(4, String + PrevBgCode, 1, "B") >= 0) { ansiBg |= 0x4; BgIntense = true; }
			else if (StringIndexOf(4, String + PrevBgCode, 1, "b") >= 0) { ansiBg |= 0x4; }
		}
		else
		{
			PrevBgCode = -1; // So that StartOfSeq will be computed properly
		}

		uint32 StartOfSeq = Min(Min(NextControlChar, (uint32)(PrevFgCode - 1)), (uint32)(PrevBgCode - 1));
		write(STDOUT_FILENO, String + LastControlChar + 1, StartOfSeq - LastControlChar - 1);

		char FgChar = DigitToChar(ansiFg);
		char BgChar = DigitToChar(ansiBg);

		write(STDOUT_FILENO, "\033[", 2);

		if (FgIntense) 	{ write(STDOUT_FILENO, "9", 1); }
		else			{ write(STDOUT_FILENO, "3", 1); }
		write(STDOUT_FILENO, &FgChar, 1);

		if (WriteBack)
		{
			if (BgIntense) 	{ write(STDOUT_FILENO, ";10", 3); }
			else			{ write(STDOUT_FILENO, ";4", 2); }
			write(STDOUT_FILENO, &BgChar, 1);
		}

		write(STDOUT_FILENO, "m", 1);
		// /write(STDOUT_FILENO, &FgChar, 1);

		LastControlChar = NextControlChar;
	}
	write(STDOUT_FILENO, String + LastControlChar + 1, Length - LastControlChar - 1);

	return true; //TODO(chronister): Error checking
}

internal bool32
PlatformColorPrintFormatted(char* FormatString, ...)
{ 
    size_t Length = StringLength(FormatString);
    size_t FormattedLength = Max(Length*2, 256); // TODO(Chronister): This allocation
    char* ScratchBuffer = (char*)PlatformAllocMemory(FormattedLength, false);

    va_list args;
    va_start(args, FormatString);
    vsnprintf(ScratchBuffer, FormattedLength, FormatString, args);
    va_end(args);

    bool32 Result = PlatformColorPrint(StringLength(ScratchBuffer), ScratchBuffer);
    PlatformFreeMemory(ScratchBuffer);

    return Result;
}

internal string
PlatformReadLine()
{ 
	string Result = {};
	char Buffer[256]; //TODO(chronister): The size of this buffer

	scanf("%s", Buffer);
	Result.Length = StringLength(Buffer) - 1;
	Result.Capacity = Result.Length + 1;
	Result.Value = (char*)Alloc(Result.Capacity);
	CopyString(256, Buffer, Result.Length, Result.Value);

	return Result;
}

int main(int argc, char* argv[])
{
	return RunFromArguments(ParseArgs(argc, argv));
}