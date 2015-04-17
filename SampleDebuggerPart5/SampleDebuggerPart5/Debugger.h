#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <thread>

#include <Windows.h>

#include "DebugEventHandler.h"
#include "DebugExceptionHandler.h"
#include "Breakpoint.h"
#include "InterruptBreakpoint.h"
#include "SafeHandle.h"
#include "Symbols.h"
#include "Disassembler.h"

namespace CodeReversing
{

class Debugger final
{
public:
    friend class DebugEventHandler;
    friend class DebugExceptionHandler;

    Debugger() = delete;
    Debugger(const DWORD dwProcessId, const bool bKillOnExit = false);

    Debugger(const Debugger &copy) = delete;
    Debugger &operator=(const Debugger &copy) = delete;

    ~Debugger();

    const bool Start();
    const bool Stop();
    const bool StepInto();
    const bool StepOver();
    const bool Continue();

    void PrintCallStack();
    void PrintDisassembly(const DWORD_PTR dwAddress);
    void PrintContext();

    const bool SetExecutingContext(const CONTEXT &ctx);
    const CONTEXT GetExecutingContext();

    const bool ChangeByteAt(const DWORD_PTR dwAddress, const unsigned char cNewByte);
    const bool PrintBytesAt(const DWORD_PTR dwAddress, size_t ulNumBytes = 40);

    const volatile bool IsActive() const;

    const bool AddBreakpoint(const DWORD_PTR dwAddress);
    const bool RemoveBreakpoint(const DWORD_PTR dwAddress);
    Breakpoint * FindBreakpoint(const DWORD_PTR dwAddress);

    const bool AddBreakpoint(const char * const pSymbolName);
    const bool RemoveBreakpoint(const char * const pSymbolName);

    const HANDLE Handle() const;
    const Symbols * const ProcessSymbols() const;

private:
    volatile bool m_bIsActive;
    bool m_bKillOnExit;
    DWORD m_dwProcessId;
    SafeHandle m_hProcess;
    SafeHandle m_hFile;
    SafeHandle m_hContinueEvent;

    Breakpoint *m_pLastBreakpoint;
    DWORD m_dwExecutingThreadId;
    CONTEXT m_lastContext;
    volatile bool m_bIsStepping;

    const bool DebuggerLoop();

    const bool Continue(const bool bIsStepping);
    const bool WaitForContinue() const;

    const DWORD ChangeMemoryPermissions(const DWORD_PTR dwAddress, const size_t ulSize, DWORD dwNewPermissions);
    SafeHandle OpenCurrentThread();

    std::unique_ptr<DebugEventHandler> m_pEventHandler;
    std::unique_ptr<DebugExceptionHandler> m_pExceptionHandler;
    std::unique_ptr<Symbols> m_pSymbols;

    std::unique_ptr<InterruptBreakpoint> m_pStepPoint;
    std::unique_ptr<Disassembler> m_pDisassembler;

    std::list<std::unique_ptr<Breakpoint>> m_lstBreakpoints;

};

}