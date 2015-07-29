#include "chr_winplatform.h"
#include "todo.cpp"

#undef Assert
#define Assert(Statement) !(Statement) && (LogF(plat::LOG_ERROR, "!! ASSERTION FAILED !! " #Statement " at " __FILE__ ": %d\n", __LINE__))

//#pragma clang diagnostic ignored "-Wwritable-strings"

int WindowsMain(int argc, char* argv[])
{
    return RunFromArguments(ParseArgs(argc, argv));
}
