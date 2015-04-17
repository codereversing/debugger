#include "Debugger.h"

#include <cstdio>

#include "Common.h"

namespace CodeReversing
{

Debugger::Debugger(const DWORD dwProcessId, const bool bKillOnExit /*= false*/) : m_bIsActive{ false },
    m_dwProcessId{ dwProcessId }, m_bKillOnExit{ bKillOnExit }, m_hProcess{ INVALID_HANDLE_VALUE }
{
    m_pEventHandler = std::unique_ptr<DebugEventHandler>(new DebugEventHandler);
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

const bool Debugger::IsActive() const
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

        fprintf(stderr, "Debug event raised in process %X -- thread %X.\n", dbgEvent.dwProcessId, dbgEvent.dwThreadId);

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

}