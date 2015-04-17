#include <Windows.h>
#include "Debugger.h"

int main(int argc, char *argv[])
{
    CodeReversing::Debugger dbg(1234);

    return dbg.Start();
}