#include "Debugger.h"

#include <algorithm>
#include <cstdio>

#include "Common.h"
#include "InterruptBreakpoint.h"

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
    (void)CloseHandle(m_hContinueEvent);
    (void)CloseHandle(m_hProcess);
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

    std::unique_ptr<InterruptBreakpoint> pNewBreakpoint(new InterruptBreakpoint(m_hProcess, dwAddress));
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
    
    return breakpoint == m_lstBreakpoints.end() ? nullptr : breakpoint->get();
}

const bool Debugger::StepInto()
{
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_ALL;
    HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, m_dwExecutingThreadId);
    if (hThread != NULL)
    {
        bool bSuccess = BOOLIFY(GetThreadContext(hThread, &ctx));
        ctx.EFlags |= 0x100;
        bSuccess &= BOOLIFY(SetThreadContext(hThread, &ctx));
        CloseHandle(hThread);
        (void)Continue(true);

        return bSuccess;
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
    return BOOLIFY(SetEvent(m_hContinueEvent));
}

const DWORD Debugger::ChangeMemoryPermissions(const DWORD_PTR dwAddress, const size_t ulSize, DWORD dwNewPermissions)
{
    DWORD dwOldProtect = 0;
    const bool bSuccess = BOOLIFY(VirtualProtectEx(m_hProcess, (LPVOID)dwAddress, ulSize, dwNewPermissions, &dwOldProtect));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not change memory permissions at address %X. Error = %X\n", dwAddress, GetLastError());
    }
    return dwOldProtect;
}

const bool Debugger::WaitForContinue() const
{
    return (WaitForSingleObject(m_hContinueEvent, INFINITE) == WAIT_OBJECT_0);
}

const HANDLE Debugger::Handle() const
{
    return m_hProcess;
}

}