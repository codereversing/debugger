#pragma once

#include <functional>
#include <map>
#include <memory>
#include <thread>

#include <Windows.h>

#include "DebugEventHandler.h"

namespace CodeReversing
{

class Debugger final
{
public:
    Debugger() = delete;
    Debugger(const DWORD dwProcessId, const bool bKillOnExit = false);

    Debugger(const Debugger &copy) = delete;
    Debugger &operator=(const Debugger &copy) = delete;

    ~Debugger() = default;

    const bool Start();
    const bool Stop();

    const bool IsActive() const;

private:
    bool m_bIsActive;
    bool m_bKillOnExit;
    DWORD m_dwProcessId;
    HANDLE m_hProcess;

    const bool DebuggerLoop();

    std::unique_ptr<DebugEventHandler> m_pEventHandler;
};

}