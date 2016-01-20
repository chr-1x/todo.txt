#if !defined(CHR_LINUX)

#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <pwd.h>
#include <dirent.h> /* Used by FindInDirectory */
#include <errno.h> /* Checking error number on sys calls */
#include <time.h> /* Local/Universal time and timestamps */
#include <sys/time.h> /* gettimeofday and timeval struct */
#include "stdarg.h"
#include "cstdlib"

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "chr.h"
#include "chr_string.h"

#include "chr_platform.h"


namespace plat 
{
	global_variable int LogFile;

	internal void
	InitializeHandles()
	{
		char Buffer[1024] = {};
		readlink("/proc/self/exe", Buffer, 1024);
		CatStrings(StringLength(Buffer), Buffer, 4, ".log", 1023, Buffer);
		LogFile = open(Buffer, O_WRONLY|O_CREAT|O_APPEND, 0666);
		if (LogFile <= 2)
		{
			PrintFC("WARNING: Unable to open log file! Log will not be saved!\n");
		}
	}

	//TODO(chronister): Low level linux allocation?`
	internal void* 
	Alloc(size_t BytesToAlloc, bool32 ZeroTheMemory)
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
	Free(void* Memory)
	{
		free(Memory);
		return true; //TODO(chronister): Is there any way to tell if this worked?
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
	ColorPrint(uint32 Length, char* String)
	{
		//int Result = write(STDOUT_FILENO, String, Length);	
		//printf("\033[31mtest");
		write(STDOUT_FILENO, "\033[39;49m", 8);
		
		astring Dest;
		int NumCodes = StringOccurrences(Length, String, 1, "`");

		// Worst case, my control character is just ` and the ANSI code is 8 digits. So expand by 7.
		Dest.Capacity = Length + NumCodes * 7; 
		Dest.Value = (char*)Alloc(Dest.Capacity, false);

		bstring Source = BSTR(String, Length);

		int64 LastControlChar = -1;
		int64 NextControlChar = -1;
		while ((NextControlChar = StringIndexOf(Source, BSTR("`"), LastControlChar+1)) >= 0)
		{
			//LastControlChar = Max(LastControlChar, 0);
			int64 PrevFgCode = StringLastIndexOf(Source, BSTR("|"), NextControlChar);
			int64 PrevBgCode = StringLastIndexOf(Source, BSTR("_"), NextControlChar);

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
				ansiFg = 9;
				PrevFgCode = -1; // So that StartOfSeq will be computed properly
			}

			if (PrevBgCode >= 0 && PrevBgCode > LastControlChar && LastControlChar - PrevBgCode <= 7)
			{ 
				if      (StringIndexOf(4, String + PrevBgCode, 1, "R") >= 0) { ansiBg |= 0x1; BgIntense = true; }
				else if (StringIndexOf(4, String + PrevBgCode, 1, "r") >= 0) { ansiBg |= 0x1; }
				if      (StringIndexOf(4, String + PrevBgCode, 1, "G") >= 0) { ansiBg |= 0x2; BgIntense = true; }
				else if (StringIndexOf(4, String + PrevBgCode, 1, "g") >= 0) { ansiBg |= 0x2; }
				if      (StringIndexOf(4, String + PrevBgCode, 1, "B") >= 0) { ansiBg |= 0x4; BgIntense = true; }
				else if (StringIndexOf(4, String + PrevBgCode, 1, "b") >= 0) { ansiBg |= 0x4; }
			}
			else
			{
				ansiBg = 9;
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

			if (BgIntense) 	{ write(STDOUT_FILENO, ";10", 3); }
			else			{ write(STDOUT_FILENO, ";4", 2); }
			write(STDOUT_FILENO, &BgChar, 1);

			write(STDOUT_FILENO, "m", 1);
			// /write(STDOUT_FILENO, &FgChar, 1);

			LastControlChar = NextControlChar;
		}
		write(STDOUT_FILENO, String + LastControlChar + 1, Length - LastControlChar - 1);
		write(STDOUT_FILENO, "\033[39;49m", 8);

		return true; //TODO(chronister): Error checking
	}

	internal bool32
	ColorPrint(char* String)
	{
		return ColorPrint(StringLength(String), String);
	}

	internal bool32
	ColorPrintFormatted(char* FormatString, ...)
	{ 
	    uint32 Length = StringLength(FormatString);
	    uint32 FormattedLength = Max(Length*2, 256); // TODO(Chronister): This allocation
	    char* ScratchBuffer = (char*)Alloc(FormattedLength, false);

	    va_list args;
	    va_start(args, FormatString);
	    vsnprintf(ScratchBuffer, FormattedLength, FormatString, args);
	    va_end(args);

	    bool32 Result = ColorPrint(StringLength(ScratchBuffer), ScratchBuffer);
	    Free(ScratchBuffer);

	    return Result;
	}

	internal astring
	ReadLine()
	{ 
		astring Result = {};
		char Buffer[256]; //TODO(chronister): The size of this buffer

		scanf("%s", Buffer);
		Result.Length = StringLength(Buffer);
		Result.Capacity = Result.Length + 1;
		Result.Value = (char*)Alloc(Result.Capacity, false);
		CopyString(256, Buffer, Result.Length, Result.Value);

		return Result;
	}


	internal bool32
	FileExists(char* Filename)
	{
		return (open(Filename, O_RDONLY) >= 0);
	}

	
	internal astring
	GetUserDir()
	{
		char *Home = getenv("HOME");
		return CatStrings(BSTR(Home), BSTR("/"));
	}

	/*internal string
	GetUserDir()
	{
		string Result;
		struct passwd* pw = getpwuid(getuid());
		
		uint32 PathLen = StringLength(pw->pw_dir);
		Result.Length = PathLen + 1;
		Result.Capacity = Result.Length + 1;
		Result.Value = (char*)Alloc(Result.Capacity, false);
		CopyString(PathLen, pw->pw_dir, PathLen, Result.Value);
		CatStrings(PathLen, Result.Value, 1, "/", Result.Length, Result.Value);
		return Result;
	}*/

	internal void
	FreeFile(read_file_result File)
	{
		if (File.Contents) { free(File.Contents); }
		File.Contents = null;
		File.ContentsSize = 0; 
	}

	internal read_file_result
	ReadEntireFile(char* Filename)
	{
		read_file_result Result = {0};
		int fd = open(Filename, O_RDONLY|O_CREAT);
		struct stat file_info = {0};
		fstat (fd, &file_info);
		Result.ContentsSize = file_info.st_size;
		Result.Contents = (char*)Alloc(Result.ContentsSize, false);
		read(fd, Result.Contents, Result.ContentsSize);
		close(fd);
		return Result;
	}

	internal bool32
	AppendToFile(char* Filename, uint32 StringSize, char* StringToAppend)
	{
		int fd = open (Filename, O_WRONLY|O_CREAT|O_APPEND, 0666);
		int written = write(fd, StringToAppend, StringSize);
		close(fd);
		return (written == StringSize);
	}

	internal bool32
	WriteEntireFile(char* Filename, uint32 StringSize, char* StringToWrite)
	{
		int fd = open (Filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		int written = write(fd, StringToWrite, StringSize);
		close(fd);
		return (written == StringSize);
	}
	
	internal astring
	GetCurrentDirectory()
	{
		astring Result;
		Result.Capacity = 1024;
		Result.Value = (char*)Alloc(Result.Capacity, false);
		//TODO(chronister): Check if the getcwd operation fails and try again with a bigger string
		getcwd(Result.Value, Result.Capacity);
		Result.Length = StringLength(Result.Value);
		return Result;
	}

	internal bool32
	SetCurrentDirectory(char* NewDirectory)
	{
		return chdir(NewDirectory);
	}

	internal bool32
	FindInDirectory(char* Path, char* Search)
	{
		bool32 Result = false;
		DIR* Directory = opendir(Path);
		dirent* Item;
		while((Item = readdir(Directory)) != null)
		{
			if (CompareStrings(Item->d_name, Search))
			{
				Result = true;
			}
		}

		closedir(Directory);
		return Result;
	}

	internal bool32
	CreateDirectory(char* Path)
	{
		int Result = mkdir(Path, 0777);
		if (Result != 0)
		{
			LogF(LOG_ERROR, "Couldn't create directory %s (error %d)!", Path, errno);
		}
	}

	internal bool32
	PathExists(char* Path)
	{
		int fd = open(Path, O_PATH);
		if (fd >= 0) { return true; }
		return errno != ENOENT;
	}

	internal bool32
	PathIsFolder(char* Path)
	{
		struct stat Attributes;
		stat(Path, &Attributes);
		return Attributes.st_mode & S_IFDIR;
	}

	internal astring*
	ListFilesInDirectory(char* Path, int* NumberOfListedFiles)
	{
		*NumberOfListedFiles = 0;
		char* Files[1024]; //TODO(chronister): This limit
		uint32 PathLen = StringLength(Path);

		DIR* Directory = opendir(Path);
		if (Directory == null) 
		{
			// It doesn't exist. We have failed.
			LogF(LOG_ERROR, "Can't get files in directory %s.", Path);
			return null;
		}
		dirent* Item;
		while((Item = readdir(Directory)) != null)
		{
			if(Item->d_type == DT_DIR) // TODO(chronister): Doesn't handle symbolic links!
			{
				// Don't recurse
			}
			else
			{
				char FullFilename[1025];
				CatStrings(PathLen, Path, 1, "/", 1024, FullFilename);
				CatStrings(PathLen + 1, FullFilename, StringLength(Item->d_name), Item->d_name, 1024, FullFilename);

				Assert(*NumberOfListedFiles < 1023); // TODO ^
				Files[(*NumberOfListedFiles)++] = DuplicateCString(FullFilename);
			}

		}
		closedir(Directory);

		astring* Result = (astring*)Alloc(*NumberOfListedFiles * sizeof(astring), false);
		for (int i = 0; i < *NumberOfListedFiles; ++i)
		{
			Result[i] = ASTR(Files[i]);
		}

		return Result;
	}

	internal void
	RunCommand(char* CommandLine)
	{
		system(CommandLine);
	}

	#define PIPE_READ 0
	#define PIPE_WRITE 1

	internal astring
	RunCommandCaptureOutput(char* CommandLine, int CharsToCapture)
	{
		int ChildStdOutPipe[2];
		pipe(ChildStdOutPipe); //TODO(Chronister): Error checking
		
		pid_t ForkedID = fork();

		Assert(ForkedID >= 0); // TODO(chronister): Error checking 

		if (ForkedID == 0)
		{
			//Child
			dup2(ChildStdOutPipe[PIPE_WRITE], STDOUT_FILENO);
			dup2(ChildStdOutPipe[PIPE_WRITE], STDERR_FILENO);

			close(ChildStdOutPipe[PIPE_READ]);
			close(ChildStdOutPipe[PIPE_WRITE]);

			int Result = execl("/bin/sh", "sh", "-c", CommandLine, (char *) 0);
			exit(Result);
		}
		else
		{
    		close(ChildStdOutPipe[PIPE_WRITE]); 

			astring Result = {};
			Result.Capacity = CharsToCapture;
			Result.Value = (char*)Alloc(CharsToCapture, false);
			read(ChildStdOutPipe[PIPE_READ], Result.Value, Result.Capacity);

			close(ChildStdOutPipe[PIPE_READ]);
			return Result;
		}
	}

	internal char*
	GetEnvironmentVariable(char* Name)
	{
		return getenv(Name);
	}

	internal int 
	LogSimple(uint32 Length, char* String)
	{
		int written = write(LogFile, String, Length);
		return 0;
	}

	internal int 
	LogSimple(char* String)
	{
        return LogSimple(StringLength(String), String);
	}

	internal plat::time
	LinuxTimeToPlatformTime(timeval TimePrecise, bool32 IsLocal = false)
	{
		tm* DateAndTime;
	    if(IsLocal) { DateAndTime = localtime(&TimePrecise.tv_sec); }
		else { DateAndTime = gmtime(&TimePrecise.tv_sec); }

		plat::time Result = {};
		Result.Year 		= (uint16)DateAndTime->tm_year + 1900;
		Result.Month 		= (uint8)DateAndTime->tm_mon;
		Result.YearDay 		= (uint8)DateAndTime->tm_yday;
		Result.MonthDay 	= (uint8)DateAndTime->tm_mday;
		Result.WeekDay 		= (uint8)DateAndTime->tm_wday;
		Result.Hour 		= (uint8)DateAndTime->tm_hour;
		Result.Minute 		= (uint8)DateAndTime->tm_min;
		Result.Second		= (uint8)DateAndTime->tm_sec;
		Result.Millisecond 	= (uint16)(TimePrecise.tv_usec / 1000);
		Result.Microsecond 	= (uint32)(TimePrecise.tv_usec);

		return Result;
	}

	internal rawtime
	GetTimestamp()
	{
		timeval UniversalTime;
		gettimeofday(&UniversalTime, NULL);
		return (rawtime)(UniversalTime.tv_sec * 10000000 + UniversalTime.tv_usec * 10);
	}

	internal plat::time
	GetTimeUniversal(rawtime Timestamp)
	{
		timeval UniversalTime;
		UniversalTime.tv_sec = Timestamp / 10000000;
		UniversalTime.tv_usec = (Timestamp % 10000000) / 10; 
		return LinuxTimeToPlatformTime(UniversalTime, false);
	}

	internal plat::time
	GetTimeUniversal()
	{
		timeval UniversalTime;
		gettimeofday(&UniversalTime, NULL);
		return LinuxTimeToPlatformTime(UniversalTime, false);
	}

	internal plat::time
	GetTimeLocal(uint64 Timestamp)
	{
		timeval UniversalTime;
		UniversalTime.tv_sec = Timestamp / 10000000;
		UniversalTime.tv_usec = (Timestamp % 10000000) / 10; 
		return LinuxTimeToPlatformTime(UniversalTime, true);
	}

	internal plat::time
	GetTimeLocal()
	{
		timeval UniversalTime;
		gettimeofday(&UniversalTime, NULL);
		return LinuxTimeToPlatformTime(UniversalTime, true);
	}
}

int LinuxMain(int argc, char* argv[]);

#if defined(CHR_MAIN)
int main(int argc, char* argv[])
{
	plat::InitializeHandles();
    return LinuxMain(argc, argv);
}
#else

#endif



#define CHR_LINUX
#endif
