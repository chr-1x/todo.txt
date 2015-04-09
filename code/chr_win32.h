/* ===============================
    Standard, useful Win32 things
   =============================== */
/*
    Some standard terminology:
     - Filename: just a files's name (todo.txt)
     - Filepath|path: file name + full directory
     - Directory: full directory path
*/
#include "windows.h"
#include "commctrl.h" // For control-related things

#define WIN32_FILENAME_SIZE MAX_PATH

namespace win32
{

    inline void
    OutputDebugError()
    {
        uint32 Code = GetLastError();
        char* MessagePtr;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    0, Code, 0, (char*)(&MessagePtr), 0, 0);
        OutputDebugString(MessagePtr);
    }
        
    inline FILETIME
    GetLastWriteTime(char *Filename)
    {
        FILETIME LastWriteTime = {};

        WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
        if (GetFileAttributesExA(Filename, GetFileExInfoStandard, &FileAttributes))
        {
            LastWriteTime = FileAttributes.ftLastWriteTime;
        }

        return LastWriteTime;
    }

    void
    GetEXEPath(char* FilepathOut)
    {
        //TODO(chronister): test this and figure out if its even necessary
        DWORD SizeOfFilename = GetModuleFileName(0, FilepathOut, WIN32_FILENAME_SIZE);
    }

    HWND
    EditControl(HWND Window, uint32 ID, 
    	int32 X, int32 Y, int32 Width, int32 Height, 
    	char* InitialText = 0, int LineLength = 32767, uint32 Flags=0)
    {
    	HWND Result = CreateWindowEx(NULL, WC_EDIT, InitialText,
	        WS_CHILD|WS_VISIBLE|Flags,
	        X, Y, Width, Height,
	        Window, (HMENU)ID,
	        0, 0);
	    Assert(Result); 
	    SendMessage(Result, EM_SETLIMITTEXT, LineLength, 0);
	    return Result;
    }

    HWND
    LabelControl(HWND Window, uint32 ID, 
    	int32 X, int32 Y, int32 Width, int32 Height,
    	char* LabelText, uint32 Flags=0)
    {
    	HWND Result = CreateWindowEx(NULL, WC_STATIC, LabelText,
	        WS_CHILD|WS_VISIBLE|Flags,
	        X, Y, Width, Height,
	        Window, (HMENU)ID,
	        0, 0);
    	Assert(Result);
    	return Result;
    }

    HWND
    ButtonControl(HWND Window, uint32 ID, 
    	int32 X, int32 Y, int32 Width, int32 Height,
    	char* LabelText=0, uint32 Flags=0)
    {
    	HWND Result = CreateWindowEx(NULL, WC_BUTTON, LabelText,
	        WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|Flags,
	        X, Y, Width, Height,
	        Window, (HMENU)ID,
	        0, 0);
    	Assert(Result);
    	return Result;
    }

    HWND
    UpDownControl(HWND Window, uint32 ID,
        int32 X, int32 Y, int32 Width, int32 Height,
        HWND BuddyWindow, 
        int MinValue = 0, int MaxValue = 100, 
        uint32 Flags=0)
    {
        HWND Result = CreateWindowEx(NULL, UPDOWN_CLASS, NULL /* Unused? */,
            WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_HOTTRACK|Flags,
            0, 0, 0, 0, // Automatically size
            Window, (HMENU)ID,
            0, 0);
        Assert(Result);
        SendMessage(Result, UDM_SETRANGE32, MinValue, MaxValue);
        SendMessage(Result, UDM_SETBUDDY, (WPARAM)BuddyWindow, 0);
        return Result;
    }

    HWND
    TreeViewControl(HWND Window, uint32 ID,
        int32 X, int32 Y, int32 Width, int32 Height,
        char* Label = 0,
        uint32 Flags=0)
    {
        HWND Result = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, Label,
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|Flags,
            X, Y, Width, Height, 
            Window, (HMENU)ID,
            0, 0);
        Assert(Result);
        return Result;
    }

    HWND
    DateTimeControl(HWND Window, uint32 ID,
        int32 X, int32 Y, int32 Width, int32 Height,
        char* Format = 0,
        char* Label = 0,
        uint32 Flags=0)
    {
        HWND Result = CreateWindowEx(0, DATETIMEPICK_CLASS, Label,
           WS_BORDER|WS_CHILD|WS_VISIBLE|Flags,
           X, Y, Width, Height,
           Window, (HMENU)ID,
           0, 0);
        Assert(Result);
        DateTime_SetFormat(Result, Format);
        return Result;
    }

    struct popup_menu_item
    {
        uint32 MenuFlags;
        uint32 ItemID;
        void* Content;
    };

    HMENU
    PopupMenu(size_t NumMenuItems, popup_menu_item* MenuItems)
    {
        HMENU Result = CreatePopupMenu();
        MENUINFO NotifyMenuInfo = {};
        NotifyMenuInfo.cbSize = sizeof(MENUINFO);
        NotifyMenuInfo.fMask = MIM_STYLE;
        NotifyMenuInfo.dwStyle = MNS_NOTIFYBYPOS;
        SetMenuInfo(Result, &NotifyMenuInfo);

        if (MenuItems) {
            foreach(popup_menu_item, Item, NumMenuItems, MenuItems)
            {
                AppendMenu(Result, Item->MenuFlags, Item->ItemID, (char*)Item->Content);
            }
        }

        return Result;
    }

    uint64 
    GetAssembledFileTime(FILETIME* TimeIn)
    {
        ULARGE_INTEGER LargeInt = {};
        LargeInt.LowPart = TimeIn->dwLowDateTime;
        LargeInt.HighPart = TimeIn->dwHighDateTime;
        return LargeInt.QuadPart;
    }

    uint64
    GetSystemTimestamp()
    {
        SYSTEMTIME SysTime = {};
        FILETIME FileTime = {};

        GetSystemTime(&SysTime);
        SystemTimeToFileTime(&SysTime, &FileTime);

        return GetAssembledFileTime(&FileTime);
    }

    void
    PrintDebug(char* FormatString, ...)
    {
        size_t Length = StringLength(FormatString);
        size_t FormattedLength = 256;
        char ScratchBuffer[256];
        
        va_list args;
        va_start(args, FormatString);
        vsprintf_s(ScratchBuffer, FormattedLength, FormatString, args);
        va_end(args);

        OutputDebugString(ScratchBuffer);
    }

    #if MEMCOUNT
    #include "psapi.h"
    void
    PrintMemoryUsage(char* Label, int Optional = 0)
    {
        PROCESS_MEMORY_COUNTERS MemCounter;
        GetProcessMemoryInfo(GetCurrentProcess(), &MemCounter, sizeof(MemCounter));

        if (Optional)
        {
            PrintDebug("%s %d\t%d\t%d\n", Label, Optional, MemCounter.WorkingSetSize, MemCounter.PagefileUsage);
        }
        else
        {
            PrintDebug("%s\t%d\t%d\n", Label, MemCounter.WorkingSetSize, MemCounter.PagefileUsage);
        }
    }
    #else
    void
    PrintMemoryUsage(char* Label, int Optional = 0)
    {
        PrintDebug("Memory usage is disabled! Set the MEMCOUNT macro to 1.\n");
    }
    #endif

    void
    HideWindowAndTaskbarIcon(HWND Window)
    {
        int32 Style = GetWindowLong(Window, GWL_STYLE);
        Style &= ~(WS_VISIBLE);    // this works - window become invisible 

        Style |= WS_EX_TOOLWINDOW;
        Style &= ~(WS_EX_APPWINDOW); 

        ShowWindow(Window, SW_HIDE); // hide the window
        SetWindowLong(Window, GWL_STYLE, Style); // set the style
    }

    void
    ShowWindowAndTaskbarIcon(HWND Window)
    {
        int32 Style = GetWindowLong(Window, GWL_STYLE);
        Style |= WS_VISIBLE;    // this works - window become invisible 

        Style &= ~(WS_EX_TOOLWINDOW);
        Style |= WS_EX_APPWINDOW; 

        ShowWindow(Window, SW_SHOW); // hide the window
        SetWindowLong(Window, GWL_STYLE, Style); // set the style
    }
}