#include "DebugEventHandler.h"

#include <memory>

#include "Common.h"

namespace CodeReversing
{

DebugEventHandler::DebugEventHandler()
{
    m_pExceptionHandler = std::unique_ptr<DebugExceptionHandler>(new DebugExceptionHandler(m_hProcess));
    Initialize();
}

void DebugEventHandler::Initialize()
{
    Register(DebugEvents::eCreateThread, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &info = dbgEvent.u.CreateThread;
        fprintf(stderr, "CREATE_THREAD_DEBUG_EVENT received.\n"
            "Handle: %X\n"
            "TLS base: %X\n"
            "Start address: %X\n",
            info.hThread, info.lpThreadLocalBase, info.lpStartAddress);
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eCreateProcess, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &info = dbgEvent.u.CreateProcessInfo;
        fprintf(stderr, "CREATE_PROCESS_DEBUG_EVENT received.\n"
            "Handle (image file): %X\n"
            "Handle (process): %X\n"
            "Handle (main thread): %X\n"
            "Image base address: %X\n"
            "Debug info file offset: %X\n"
            "Debug info size: %X\n"
            "TLS base: %X\n"
            "Start address: %X\n",
            info.hFile, info.hProcess, info.hThread, info.lpBaseOfImage,
            info.dwDebugInfoFileOffset, info.nDebugInfoSize, info.lpThreadLocalBase,
            info.lpStartAddress);

        m_hProcess = info.hProcess;

        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eExitThread, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "EXIT_THREAD_DEBUG_EVENT received.\n"
            "Thread %X exited with code %X.\n",
            dbgEvent.dwThreadId, dbgEvent.u.ExitThread.dwExitCode);
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eExitProcess, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "EXIT_PROCESS_DEBUG_EVENT received.\n"
            "Process %X exited with code %X.\n",
            dbgEvent.dwProcessId, dbgEvent.u.ExitProcess.dwExitCode);
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eLoadDll, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &info = dbgEvent.u.LoadDll;
        fprintf(stderr, "LOAD_DLL_DEBUG_EVENT received.\n"
            "Handle: %X\n"
            "Base address: %X\n"
            "Debug info file offset: %X\n"
            "Debug info size: %X\n",
            info.hFile, info.lpBaseOfDll, info.dwDebugInfoFileOffset,
            info.nDebugInfoSize);
        char strName[MAX_PATH] = { 0 };
        (void)GetFinalPathNameByHandleA(info.hFile, strName, sizeof(strName), FILE_NAME_NORMALIZED);
        fprintf(stderr, "Name: %s\n", strName);

        m_dwContinueStatus = DBG_CONTINUE;
    });

    Register(DebugEvents::eUnloadDll, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "UNLOAD_DLL_DEBUG_EVENT received.\n"
            "Dll at %X has unloaded.\n", dbgEvent.u.UnloadDll.lpBaseOfDll);
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eDebugString, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &info = dbgEvent.u.DebugString;
        fprintf(stderr, "OUTPUT_DEBUG_STRING_EVENT received.\n");
        std::unique_ptr<char[]> pBuffer = std::unique_ptr<char[]>(new char[info.nDebugStringLength]);
        DWORD dwBytesRead = 0;

        const bool bSuccess = BOOLIFY(ReadProcessMemory(m_hProcess, info.lpDebugStringData, pBuffer.get(), info.nDebugStringLength, &dwBytesRead));
        if (bSuccess)
        {
            if (info.fUnicode)
            {
                fwprintf(stderr, L"Debug string: %s\n", (wchar_t *)pBuffer.get());
            }
            else
            {
                fprintf(stderr, "Debug string: %s\n", pBuffer.get());
            }
        }
        else
        {
            fprintf(stderr, "Could not read debug string. Error = %X\n", GetLastError());
        }

        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eRipEvent, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received RIP event.\n");
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugEvents::eException, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &exception = dbgEvent.u.Exception;
        fprintf(stderr, "Received exception event.\n"
            "First chance exception: %X\n"
            "Exception code: %X\n"
            "Exception flags: %X\n"
            "Exception address: %X\n"
            "Number parameters (associated with exception): %X\n",
            exception.dwFirstChance, exception.ExceptionRecord.ExceptionCode,
            exception.ExceptionRecord.ExceptionFlags, exception.ExceptionRecord.ExceptionAddress,
            exception.ExceptionRecord.NumberParameters);

        m_pExceptionHandler->Notify((DebugExceptions)dbgEvent.u.Exception.ExceptionRecord.ExceptionCode, dbgEvent);
        SetContinueStatus(m_pExceptionHandler->ContinueStatus());
    });
}

const DWORD DebugEventHandler::ContinueStatus() const
{
    return m_dwContinueStatus;
}

void DebugEventHandler::SetContinueStatus(const DWORD dwContinueStatus)
{
    m_dwContinueStatus = dwContinueStatus;
}

}