@echo off
setlocal

echo ================================
echo  Todo.txt Parser -- Build Batch
echo ================================
echo.

IF NOT EXIST build mkdir build
pushd build

goto x32
:x64
IF NOT EXIST win64 mkdir win64
pushd win64
call "%VC%\vcvarsall.bat" amd64
cl /nologo /MTd /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4706 /FC /Z7 /Fm ../../code/windows_todo.cpp user32.lib shell32.lib  /link /incremental:no /opt:ref /subsystem:console,5.2 /OUT:todo.exe
goto eof

:x32
IF NOT EXIST win32 mkdir win32
pushd win32
call "%VC%\vcvarsall.bat" amd64_x86
cl /nologo /MTd /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4706 /FC /Z7 /Fm ../../code/windows_todo.cpp user32.lib shell32.lib  /link /incremental:no /opt:ref /subsystem:console,5.1 /OUT:todo.exe
goto eof

popd
popd

:eof
