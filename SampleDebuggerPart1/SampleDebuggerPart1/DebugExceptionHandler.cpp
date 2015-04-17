#include "DebugExceptionHandler.h"

namespace CodeReversing
{

DebugExceptionHandler::DebugExceptionHandler(const HANDLE &hProcess) : m_hProcess{ hProcess }
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
        fprintf(stderr, "Received breakpoint\n");
        SetContinueStatus(DBG_CONTINUE);
    });

    Register(DebugExceptions::eSingleStep, [&](const DEBUG_EVENT &dbgEvent)
    {
        fprintf(stderr, "Received single step\n");
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