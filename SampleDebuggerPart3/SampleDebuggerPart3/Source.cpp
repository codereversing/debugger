#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include "Debugger.h"

DWORD WINAPI DebuggingThread(LPVOID lpParameters)
{
    CodeReversing::Debugger *pDebugger = (CodeReversing::Debugger *)lpParameters;

    return pDebugger->Start();
}

CONTEXT PromptModifyContext(CodeReversing::Debugger *dbg)
{
    CONTEXT ctx = dbg->GetExecutingContext();
    char strRegister[8] = { 0 };

#ifdef _M_IX86
    char *pRegisters[] = {
        "EAX", "EBX", "ECX", "EDX",
        "ESP", "EBP", "ESI", "EDI",
        "EIP", "EFlags"
    };
    DWORD_PTR *pdwRegisters[] = {
        &ctx.Eax, &ctx.Ebx, &ctx.Ecx, &ctx.Edx,
        &ctx.Esp, &ctx.Ebp, &ctx.Esi, &ctx.Edi,
        &ctx.Eip, &ctx.EFlags
    };

#elif defined _M_AMD64
    char *pRegisters[] = {
        "RAX", "RBX", "RCX", "RDX",
        "RSP", "RBP", "RSI", "RDI",
        "R8", "R9", "R10", "R11",
        "R12", "R13", "R14", "R15",
        "RIP", "EFlags"
    };
    DWORD_PTR *pdwRegisters[] = {
        &ctx.Rax, &ctx.Rbx, &ctx.Rcx, &ctx.Rdx,
        &ctx.Rsp, &ctx.Rbp, &ctx.Rsi, &ctx.Rdi,
        &ctx.R8, &ctx.R9, &ctx.R10, &ctx.R11,
        &ctx.R12, &ctx.R13, &ctx.R14, &ctx.R15,
        &ctx.Rip
    };

#else
#error "Unsupported platform"
#endif

    fprintf(stderr, "Enter register to change: ");
    fscanf(stdin, "%7s", strRegister);

    for (size_t i = 0; i < sizeof(pRegisters) / sizeof(pRegisters[0]); ++i)
    {
        if (_stricmp(pRegisters[i], strRegister) == 0)
        {
            DWORD_PTR dwNewValue = 0;
            fprintf(stderr, "Enter new value for register %s: 0x", pRegisters[i]);
            fscanf(stdin, "%p", &dwNewValue);
#ifdef _M_AMD64
            //Annoying workaround since EFlags is 32-bit on x64
            if (_stricmp("EFlags", strRegister) == 0)
            {
                DWORD *pdwEFlags = &ctx.EFlags;
                *pdwEFlags = (DWORD)dwNewValue;
            }
            else
            {
                *pdwRegisters[i] = dwNewValue;
            }
#else
            *pdwRegisters[i] = dwNewValue;
#endif
            break;
        }

    }

    return ctx;
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
        "[P]rint context\n"
        "[M]odify context"
        "Ca[l]l Stack\n"
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
        case 'P':
        case 'p':
            dbg.PrintContext();
            break;
        case 'M':
        case 'm':
        {
            CONTEXT ctx = PromptModifyContext(&dbg);
            (void)dbg.SetExecutingContext(ctx);
        }
            break;
        case 'L':
        case 'l':
            dbg.PrintCallStack();
            break;
        }

    } while (cInput != 'Q' || cInput != 'q');

    return 0;
}