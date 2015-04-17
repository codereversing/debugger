#pragma comment(lib, "DbgHelp.lib")

#include "Debugger.h"

#include <algorithm>
#include <cstdio>
#include <DbgHelp.h>

#include "Common.h"

namespace CodeReversing
{

Debugger::Debugger(const DWORD dwProcessId, const bool bKillOnExit /*= false*/) : m_bIsActive{ false },
    m_dwProcessId{ dwProcessId }, m_bKillOnExit{ bKillOnExit }, m_hProcess{ INVALID_HANDLE_VALUE },
    m_pLastBreakpoint{ nullptr }, m_dwExecutingThreadId{ 0 }, m_bIsStepping{ false }
{
    m_pEventHandler = std::unique_ptr<DebugEventHandler>(new DebugEventHandler(this));
    m_pExceptionHandler = std::unique_ptr<DebugExceptionHandler>(new DebugExceptionHandler(this));
    m_hContinueEvent = CreateEvent(nullptr, false, false, L"Continue event");
}

Debugger::~Debugger()
{
}

const bool Debugger::Start()
{
    m_bIsActive = BOOLIFY(DebugActiveProcess(m_dwProcessId));
    if (m_bIsActive)
    {
        const bool bIsSuccess = BOOLIFY(DebugSetProcessKillOnExit(m_bKillOnExit));
        if (!bIsSuccess)
        {
            fprintf(stderr, "Could not set process kill on exit policy. Error = %X\n", GetLastError());
        }
        return DebuggerLoop();
    }
    else
    {
        fprintf(stderr, "Could not debug process %X. Error = %X\n", m_dwProcessId, GetLastError());
    }

    return false;
}

const bool Debugger::Stop()
{
    m_bIsActive = BOOLIFY(DebugActiveProcessStop(m_dwProcessId));
    if (!m_bIsActive)
    {
        fprintf(stderr, "Could not stop debugging process %X. Error = %X\n", m_dwProcessId, GetLastError());
    }

    return m_bIsActive;
}

const volatile bool Debugger::IsActive() const
{
    return m_bIsActive;
}

const bool Debugger::DebuggerLoop()
{
    DEBUG_EVENT dbgEvent = { 0 };
    DWORD dwContinueStatus = 0;
    bool bSuccess = false;

    while (m_bIsActive)
    {
        bSuccess = BOOLIFY(WaitForDebugEvent(&dbgEvent, INFINITE));
        if (!bSuccess)
        {
            fprintf(stderr, "WaitForDebugEvent returned failure. Error = %X\n", GetLastError());
            return false;
        }

        //fprintf(stderr, "Debug event raised in process %X -- thread %X.\n", dbgEvent.dwProcessId, dbgEvent.dwThreadId);

        m_pEventHandler->Notify((DebugEvents)dbgEvent.dwDebugEventCode, dbgEvent);
        dwContinueStatus = m_pEventHandler->ContinueStatus();

        bSuccess = BOOLIFY(ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, dwContinueStatus));
        if (!bSuccess)
        {
            fprintf(stderr, "ContinueDebugEvent returned failure. Error = %X\n", GetLastError());
            return false;
        }
    }

    return true;
}

const bool Debugger::AddBreakpoint(const DWORD_PTR dwAddress)
{
    bool bSuccess = false;
    DWORD dwOldProtect = ChangeMemoryPermissions(dwAddress, sizeof(DWORD_PTR), PAGE_EXECUTE_READWRITE);

    std::unique_ptr<InterruptBreakpoint> pNewBreakpoint(new InterruptBreakpoint(m_hProcess(), dwAddress));
    if (pNewBreakpoint->EnableBreakpoint())
    {
        m_lstBreakpoints.emplace_back(std::move(pNewBreakpoint));
        bSuccess = true;
    }

    (void)ChangeMemoryPermissions(dwAddress, sizeof(DWORD_PTR), dwOldProtect);

    return bSuccess;
}

const bool Debugger::RemoveBreakpoint(const DWORD_PTR dwAddress)
{
    bool bSuccess = false;
    DWORD dwOldProtect = ChangeMemoryPermissions(dwAddress, sizeof(DWORD_PTR), PAGE_EXECUTE_READWRITE);

    auto breakpoint = std::find_if(m_lstBreakpoints.begin(), m_lstBreakpoints.end(), [=](const std::unique_ptr<Breakpoint> &breakpoint)
    {
        return breakpoint->Address() == dwAddress;
    });
    if (breakpoint != m_lstBreakpoints.end())
    {
        (void)(*breakpoint)->Disable();
        m_lstBreakpoints.remove(*breakpoint);
        bSuccess = true;
    }

    (void)ChangeMemoryPermissions(dwAddress, sizeof(DWORD_PTR), dwOldProtect);

    return bSuccess;
}

Breakpoint * Debugger::FindBreakpoint(const DWORD_PTR dwAddress)
{
    auto breakpoint = std::find_if(m_lstBreakpoints.begin(), m_lstBreakpoints.end(), [=](const std::unique_ptr<Breakpoint> &breakpoint)
    {
        return breakpoint->Address() == dwAddress;
    });
    if (breakpoint != m_lstBreakpoints.end())
    {
        return breakpoint->get();
    }
    if (m_pStepPoint->Address() == dwAddress)
    {
        return m_pStepPoint.get();
    }

    return nullptr;
}

const bool Debugger::AddBreakpoint(const char * const pSymbolName)
{
    auto symbol = m_pSymbols->FindSymbolByName(pSymbolName);
    if (symbol != nullptr)
    {
        return AddBreakpoint(symbol->dwAddress);
    }

    return false;
}

const bool Debugger::RemoveBreakpoint(const char * const pSymbolName)
{
    auto symbol = m_pSymbols->FindSymbolByName(pSymbolName);
    if (symbol != nullptr)
    {
        return RemoveBreakpoint(symbol->dwAddress);
    }

    return false;
}

const bool Debugger::StepInto()
{
    CONTEXT ctx = GetExecutingContext();
    ctx.EFlags |= 0x100;
    if (SetExecutingContext(ctx))
    {
        (void)Continue(true);
        return true;
    }

    return false;
}

const bool Debugger::StepOver()
{
    CONTEXT ctx = GetExecutingContext();
    bool bIsUnconditionalBranch = false;
#ifdef _M_IX86
    DWORD_PTR dwStepOverAddress = m_pDisassembler->GetNextInstruction(ctx.Eip, bIsUnconditionalBranch);
#elif defined _M_AMD64
    DWORD_PTR dwStepOverAddress = m_pDisassembler->GetNextInstruction(ctx.Rip, bIsUnconditionalBranch);
#else
#error "Unsupported platform"
#endif
    if (bIsUnconditionalBranch)
    {
        return StepInto();
    }
    else if (dwStepOverAddress != 0)
    {
        m_pStepPoint->Disable();
        m_pStepPoint->ChangeAddress(dwStepOverAddress);
        (void)m_pStepPoint->Enable();

        ctx.EFlags &= ~0x100;
        (void)SetExecutingContext(ctx);

        return Continue(true);
    }

    return false;
}

const bool Debugger::Continue()
{
    return Continue(false);
}

const bool Debugger::Continue(const bool bIsStepping)
{
    m_bIsStepping = bIsStepping;
    return BOOLIFY(SetEvent(m_hContinueEvent()));
}

void Debugger::PrintContext()
{
#ifdef _M_IX86
        fprintf(stderr, "EAX: %p EBX: %p ECX: %p EDX: %p\n"
            "ESP: %p EBP: %p ESI: %p EDI: %p\n"
            "EIP: %p FLAGS: %X\n",
            m_lastContext.Eax, m_lastContext.Ebx, m_lastContext.Ecx, m_lastContext.Edx,
            m_lastContext.Esp, m_lastContext.Ebp, m_lastContext.Esi, m_lastContext.Edi,
            m_lastContext.Eip, m_lastContext.EFlags);
#elif defined _M_AMD64
        fprintf(stderr, "RAX: %p RBX: %p RCX: %p RDX: %p\n"
            "RSP: %p RBP: %p RSI: %p RDI: %p\n"
            "R8: %p R9: %p R10: %p R11: %p\n"
            "R12: %p R13: %p R14: %p R15: %p\n"
            "RIP: %p FLAGS: %X\n",
            m_lastContext.Rax, m_lastContext.Rbx, m_lastContext.Rcx, m_lastContext.Rdx,
            m_lastContext.Rsp, m_lastContext.Rbp, m_lastContext.Rsi, m_lastContext.Rdi,
            m_lastContext.R8, m_lastContext.R9, m_lastContext.R10, m_lastContext.R11,
            m_lastContext.R12, m_lastContext.R13, m_lastContext.R14, m_lastContext.R15,
            m_lastContext.Rip, m_lastContext.EFlags);
#else
#error "Unsupported architecture"
#endif
    }

    void Debugger::PrintDisassembly(const DWORD_PTR dwAddress)
    {
        (void)m_pDisassembler->BytesAtAddress(dwAddress);
    }

    void Debugger::PrintCallStack()
    {
        STACKFRAME64 stackFrame = { 0 };
        const DWORD_PTR dwMaxFrames = 50;
        CONTEXT ctx = GetExecutingContext();

        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;

#ifdef _M_IX86
        DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = ctx.Eip;
        stackFrame.AddrFrame.Offset = ctx.Ebp;
        stackFrame.AddrStack.Offset = ctx.Esp;
#elif defined _M_AMD64
        DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = ctx.Rip;
        stackFrame.AddrFrame.Offset = ctx.Rbp;
        stackFrame.AddrStack.Offset = ctx.Rsp;
#else
#error "Unsupported platform"
#endif

        SafeHandle hThread = OpenCurrentThread();
        for (int i = 0; i < dwMaxFrames; ++i)
        {
            const bool bSuccess = BOOLIFY(StackWalk64(dwMachineType, m_hProcess(), hThread(), &stackFrame,
                (dwMachineType == IMAGE_FILE_MACHINE_I386 ? nullptr : &ctx), nullptr,
                SymFunctionTableAccess64, SymGetModuleBase64, nullptr));
            if (!bSuccess || stackFrame.AddrPC.Offset == 0)
            {
                fprintf(stderr, "StackWalk64 finished.\n");
                break;
            }

            fprintf(stderr, "Frame: %X\n"
                "Execution address: %p\n"
                "Stack address: %p\n"
                "Frame address: %p\n",
                i, stackFrame.AddrPC.Offset,
                stackFrame.AddrStack.Offset, stackFrame.AddrFrame.Offset);

            auto symbol = m_pSymbols->FindSymbolByAddress((DWORD_PTR)stackFrame.AddrPC.Offset);
            if (symbol == nullptr)
            {
                const SymbolInfo *pSymInfo = nullptr;
                const bool bSuccess = m_pSymbols->SymbolFromAddress(stackFrame.AddrPC.Offset, &pSymInfo);
                if (bSuccess && pSymInfo != nullptr)
                {
                    m_pSymbols->PrintSymbol(pSymInfo);
                }
            }
            else
            {
                m_pSymbols->PrintSymbol(symbol);
            }
        }
    }

    const DWORD Debugger::ChangeMemoryPermissions(const DWORD_PTR dwAddress, const size_t ulSize, DWORD dwNewPermissions)
    {
        DWORD dwOldProtect = 0;
        const bool bSuccess = BOOLIFY(VirtualProtectEx(m_hProcess(), (LPVOID)dwAddress, ulSize, dwNewPermissions, &dwOldProtect));
        if (!bSuccess)
        {
            fprintf(stderr, "Could not change memory permissions at address %X. Error = %X\n", dwAddress, GetLastError());
        }
        return dwOldProtect;
    }

    SafeHandle Debugger::OpenCurrentThread()
    {
        SafeHandle handle = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, m_dwExecutingThreadId);
        return handle;
    }

    const CONTEXT Debugger::GetExecutingContext()
    {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_ALL;
        SafeHandle hThread = OpenCurrentThread();
        if (hThread.IsValid())
        {
            bool bSuccess = BOOLIFY(GetThreadContext(hThread(), &ctx));
            if (!bSuccess)
            {
                fprintf(stderr, "Could not get context for thread %X. Error = %X\n", m_dwExecutingThreadId, GetLastError());
            }
        }

        memcpy(&m_lastContext, &ctx, sizeof(CONTEXT));

        return ctx;
    }

    const bool Debugger::SetExecutingContext(const CONTEXT &ctx)
    {
        bool bSuccess = false;
        SafeHandle hThread = OpenCurrentThread();
        if (hThread.IsValid())
        {
            bSuccess = BOOLIFY(SetThreadContext(hThread(), &ctx));
        }

        memcpy(&m_lastContext, &ctx, sizeof(CONTEXT));

        return bSuccess;
    }

const bool Debugger::ChangeByteAt(const DWORD_PTR dwAddress, const unsigned char cNewByte)
{
    SIZE_T ulBytesWritten = 0;
    const bool bSuccess = BOOLIFY(WriteProcessMemory(m_hProcess(), (LPVOID)dwAddress, &cNewByte, sizeof(unsigned char), &ulBytesWritten));
    if (bSuccess && ulBytesWritten == sizeof(unsigned char))
    {
        return true;
    }

    fprintf(stderr, "Could not change byte at %p. Error = %X\n", dwAddress, GetLastError());
    return false;
}

const bool Debugger::PrintBytesAt(const DWORD_PTR dwAddress, size_t ulNumBytes /*= 40*/)
{
    SIZE_T ulBytesRead = 0;
    std::unique_ptr<unsigned char[]> pBuffer = std::unique_ptr<unsigned char[]>(new unsigned char[ulNumBytes]);
    const bool bSuccess = BOOLIFY(ReadProcessMemory(m_hProcess(), (LPCVOID)dwAddress, pBuffer.get(), ulNumBytes, &ulBytesRead));
    if (bSuccess && ulBytesRead == ulNumBytes)
    {
        for (unsigned int i = 0; i < ulBytesRead; ++i)
        {
            fprintf(stderr, "%02X ", pBuffer.get()[i]);
        }
        fprintf(stderr, "\n");
        return true;
    }
    
    fprintf(stderr, "Could not read memory at %p. Error = %X\n", dwAddress, GetLastError());
    return false;

}

const bool Debugger::WaitForContinue() const
{
    return (WaitForSingleObject(m_hContinueEvent(), INFINITE) == WAIT_OBJECT_0);
}

const HANDLE Debugger::Handle() const
{
    return m_hProcess();
}

const Symbols * const Debugger::ProcessSymbols() const
{
    return m_pSymbols.get();
}

}