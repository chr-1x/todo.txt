#if !defined(CHR_WINPLATFORM)

#include "windows.h"
#include "shlobj.h" // For user profile

#include "chr_winutils.h"
#include "chr_string.h"
#include "chr_array.h"

#include "chr_platform.h"

// Need non-namespaced versions for chr_string and such.


namespace plat 
{
	global_variable HANDLE MainHeap;
	global_variable HANDLE ConsoleOut;
	global_variable HANDLE ConsoleIn;
	global_variable HANDLE ConsoleError;
	global_variable HANDLE LogFile;

	void 
	InitializeHandles()
	{
		plat::MainHeap = GetProcessHeap();
		plat::ConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
		plat::ConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
		plat::ConsoleError = GetStdHandle(STD_ERROR_HANDLE);

		char Buffer[1024] = {};
		GetModuleFileName(GetModuleHandle(NULL), Buffer, 1023);
		//char* ExeFileName = GetFileName(1023, Buffer);
		//uint32 PathEnd = (uint32)((ExeFileName - Buffer));
		int Offset = 0;
		int64 DotExe = StringLastIndexOf(StringLength(Buffer), Buffer, 4, ".exe");
		if (DotExe > 0)
		{
			Offset = -4;
		}
		CatStrings(StringLength(Buffer) + Offset, Buffer, 4, ".log", 1023, Buffer);
		plat::LogFile = CreateFile(Buffer, FILE_APPEND_DATA, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	}

	void
	Initialize()
	{
		InitializeHandles();
		::StringAllocFunc = Alloc;
		::StringFreeFunc = Free;
        
		::ArrayAllocFunc = Alloc;
		::ArrayFreeFunc = Free;
	}

	/* ===============
		   Memory
	   =============== */

	void* 
	Alloc(size_t BytesToAlloc, bool32 ZeroTheMemory)
	{
		void* Result = HeapAlloc(MainHeap, ZeroTheMemory ? HEAP_ZERO_MEMORY : 0, BytesToAlloc);
		win32::PrintDebug("ALLOC\t%p\t%d\n", Result, BytesToAlloc);
		//memset(Result, 0xBC, BytesToAlloc);
		return Result;
	}

	bool32
	Free(void* Memory)
	{
		bool32 Result = false;
		//win32::PrintDebug("FREE\t%p\n", Memory);
		if (Memory) {
			Result = HeapFree(MainHeap, 0, Memory);
		}
		return Result;
	}

    void*
    AllocateVirtualMemory(size_t BytesToAlloc)
    {
        return VirtualAlloc(0, BytesToAlloc, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);   
    }

    void
    FreeVirtualMemory(void* Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
	/* =======================
		Standard Input/Output
	   ======================= */

	bool32
	Print(uint32 Length, char* String)
	{
		bool32 Result;
		uint32 CharsWritten;
		uint32 CharsToWrite;

		CharsToWrite = (uint32)Length;
		SetConsoleTextAttribute(ConsoleOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
		Result &= WriteConsole(ConsoleOut, String, (uint32)CharsToWrite, (LPDWORD)&CharsWritten, NULL);
		Result &= (CharsWritten == CharsToWrite);

		return Result;
	}

	bool32
	PrintFormatted(char* FormatString, ...)
	{
		
		va_list args;
		va_start(args, FormatString);

		uint32 FormattedLength = _vscprintf(FormatString, args);
		char* ScratchBuffer = (char*)Alloc(FormattedLength + 1, false);
		vsprintf_s(ScratchBuffer, FormattedLength + 1, FormatString, args);
		
		va_end(args);

		bool32 Result = Print(FormattedLength + 1, ScratchBuffer);
		Free(ScratchBuffer);

		return Result;
	}

	int16
	WinColorFromCharacter(char Character, bool32 Foreground)
	{
		switch (Character)
		{
			case 'r':
				return Foreground ? FOREGROUND_RED : BACKGROUND_RED;
			case 'R':
				return Foreground ? FOREGROUND_RED | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_INTENSITY;
			case 'g':
				return Foreground ? FOREGROUND_GREEN : BACKGROUND_GREEN;
			case 'G':
				return Foreground ? FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_INTENSITY;\
			case 'b':
				return Foreground ? FOREGROUND_BLUE : BACKGROUND_BLUE;
			case 'B':
				return Foreground ? FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_BLUE | BACKGROUND_INTENSITY;
			default:
				return 0;
		}
	}

	bool32
	ColorPrint(uint32 Length, char* String)
	{
		bool32 Result;
		int16 WinColor;
		uint32 CharsWritten;
		uint32 CharsToWrite;

		int32 i = 0;
        char* S = String;
		for (char* C = String; *C != '\0' && i < (int32)Length; ++C, ++i)
		{
			if (*C == '`')
			{
				char ColorString[9] = {};
				char* StringEnd = Max(C - 8, S);
				CopyString((int)(C - StringEnd), StringEnd, 8, ColorString, false);

				WinColor = 0;
				int32 CharsToSkip = 0;

				int32 LastForeground = (int32)StringLastIndexOf(ColorString, "|");
				if (LastForeground >= 0)
				{   
					for (char* Col = ColorString + LastForeground; *Col != '\0' && *Col != '_'; ++Col)
					{
						WinColor |= WinColorFromCharacter(*Col, true);
					}
				}

				int32 LastBackground = (int32)StringLastIndexOf(ColorString, "_");
				if (LastBackground >= 0)
				{
					for (char* Col = ColorString + LastBackground; *Col != '\0' && *Col != '|'; ++Col)
					{
						WinColor |= WinColorFromCharacter(*Col, false);
					}
				}

				if (LastBackground >= 0 || LastForeground >= 0)
				{
					CharsToSkip = (int32)(C - StringEnd + 1) - (int32)Min((uint32)LastBackground, (uint32)LastForeground);
				}
				else
				{
					WinColor = FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
				}

				CharsToWrite = (uint32)Max(((C - S)) - (int64)CharsToSkip, 0);
				Result &= WriteConsole(ConsoleOut, S, (uint32)CharsToWrite, (LPDWORD)&CharsWritten, NULL);
				Result &= (CharsWritten == CharsToWrite);

				S = ++C;

				SetConsoleTextAttribute(ConsoleOut, WinColor);
			}
		}

		CharsToWrite = (uint32)(Length - (S - String));
		Result &= WriteConsole(ConsoleOut, S, (uint32)CharsToWrite, (LPDWORD)&CharsWritten, NULL);
		Result &= (CharsWritten == CharsToWrite);

		SetConsoleTextAttribute(ConsoleOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
		return Result;
	}

	bool32
	ColorPrint(char* String) { return ColorPrint(StringLength(String), String); }

	bool32
	ColorPrintFormatted(char* FormatString, ...)
	{
		va_list args;
		va_start(args, FormatString);

		uint32 FormattedLength = _vscprintf(FormatString, args);
		char* ScratchBuffer = (char*)Alloc(FormattedLength + 1, false);
		vsprintf_s(ScratchBuffer, FormattedLength + 1, FormatString, args);
		
		va_end(args);

		bool32 Result = ColorPrint(FormattedLength, ScratchBuffer);
		Free(ScratchBuffer);

		return Result;
	}

	astring
	ReadLine()
	{
		astring Result;
		Result.Capacity = 1024;
		Result.Value = (char*)Alloc(Result.Capacity, false);
		
		DWORD CharsRead;

		ReadConsole(ConsoleIn, (void*)Result.Value, (DWORD)Result.Capacity, (&CharsRead), NULL);

		Result.Value[CharsRead - 2] = Result.Value[CharsRead - 1] = 0;
		Result.Length = CharsRead - 2;
		return Result;
	}

	bool32
	Error(char* ErrorMessage)
	{
		uint32 ErrorLength = StringLength(ErrorMessage);
		SetConsoleTextAttribute(ConsoleError, FOREGROUND_RED);
		uint32 CharsWritten;
		bool32 Result = WriteConsole(ConsoleError, ErrorMessage, (uint32)ErrorLength, (LPDWORD)&CharsWritten, NULL);
		SetConsoleTextAttribute(ConsoleError, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
		
		return Result && CharsWritten == ErrorLength;
	}

	/* ==================
		   Filesystem 
	   ================== */

	astring
	GetCurrentDirectory()
	{
		astring Result;
		Result.Capacity = 1024;
		Result.Value = (char*)Alloc(Result.Capacity * sizeof(char), false); 
		::GetCurrentDirectory(1024, Result.Value);
		Result.Length = StringLength(Result.Value);
		return Result;
	}

	bool32
	SetCurrentDirectory(char* NewDirectory)
	{
		return ::SetCurrentDirectory(NewDirectory);
	}

	char*
	GetFileName(uint32 PathLength, char* Path)
	{
		return Path + Max(Max(StringLastIndexOf(PathLength, Path, 1, "\\"), StringLastIndexOf(PathLength, Path, 1, "/")), 0);
	}

	bool32
	FindInDirectory(char* Path, char* Search)
	{
		uint32 PathLen = StringLength(Path);
		DWORD FileAttributes = GetFileAttributes(Path);
		if (FileAttributes != INVALID_FILE_ATTRIBUTES && FileAttributes & FILE_ATTRIBUTE_DIRECTORY) // It's a directory
		{
			char SearchName[MAX_PATH + 2] = {};
			Assert(PathLen < MAX_PATH);
			CatStrings(PathLen, Path, 2, "\\*", MAX_PATH + 2, SearchName);

			WIN32_FIND_DATA FindData = {};
			HANDLE FindHandle = FindFirstFile(SearchName, &FindData);

			bool32 Found = true;
			do
			{   
				uint32 Attribs = FindData.dwFileAttributes;
				if (CompareStrings(FindData.cFileName, Search))
				{
					return true;
				}
				Found = FindNextFile(FindHandle, &FindData);
			} while(Found != 0);

			uint32 LastError = GetLastError();
			if (LastError != ERROR_NO_MORE_FILES)
			{
				OutputDebugString("Couldn't get all files: ");
				win32::OutputDebugError();
				return false;
			}

			FindClose(FindHandle);
		}
		else if (FileAttributes != INVALID_FILE_ATTRIBUTES) // It's a file
		{
			return CompareStrings(GetFileName(PathLen, Path), Search);
		}
		return false;
	}

	bool32
	PathExists(char* Path)
	{
		DWORD FileAttributes = GetFileAttributes(Path);
		return FileAttributes != INVALID_FILE_ATTRIBUTES;
	}

	bool32
	PathIsFolder(char* Path)
	{
	    return (GetFileAttributes(Path) & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool32
	CreateDirectory(char* Path)
	{
		return ::CreateDirectory(Path, null);
	}

	astring*
	ListFilesInDirectory(char* Path, int* NumberOfListedFiles)
	{
		*NumberOfListedFiles = 0;
		char* Files[1024]; //TODO(chronister): This limit

		uint32 PathLen = StringLength(Path);
		DWORD FileAttributes = GetFileAttributes(Path);
		if (FileAttributes != INVALID_FILE_ATTRIBUTES && FileAttributes & FILE_ATTRIBUTE_DIRECTORY) // It's a directory
		{
			char SearchName[MAX_PATH + 2] = {};
			Assert(PathLen < MAX_PATH);
			CatStrings(PathLen, Path, 2, "\\*", MAX_PATH + 1, SearchName);

			WIN32_FIND_DATA FindData = {};
			HANDLE FindHandle = FindFirstFile(SearchName, &FindData);

			bool32 Found = true;
			do
			{   
				uint32 Attribs = FindData.dwFileAttributes;
				if (Attribs & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Don't recurse
				}
				else
				{
					char FullFilename[2 * MAX_PATH + 1];
					CatStrings(PathLen, Path, 1, "\\", MAX_PATH * 2, FullFilename);
					CatStrings(PathLen + 1, FullFilename, StringLength(FindData.cFileName), FindData.cFileName, MAX_PATH * 2, FullFilename);

					Files[(*NumberOfListedFiles)++] = DuplicateCString(FullFilename);
				}
				Found = FindNextFile(FindHandle, &FindData);
			} while(Found != 0);

			uint32 LastError = GetLastError();
			if (LastError != ERROR_NO_MORE_FILES)
			{
				OutputDebugString("Couldn't get all files: ");
				win32::OutputDebugError();
			}

			FindClose(FindHandle);
		}
		else if (FileAttributes != INVALID_FILE_ATTRIBUTES) // It's a file
		{
			Files[(*NumberOfListedFiles)++] = Path;
		}

		astring* Result = (astring*)Alloc(*NumberOfListedFiles * sizeof(astring), false);
		for (int i = 0; i < *NumberOfListedFiles; ++i)
		{
			Result[i] = ASTR(Files[i]);
		}
		return Result;
	}

	void
	FreeFile(read_file_result File)
	{
		VirtualFree(File.Contents, 0, MEM_RELEASE);
		File.Contents = NULL;
		File.ContentsSize = 0;
	}

	read_file_result
	ReadFile(char* Filename, int32 BytesToRead)
	{
		read_file_result Result = {};

		HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{

			LARGE_INTEGER FileSize;
			if (GetFileSizeEx(FileHandle, &FileSize))
			{
				
				uint32 FileSize32 = BytesToRead;
				if (BytesToRead < 0)
				{
					FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);  
				} 
				Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				if (Result.Contents)
				{
					DWORD BytesRead;
					if (::ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
						(FileSize32 == BytesRead)) 
					{
						//NOTE(chronister): File read successfully
						Result.ContentsSize = FileSize32;
					}
					else 
					{
						//TODO(chronister): Logging
						VirtualFree(Result.Contents, FileSize32, MEM_RELEASE);
						Result.Contents = 0;
					}
					
				}
				else 
				{
					//TODO(chronister): Logging
				}
			}
			else 
			{
				//TODO(chronister): Logging
			}

			CloseHandle(FileHandle);
		}
		else 
		{
			//TODO(chronister): Logging
		}

		return(Result);
	}

	read_file_result
	ReadEntireFile(char* Filename)
	{
		return ReadFile(Filename, -1);
	}

	bool32
	AppendToFile(char* Filepath, uint32 StringSize, char* StringToAppend)
	{
		bool32 Result = false;

		HANDLE FileHandle = CreateFile(Filepath, FILE_APPEND_DATA, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			DWORD BytesWritten;
			if (WriteFile(FileHandle, StringToAppend, (DWORD)StringSize, &BytesWritten, 0)) 
			{
				//NOTE(chronister): File appended successfully
				Result = (BytesWritten == StringSize);
			}
			else 
			{
				//TODO(chronister): Logging
				PrintFC("|R`Unable to write to %s!\n", Filepath);
			}

			CloseHandle(FileHandle);
		}
		else 
		{
			//TODO(chronister): Proper logging
			PrintFC("|R`Unable to open %s for appending!", Filepath);
		}

		return Result;
	}

	bool32
	WriteEntireFile(char* Filepath, uint32 StringSize, char* StringToWrite)
	{
		bool32 Result = false;

		Assert(StringSize < UINT32_MAX);

		HANDLE FileHandle = CreateFileA(Filepath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{

			DWORD BytesWritten;
			if (WriteFile(FileHandle, StringToWrite, (DWORD)StringSize, &BytesWritten, 0)) 
			{
				//NOTE(chronister): File read successfully
				Result = (BytesWritten == StringSize);
			}
			else 
			{
				//TODO(chronister): Proper Logging
				PrintFC("|R`Unable to write to %s!\n", Filepath);
			}

			CloseHandle(FileHandle);
		}
		else 
		{
			//TODO(chronister): Proper Logging
			PrintFC("|R`Unable to open %s for writing!\n", Filepath);
		}

		return(Result);
	}

	bool32
	FileExists(char* Filepath)
	{
	    WIN32_FIND_DATA Ignored;
	    if (FindFirstFile(Filepath, &Ignored) != INVALID_HANDLE_VALUE)
	    {
	        return true;
	    }
	    return false;
	}

	astring
	GetUserDir()
	{
	    char Path[MAX_PATH];
	    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, Path))) 
	    {
	        uint32 PathLen = StringLength(Path);
			astring Result = AllocateString(PathLen + 1);
			AppendToString(&Result, BSTR(Path));
			AppendToString(&Result, BSTR("/"));

	        return Result;
	    }
	    else
	    {
	        //TODO(chronister): Proper logging!
	        astring Nil = {0};
	        return Nil;
	    }
	}

	/* =====================
		 Command execution 
	   =====================*/

	void
	RunCommand(char* CommandLine)
	{
		STARTUPINFO StartupInfo = {};

		SECURITY_ATTRIBUTES BufferSecurity = {};
		BufferSecurity.nLength = sizeof(SECURITY_ATTRIBUTES); 
		BufferSecurity.bInheritHandle = true;

		StartupInfo.cb = sizeof(StartupInfo);

		PROCESS_INFORMATION ProcessInfo = {};
		CreateProcess(0, CommandLine, 
			0, 0, /* Security stuff */
			true,
			0 /* Creation flags */,
			0 /* ENV */, 
			0 /* cwd */,
			&StartupInfo,
			&ProcessInfo);

		DWORD ExitCode;
		do
		{
			Sleep(100);
			GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
		}
		while(ExitCode == STILL_ACTIVE);

		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);
	}

	astring
	RunCommandCaptureOutput(char* CommandLine, int CharsToCapture)
	{
		STARTUPINFO StartupInfo = {};

		SECURITY_ATTRIBUTES BufferSecurity = {};
		BufferSecurity.nLength = sizeof(SECURITY_ATTRIBUTES); 
		BufferSecurity.bInheritHandle = true;

		HANDLE ChildStandardOutRead;
		HANDLE ChildStandardOutWrite;
		CreatePipe(&ChildStandardOutRead, &ChildStandardOutWrite, &BufferSecurity, 0);
		SetHandleInformation(ChildStandardOutRead, HANDLE_FLAG_INHERIT, 0);

		StartupInfo.cb = sizeof(StartupInfo);
		StartupInfo.dwFlags = STARTF_USESTDHANDLES;
		StartupInfo.hStdOutput = ChildStandardOutWrite; 

		PROCESS_INFORMATION ProcessInfo = {};
		CreateProcess(0, CommandLine, 
			0, 0, /* Security stuff */
			true,
			0 /* Creation flags */,
			0 /* ENV */, 
			0 /* cwd */,
			&StartupInfo,
			&ProcessInfo);

		DWORD ExitCode;
		do
		{
			Sleep(100);
			GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
		}
		while(ExitCode == STILL_ACTIVE);

		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);

		astring Result = {};
		if (CharsToCapture > 0)
		{
			Result.Capacity = CharsToCapture;
			Result.Value = (char*)Alloc(Result.Capacity, false);
			
			DWORD CharsRead;

			SetConsoleMode(ChildStandardOutRead, 0);
			::ReadFile(ChildStandardOutRead, (void*)Result.Value, (DWORD)Result.Capacity, (&CharsRead), NULL);

			Result.Value[CharsRead - 2] = Result.Value[CharsRead - 1] = 0;
			Result.Length = CharsRead - 2;
		}

		CloseHandle(ChildStandardOutRead);
		CloseHandle(ChildStandardOutWrite);

		return Result;
	}

	/* TODO
	string
	RunCommandInteractive(char* CommandLine, int CharsToCapture)
	{
		// Need to figure out how to specify which parts of the command get captured
		// If there's prompts, they need to be echoed so the user knows what to enter
		// How would this one be different from simply RunCommand?	
	}
	*/

	plat::time
	FileTimeToPlatformTime(FILETIME FileTime)
	{
		SYSTEMTIME SysTime;
		FileTimeToSystemTime(&FileTime, &SysTime);

		plat::time Result = {};
		Result.Year 		= (uint16)SysTime.wYear;
		Result.Month 		= (uint8 )SysTime.wMonth;
		Result.YearDay 		= (uint8 )(SysTime.wMonth * 31 + SysTime.wDay); // TODO(chronister): this is incorrect!
		Result.MonthDay 	= (uint8 )SysTime.wDay;
		Result.WeekDay 		= (uint8 )SysTime.wDayOfWeek;
		Result.Hour 		= (uint8 )SysTime.wHour;
		Result.Minute 		= (uint8 )SysTime.wMinute;
		Result.Second		= (uint8 )SysTime.wSecond;
		Result.Millisecond 	= (uint16)SysTime.wMilliseconds;

		// Filetime is given in 100-nanosecond increments
		Result.Microsecond = FileTime.dwLowDateTime / 10;

		return Result;
	}

	uint64
	FileTimeAsInt(FILETIME FileTime)
	{
		ULARGE_INTEGER LargeInt;
		LargeInt.LowPart = FileTime.dwLowDateTime;
		LargeInt.HighPart = FileTime.dwHighDateTime;
		return LargeInt.QuadPart;
	}

	rawtime
	GetTimestamp()
	{
	    FILETIME PreciseTime;
		GetSystemTimeAsFileTime(&PreciseTime);
        return FileTimeAsInt(PreciseTime);
	}

#define WINDOWS_TICK(NS) NS/10000000
#define S_FROM_1601_TO_UNIX_EPOCH 11644473600LL
	int64
	GetUnixTimestamp()
	{
	    FILETIME PreciseTime;
		GetSystemTimeAsFileTime(&PreciseTime);
        rawtime Ticks = FileTimeAsInt(PreciseTime);
        rawtime SecondsSince1601 = WINDOWS_TICK(Ticks);
        return SecondsSince1601 - S_FROM_1601_TO_UNIX_EPOCH;
	}
#undef WINDOWS_TICK
#undef S_FROM_1601_TO_UNIX_EPOCH

	plat::time
	GetTimeUniversal()
	{
		FILETIME CurrentTime;
		GetSystemTimeAsFileTime(&CurrentTime);

		return FileTimeToPlatformTime(CurrentTime);
	}

	plat::time
	GetTimeUniversal(rawtime Timestamp)
	{
		FILETIME WindowsFileTime;
		WindowsFileTime.dwLowDateTime = Timestamp & UINT32_MAX;
		WindowsFileTime.dwHighDateTime = Timestamp >> 32;
		return FileTimeToPlatformTime(WindowsFileTime);
	}

	plat::time
	GetTimeLocal()
	{
		FILETIME UniversalFileTime; 
		GetSystemTimeAsFileTime(&UniversalFileTime);

		FILETIME LocalFileTime;
		FileTimeToLocalFileTime(&UniversalFileTime, &LocalFileTime);

		return FileTimeToPlatformTime(LocalFileTime);
	}

	plat::time
	GetTimeLocal(rawtime Timestamp)
	{
		FILETIME UniversalFileTime;
		UniversalFileTime.dwLowDateTime = Timestamp & UINT32_MAX;
		UniversalFileTime.dwHighDateTime = Timestamp >> 32;

		FILETIME LocalFileTime;
		FileTimeToLocalFileTime(&UniversalFileTime, &LocalFileTime);

		return FileTimeToPlatformTime(LocalFileTime);
	}

	int LogSimple(uint32 Length, char* String)
	{
		DWORD BytesWritten;
		::WriteFile(LogFile, String, Length, &BytesWritten, 0); 
		return 0;
	}

	int LogSimple(char* String)
	{
        return LogSimple(StringLength(String), String);
	}
}

#ifdef CHR_INET
internal bool32
InitializeSockets()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		LogF(plat::LOG_WARNING, "WSAStartup failed with error: %d\n", iResult);
	}
	return iResult == 0;
}

internal void 
CleanupSockets()
{
	WSACleanup();
}

int
GetSocketError()
{
	return WSAGetLastError();
}

#endif

int WindowsMain(int argc, char* argv[]);

#if defined(CHR_MAIN)
int main(int argc, char* argv[])
{
	plat::Initialize();
#ifdef CHR_INET
	InitializeSockets();
#endif

    return WindowsMain(argc, argv);
}
#else

#endif

#define CHR_WINPLATFORM
#endif
