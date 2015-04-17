#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include "Debugger.h"

DWORD WINAPI DebuggingThread(LPVOID lpParameters)
{
    CodeReversing::Debugger *pDebugger = (CodeReversing::Debugger *)lpParameters;

    return pDebugger->Start();
}

int main(int argc, char *argv[])
{
    DWORD dwPid = 0;
    fprintf(stderr, "Enter target process id to attach to: ");
    fscanf(stdin, "%i", &dwPid);

    CodeReversing::Debugger dbg(dwPid);
    DWORD dwThreadId = 0;

    HANDLE hDebugThread = CreateThread(nullptr, 0, DebuggingThread, &dbg, 0, &dwThreadId);

    printf("[A]dd breakpoint.\n"
        "[R]emove breakpoint.\n"
        "[S]tep instruction.\n"
        "[C]ontinue.\n"
        "[Q]uit.\n");

    char cInput = 0;
    DWORD_PTR dwTargetAddress = 0;

    do
    {
        cInput = getchar();
        switch (cInput)
        {
        case 'A':
        case 'a':
            printf("Target address: 0x");
            fscanf(stdin, "%p", &dwTargetAddress);
            (void)dbg.AddBreakpoint(dwTargetAddress);
            break;
        case 'R':
        case 'r':
            printf("Target address: 0x");
            fscanf(stdin, "%p", &dwTargetAddress);
            (void)dbg.RemoveBreakpoint(dwTargetAddress);
            break;
        case 'S':
        case 's':
            (void)dbg.StepInto();
            break;
        case 'C':
        case 'c':
            (void)dbg.Continue();
            break;
        }

    } while (cInput != 'Q' || cInput != 'q');

    return 0;
}