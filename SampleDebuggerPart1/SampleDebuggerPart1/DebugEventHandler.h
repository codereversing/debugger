#pragma once

#include <list>
#include <memory>

#include <Windows.h>

#include "Observable.h"
#include "DebugExceptionHandler.h"

namespace CodeReversing
{

enum DebugEvents
{
    //General events
    eException = EXCEPTION_DEBUG_EVENT,
    eCreateThread = CREATE_THREAD_DEBUG_EVENT,
    eCreateProcess = CREATE_PROCESS_DEBUG_EVENT,
    eExitThread = EXIT_THREAD_DEBUG_EVENT,
    eExitProcess = EXIT_PROCESS_DEBUG_EVENT,
    eLoadDll = LOAD_DLL_DEBUG_EVENT,
    eUnloadDll = UNLOAD_DLL_DEBUG_EVENT,
    eDebugString = OUTPUT_DEBUG_STRING_EVENT,
    eRipEvent = RIP_EVENT,
};

class DebugEventHandler final : public Observable<DebugEvents, void, const DEBUG_EVENT &>
{
public:
    DebugEventHandler();

    DebugEventHandler(const DebugEventHandler &copy) = delete;
    DebugEventHandler &operator=(const DebugEventHandler &copy) = delete;

    ~DebugEventHandler() = default;

    const DWORD ContinueStatus() const;

private:
    void Initialize();
    void SetContinueStatus(const DWORD dwContinueStatus);

    DWORD m_dwContinueStatus;
    HANDLE m_hProcess;
    std::unique_ptr<DebugExceptionHandler> m_pExceptionHandler;
};

}