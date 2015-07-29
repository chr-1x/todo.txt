@echo off
setlocal

echo ================================
echo  Todo.txt Parser -- Build Batch
echo ================================
echo.

IF NOT EXIST build mkdir build
pushd build

set BASE_NAME=windows_todo
set EXE_NAME=todo.exe

set COMPAT= /MTd
set OPTIMIZATION= /GR- /EHa- /Od /Oi
set WARNINGS= /w
set DEBUG= /Zi
set DEFINES= /DMEMCOUNT=1 /DCHR_MAIN
set INCLUDE= /I%DEV%/lib/chr
set CL_OPTIONS= /nologo %COMPAT% %DEFINES% %OPTIMIZATION% %WARNINGS% %DEBUG% %INCLUDE%

set CODE= ../../code/windows_todo.cpp
set LIBS= user32.lib shell32.lib
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
  if /i "%~1"=="win64"   goto win64
  if /i "%~1"=="win32"   goto win32
  :args
  SHIFT
  goto :arg-top
)
if %ACTED%=="" (
  goto win32
)
goto :eof

:clean
echo Cleaning %CD%
del /Q . > NUL 2> NUL
set CLEAN="True"
set ACTED="True"
goto :args

:win64
echo Building win64
IF NOT EXIST win64-clang mkdir win64-clang
pushd win64-clang
	if %CLEAN% NEQ "" del /Q . > NUL 2> NUL
	del *.pdb > NUL 2> NUL
	call "C:\Programs\Visual Studio 12.0\VC\vcvarsall.bat" amd64
	clang-cl %CL_OPTIONS% %CODE% -m64 %LIBS% %LINKER_64% /OUT:%EXE_NAME%
	if ERRORLEVEL 1 goto :eof
popd
set ACTED="True"
echo.
goto :args

:win32
echo Building win32
IF NOT EXIST win32-clang mkdir win32-clang
pushd win32-clang
	if %CLEAN% NEQ "" del /Q . > NUL 2> NUL
	del *.pdb > NUL 2> NUL
	call "C:\Programs\Visual Studio 12.0\VC\vcvarsall.bat" amd64_x86
	clang-cl %CL_OPTIONS% %CODE% %LIBS% %LINKER_32% /OUT:%EXE_NAME%
	if ERRORLEVEL 1 goto :eof
popd
set ACTED="True"
echo.
goto :args

popd
