Todo.txt
===

This is a simple C+ CLI interface to [Todo.txt](http://todotxt.com/) files as specified by 
[Gina Trapani](https://github.com/ginatrapani/todo.txt-cli). 

I refer to the code style as C+ because it makes use of some C++ conveniences, such as simplified
struct declaration, namespaces, and a (simple) array template, but is otherwise written in a 
procedural style that is closest to software written in C.

Supported commands
===
 - List todo items ("ls"/"list")
 - Add an item ("a"/"add")
 - Edit an item ("edit"/"replace")
 - Remove an item ("rm"/"del"/"remove")
 - Prioritize or reprioritize an item ("p"/"pri"/"prioritize")
 - Deprioritize an item ("dp"/"depri"/"deprioritize")
 - Mark an item completed ("do"/"did"/"complete"/"finish")
 - Archive an item to done.txt ("ar"/"archive")
 - Add more text to an item ("addkeyword"/"addkw"/"akw"/"append"/"app")
 - Add a #Project tag to an item ("addproject"/"addproj"/"ap")
 - Add a @Context tag to an item ("addcontext"/"addcon"/"ac")
 - Remove words from an item ("removekeyword"/"rmkw"/"rkw")
 - Remove a #Project tag from an item ("removeproject"/"rmproj"/"rp")
 - Remove a @Context tag from an item ("removecontext"/"rmcon"/"rc")
 - Initialize a todo.txt file in the current directory ("init"/"in")

By default, it looks for a todo.txt file in the following locations, in order:
 1 Current working directory
 1 Each folder up to root
 1 User profile

It will ask to create a todo.txt file in your user profile directory if none was found.

Building
===
The easiest way to build the program is by running `build.bat` or `build.sh`. build.bat presently assumes that an
environment variable called %VC% is available that points to Visual Studio's C/C++ tools folder, e.g.:

`C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC`

A visual studio vcxproj, and a makefile, will be added shortly.

The build process is extremely simple as this project makes use of a Single Translation Unit build format, where only
one file is passed to the compiler. This simplifies the compilation process tremendously, eliminating the need for
complicated build systems. The file passed to the compiler only needs to include whichever other files it needs directly.
As long as care is taken to guard headers, as always, there are no conflicts.

