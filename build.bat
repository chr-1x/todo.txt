@echo off
setlocal

echo ================================
echo  Todo.txt Parser -- Build Batch
echo ================================
echo.

IF NOT EXIST build mkdir build
pushd build

set BASE_NAME=win32_todo
set EXE_NAME=todo.exe

set COMPAT= /MTd
set OPTIMIZATION= /Gm- /GR- /EHa- /Od /Oi
set WARNINGS= /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505
set DEBUG= /FC /Z7 /Fm
set CL_OPTIONS= /nologo %COMPAT% %OPTIMIZATION% %WARNINGS% %DEBUG%

set CODE= ../../code/win32_todo.cpp
set LIBS= user32.lib gdi32.lib winmm.lib
set LINKER_SHARED= /incremental:no /opt:ref 
set LINKER_64= /link %LINKER_SHARED% /subsystem:console,5.2
set LINKER_32= /link %LINKER_SHARED% /subsystem:console,5.1

set TIMESTAMP=%DATE:/=-%.%TIME::=-%
set TIMESTAMP=%TIMESTAMP: =%

set CLEAN=""
set ACTED=""

:arg-top
if "%~1" NEQ "" (
  if /i "%~1"=="clean" goto clean
  if /i "%~1"=="x64"   goto x64
  if /i "%~1"=="x86"   goto x86
  :args
  SHIFT
  goto :arg-top
)
if %ACTED%=="" (
  goto x64
)
goto :eof

:clean
echo Cleaning %CD%
del /Q . > NUL 2> NUL
set CLEAN="True"
set ACTED="True"
goto :args

:x64
echo Building x64
IF NOT EXIST x64 mkdir x64
pushd x64
	if %CLEAN% NEQ "" del /Q . > NUL 2> NUL
	del *.pdb > NUL 2> NUL
	call "C:\Programs\Visual Studio 12.0\VC\vcvarsall.bat" amd64
	cl %CL_OPTIONS% %CODE% %LIBS% %LINKER_64% /OUT:%EXE_NAME%
	if ERRORLEVEL 1 goto :eof
popd
set ACTED="True"
echo.
goto :args

:x86
echo Building x86
IF NOT EXIST x86 mkdir x86
pushd x86
	if %CLEAN% NEQ "" del /Q . > NUL 2> NUL
	del *.pdb > NUL 2> NUL
	call "C:\Programs\Visual Studio 12.0\VC\vcvarsall.bat" amd64_x86
	cl %CL_OPTIONS% %CODE% %LIBS% %LINKER_32% /OUT:%EXE_NAME%
	if ERRORLEVEL 1 goto :eof
popd
ACTED="True"
echo.
goto :args

popd