#include "DebugExceptionHandler.h"

#include "Debugger.h"

namespace CodeReversing
{

DebugExceptionHandler::DebugExceptionHandler(Debugger *pDebugger) : m_pDebugger{ pDebugger }
{
    Initialize();
}

void DebugExceptionHandler::Initialize()
{
    Register(DebugExceptions::eAccessViolation, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received access violation\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eDataTypeMisalignment, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received datatype misalignment\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eBreakpoint, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &exceptionRecord = dbgEvent.u.Exception.ExceptionRecord;
        const DWORD_PTR dwExceptionAddress = (DWORD_PTR)exceptionRecord.ExceptionAddress;
        fprintf(stderr, "Received breakpoint at address %p.\n", dwExceptionAddress);  

        Breakpoint *pBreakpoint = m_pDebugger->FindBreakpoint(dwExceptionAddress);
        if (pBreakpoint != nullptr)
        {
            if (pBreakpoint->Disable())
            {
                m_pDebugger->m_dwExecutingThreadId = dbgEvent.dwThreadId;
                CONTEXT ctx = m_pDebugger->GetExecutingContext();
#ifdef _M_IX86
                ctx.Eip = (DWORD_PTR)dwExceptionAddress;
#elif defined _M_AMD64
                ctx.Rip = (DWORD_PTR)dwExceptionAddress;
#else
#error "Unsupported architecture"
#endif
                ctx.EFlags |= 0x100;
                m_pDebugger->m_pLastBreakpoint = pBreakpoint;
                m_pDebugger->m_dwExecutingThreadId = dbgEvent.dwThreadId;
                if (m_pDebugger->SetExecutingContext(ctx))
                {
                    fprintf(stderr, "Press c to continue or s to begin stepping.\n");
                    (void)m_pDebugger->WaitForContinue();
                }
            }
            else
            {
                fprintf(stderr, "Could not remove breakpoint at address %p.", dwExceptionAddress);
            }
        }
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugExceptions::eSingleStep, [&](const DEBUG_EVENT &dbgEvent)
    {
        auto &exceptionRecord = dbgEvent.u.Exception.ExceptionRecord;
        const DWORD_PTR dwExceptionAddress = (DWORD_PTR)exceptionRecord.ExceptionAddress;
        fprintf(stderr, "Received single step at address %p\n", dwExceptionAddress);
        if (m_pDebugger->m_bIsStepping)
        {
            fprintf(stderr, "Press s to continue stepping.\n");
            m_pDebugger->m_dwExecutingThreadId = dbgEvent.dwThreadId;
            CONTEXT ctx = m_pDebugger->GetExecutingContext();
            if (m_pDebugger->SetExecutingContext(ctx))
            {
                (void)m_pDebugger->WaitForContinue();
            }
        }
        if (!m_pDebugger->m_pLastBreakpoint->IsEnabled())
        {
            (void)m_pDebugger->m_pLastBreakpoint->Enable();
        }

        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugExceptions::eArrayBoundsExceeded, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received array bounds exceeded\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltDenormal, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point denormal\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltDivideByZero, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point divide by zero\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltInexactResult, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point inexact result\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltInvalidOperation, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point invalid operation\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltOverflow, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point overflow\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltStackCheck, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point stack check\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eFltUnderflow, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received floating point underflow\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eIntDivideByZero, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received integer divide by zero\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eIntOverflow, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received integer overflow\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::ePrivilegedInstruction, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received privileged instruction\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::ePageError, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received page error\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eIllegalInstruction, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received illegal instruction\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eNoncontinuableException, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received non-continuable exception\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eStackOverflow, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received stack overflow\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eInvalidDisposition, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received invalid disposition\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eGuardPage, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received guard page\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });

    Register(DebugExceptions::eInvalidHandle, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received invalid handle\n");
        SetContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    });
}

const DWORD DebugExceptionHandler::ContinueStatus() const
{
    return m_dwContinueStatus;
}

void DebugExceptionHandler::SetContinueStatus(const DWORD dwContinueStatus)
{
    m_dwContinueStatus = dwContinueStatus;
}

}