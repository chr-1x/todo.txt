#if !defined(CHR_PLATFORM)

#include "chr.h"
#include "chr_string.h"
#include "chr_printf.h"

#define PrintFC plat::ColorPrintFormatted
#define LogF plat::LogFormatted

#if !defined(LOG_LEVEL)
	#define LOG_LEVEL plat::LOG_WARNING
#endif
#if !defined(LOG_LEVEL_PRINT)
	#define LOG_LEVEL_PRINT LOG_LEVEL
#endif
#if !defined(LOG_LEVEL_WRITE)
	#define LOG_LEVEL_WRITE LOG_LEVEL
#endif

// With LogF defined, we can do a better assert!
#undef Assert
#define Assert(Statement) !(Statement) && !(LogF(plat::LOG_ERROR, "!! ASSERTION FAILED !! " #Statement " at " __FILE__ ": %d\n", __LINE__)) && ((*(int*)0) = 0)

/* 
	Struct definitions
*/
namespace plat
{
	struct time
	{
		uint32 Microsecond;
		uint16 Millisecond;
		uint16 Year;
		uint16 YearDay;		// Ordinal
		uint8  Month;		// Ordinal (1 = January, 2 = February, etc)
		uint8  MonthDay;	// Ordinal
		uint8  WeekDay;		// Ordinal (1 = Monday, 2 = Tuesday, 3 = Wednesday, etc)
		uint8  Hour;
		uint8  Minute;
		uint8  Second;
	};

	// NOTE(chronister): This uses the Windows convention of 100-nanosecond intervals.
	//	 That is, 1 unit of raw time == 100 nanoseconds.
	typedef uint64 rawtime;

	struct read_file_result
	{
		uint32 ContentsSize;
		void* Contents;
	};

	struct cstring_stack
	{
		cstring* Values;
		uint32 Count;
		uint32 Capacity;
	};

	enum log_level
	{
		LOG_DEBUG,
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR
	};

	char* LogLevelNames[4] = { "DEBUG", "INFO", "WARNING", "ERROR" };
	char* LogLevelColoredNames[4] = { "_rgb|-`DEBUG`", "|g`INFO`", "|rg`WARNING`", "|r`ERROR`" };
}

/*
	Forward declared functions that will be implemented by each platform
*/
namespace plat
{
	internal void* 
	Alloc(size_t BytesToAlloc, bool32 ZeroTheMemory = false);

	internal bool32
	Free(void* Memory);

    internal void*
    AllocateVirtualMemory(size_t BytesToAlloc);

    internal void
    FreeVirtualMemory(void* Memory);

	/* =======================
		Standard Input/Output
	   ======================= */

	internal bool32
	Print(uint32 Length, char* String);

	internal bool32
	Print(char* String);

	internal bool32
	PrintFormatted(uint32 Length, char* String);

	internal bool32
	ColorPrint(uint32 Length, char* String);

	internal bool32
	ColorPrint(char* String);

	internal bool32
	ColorPrintFormatted(char* FormatString, ...);

	internal astring
	ReadLine();

	internal int 
	LogSimple(uint32 Length, char* String);

	internal int 
	LogSimple(char* String);

	internal int	
	Log(log_level Level, char* String);

	internal int	
	LogFormatted(log_level Level, char* FormatString, ...);

	/* ==================
		   Filesystem 
	   ================== */

	internal astring
	GetCurrentDirectory();

	internal bool32
	SetCurrentDirectory(char* NewDirectory);

	internal char*
	GetFileName(uint32 PathLength, char* Path);

	internal bool32
	FindInDirectory(char* Path, char* Search);

	internal bool32
	PathExists(char* Path);

	internal bool32
	PathIsFolder(char* Path);

	internal bool32
	CreateDirectory(char* Path);

	internal astring*
	ListFilesInDirectory(char* Path, int* NumberOfListedFiles);

	internal void
	FreeFile(read_file_result File);

	internal read_file_result
	ReadFile(char* Filename, int32 BytesToRead);

	internal read_file_result
	ReadEntireFile(char* Filename);

	internal bool32
	AppendToFile(char* Filepath, uint32 StringSize, char* StringToAppend);

	bool32
	internal WriteEntireFile(char* Filepath, uint32 StringSize, char* StringToWrite);

	internal bool32
	FileExists(char* Filepath);

	internal astring
	GetUserDir();

	/* =====================
		 Command execution 
	   =====================*/

	internal void
	RunCommand(char* CommandLine);

	internal astring
	RunCommandCaptureOutput(char* CommandLine, int CharsToCapture);

	/* =====================
		  Time and Timing 
	   =====================*/

	internal plat::time
	RawTimeToPlatformTime(rawtime Time);
	
	internal rawtime 
	GetTimestamp();

	internal plat::time
	GetTimeUniversal();

	internal plat::time
	GetTimeUniversal(rawtime Timestamp);

	internal plat::time
	GetTimeLocal();

	internal plat::time
	GetTimeLocal(rawtime Timestamp);
}

/*
	Functions defined and implemented here portably.
*/
namespace plat 
{
	void
	PushOntoCStringStack(cstring_stack* Stack, char* String)
	{
		if (Stack->Values == null)
		{
			Stack->Capacity = 10;
			Stack->Values = (char**)Alloc(Stack->Capacity*sizeof(char*), false);
		}

		if (Stack->Count + 1 >= Stack->Capacity)
		{
			Stack->Capacity += 10;

			char** Old = Stack->Values;
			Stack->Values = (char**)Alloc(Stack->Capacity*sizeof(char*), false);
			uint32 BytesToCopy = Stack->Count * sizeof(char*);
			CopyMemory(Stack->Values, Old, BytesToCopy);
			Free(Old);
		}

		Stack->Values[Stack->Count++] = String;
	}

	char*
	PopOffOfCStringStack(cstring_stack* Stack)
	{
		char* Item = Stack->Values[--Stack->Count];
		Stack->Values[Stack->Count] = 0;
		return Item;
	}

	//TODO(chronister): Need some kind of program/thread context, probably
	global_variable cstring_stack GlobalDirectoryStack;

	bool32
	PushDirectory(char* NewDirectory)
	{
		astring Current = GetCurrentDirectory();
		PushOntoCStringStack(&GlobalDirectoryStack, Current.Value);
		return SetCurrentDirectory(NewDirectory);
	}

	bool32
	PopDirectory()
	{
		char* Popped = PopOffOfCStringStack(&GlobalDirectoryStack); 
		bool32 Result = SetCurrentDirectory(Popped);
		Free(Popped);
		return Result;
	}

	bool32 
	ConfirmAction(char* Prompt)
	{
		int32 Result = -1;
		astring Response;

		do {
			PrintFC("%s >> ", Prompt);
			Response = plat::ReadLine();
			LowercaseString(Response.Value);
			//Let's test a truly rediculous number of options.
			if (CompareStrings(Response.Value, "y")
			 || CompareStrings(Response.Value, "yes")
			 || CompareStrings(Response.Value, "yeah")
			 || CompareStrings(Response.Value, "ok")
			 || CompareStrings(Response.Value, "okay"))
			{
				Result = 1;
			}
			else if (CompareStrings(Response.Value, "n")
			 || CompareStrings(Response.Value, "no")
			 || CompareStrings(Response.Value, "nah")
			 || CompareStrings(Response.Value, "nope")
			 || CompareStrings(Response.Value, "quit")
			 || CompareStrings(Response.Value, "q"))
			{
				Result = 0;
			}
			else
			{
				PrintFC("Yes or No (couldn't figure out |R`\"%s\"`), please try again.\n", Response.Value);
			}
		} while(Result < 0);

		return Result > 0;
	}
	
	/* Note on time formatting:
		The time format is similar to printf, with % being used as a format specifier.
		The accepted values for format specifiers are:
		 - %[n]y   Year, where n is the number of digits to print (usually 2 or 4, 4 if unspecified).
		 - %[Ss]m  Month. If s is specified, use the shortened month name. If S, use the full month name.
		 - %d	   Day of month.
		 - %D	   Day of year.
		 - %[Ss]w  Day of week. If s is specified, use the shortened day name. If S, use the full day name. 
		 - %[s]H   Hour (24-hour time).
		 - %[s]h   Hour (12-hour time).
		 - %[s]M   Minute.
		 - %[s]s   Second.
		 - %[n]S   "Fractions" of second, where n is the number of digits to print (starting from the left). 
					 i.e., 1 for tenths, 2 for hundreths, 3 for thousandths ( == milliseconds)

		For numeric quantities, if [s] is present before the format specifier, use the word form of the number.
		The number of milliseconds is between 0 and 999.
	*/
	bool32
	IsTimeFormatSpecifier(char C)
	{
		return (C == 'y' || C == 'm' || C == 'd' || C == 'D' || C == 'w' 
			 || C == 'H' || C == 'h' || C == 'M' || C == 's' || C == 'S' 
			 || IsNumber(C));
	}

	astring 
	GetFirstNDigitsOf(int Num, uint N)
	{
		astring Result = AllocateString(N + 1);

		int Length = (int)integer::Log10(Num) + 1;
		int TargetLength = N; 
		if (Length <= TargetLength)
		{
			while (Length < TargetLength)
			{
				AppendToString(&Result, BSTR("0"));
				++Length;
			}
			AppendFormatIntoString(&Result, "%d", Num);
		}
		else
		{
			while (Log10(Num) + 1 > TargetLength)
			{
				Num /= 10;
			}	
			AppendFormatIntoString(&Result, "%d", Num);
		}
		return Result;
	}

	void
	FormatTimeInto(astring* Result, char* FormatString, plat::time Time)
	{
		uint32 FormatLength = StringLength(FormatString);
		Result->Length = 0; // We'll be overwriting it. 

		uint32 FormatCursor = 0, FormatStart = 0, FormatEnd = 0;
		bool32 InFormatSpecifier = false;
		for (FormatCursor; 
			 FormatCursor < FormatLength + 1;
			 ++FormatCursor)
		{
			if (InFormatSpecifier)
			{
				if (!IsTimeFormatSpecifier(FormatString[FormatCursor]))
				{
					InFormatSpecifier = false;
					FormatEnd = FormatCursor;

					char FormatCharacter = FormatString[FormatCursor - 1];

					switch(FormatCharacter)
					{
						case 'y':
						{
							// Year format. TODO(chronister): Digit specifier
							char DigBuf[5];
							astring Temp = ASTR(DigBuf, 5, 0);
							AppendFormatIntoString(&Temp, "%04d", Time.Year);
							AppendToString(Result, A2BSTR(Temp));

						} break;

						case 'm':
						{
							// Month format. TODO(Chronister): Long forms
							AppendFormatIntoString(Result, "%02d", Time.Month);
						} break;

						case 'd':
						{
							// Day of month format.
							AppendFormatIntoString(Result, "%02d", Time.MonthDay);
						} break;

						case 'D':
						{
							// Day of year format.
							AppendFormatIntoString(Result, "%d", Time.YearDay);
						} break;

						case 'w':
						{
							// Day of week format. TODO(chronister): Long forms
							AppendFormatIntoString(Result, "%d", Time.WeekDay);
						} break;

						case 'H':
						{
							// 24-Hour format. TODO(chronister): Long forms
							AppendFormatIntoString(Result, "%02d", Time.Hour);
						} break;

						case 'h':
						{
							// 12-hour format. TODO(chronister): Long forms
							uint8 Hour = (Time.Hour % 12);
							if (Hour == 0) { Hour = 12; }
							AppendFormatIntoString(Result, "%d", Hour);
						} break;

						case 'M':
						{
							// Minute format. TODO(chronister): Long forms
							AppendFormatIntoString(Result, "%02d", Time.Minute);
						} break;

						case 's':
						{
							// Second format. TODO(chronister): Long forms
							AppendFormatIntoString(Result, "%02d", Time.Second);
						} break;

						case 'S':
						{
							// Fraction of second format. TODO(chronister): Length specifier
							char DigBuf[4];
							astring Temp = ASTR(DigBuf, 4, 0);
							AppendFormatIntoString(&Temp, "%03d", Time.Millisecond);
							AppendToString(Result, A2BSTR(Temp));
						} break;

						case '%':
						{
							// Literal percent. Just print it.
							AppendToString(Result, BSTR("%"));
						}

						default:
						{
							// Not an accepted character. Ignore this format string. 
						} break;
					}
				}
			}
			else
			{
				if (FormatString[FormatCursor] == '%')
				{
					InFormatSpecifier = true;
					FormatStart = FormatCursor;
					if (FormatStart > 0) { 
						CopyMemory(Result->Value + Result->Length, FormatString + FormatEnd, FormatStart - FormatEnd); 
						Result->Length += (FormatStart - FormatEnd);
					}
				}
			}
		}
		AppendToString(Result, BSTR(FormatString + FormatEnd, FormatLength - FormatEnd)); 
	}

	astring
	FormatTime(char* FormatString, plat::time Time)
	{
		astring Result = AllocateString(StringLength(FormatString) + 64);
		FormatTimeInto(&Result, FormatString, Time);
		return Result;
	}

	internal int	
	Log(log_level Level, uint32 Length, char* String)
	{
		if (Level < LOG_LEVEL_WRITE && Level < LOG_LEVEL_PRINT) { return 0; }

		char TimeBuffer[256];
		astring TimeString = ASTR(TimeBuffer, 256);
		FormatTimeInto(&TimeString, "%y-%m-%d %h:%M:%s.%S", GetTimeLocal());
		
		if (Level >= LOG_LEVEL_WRITE)
		{
			LogSimple(TimeString.Length, TimeString.Value);
		   	LogSimple("\t");
			LogSimple(LogLevelNames[Level]);
		   	LogSimple("\t");
			LogSimple(Length, String);
			LogSimple("\n");
		}
		if (Level >= LOG_LEVEL_PRINT)
		{
			ColorPrint("_rgb|-`");
			ColorPrint(TimeString.Length, TimeString.Value);
			ColorPrint("`\t");
			ColorPrint(LogLevelColoredNames[Level]);
			ColorPrint("\t");
			ColorPrint(Length, String);
			ColorPrint("\n");
		}
		return 0;
	}

    internal int
    Log(log_level Level, char* String)
    {
        return Log(Level, StringLength(String), String);
    }

	int	
	LogFormatted(log_level Level, char* FormatString, ...)
	{
		va_list args;
		va_start(args, FormatString);
		astring FormattedString = FormatStringVariadic(FormatString, args);
		Log(Level, FormattedString.Length, FormattedString.Value);
		FreeString(&FormattedString);
		va_end(args);

		return 0;
	}
}

#define CHR_PLATFORM
#endif
