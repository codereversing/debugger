#pragma once

#include <Windows.h>

#include "Observable.h"

namespace CodeReversing
{

class Debugger;

enum DebugExceptions
{                     
    eAccessViolation = EXCEPTION_ACCESS_VIOLATION,
    eDataTypeMisalignment = EXCEPTION_DATATYPE_MISALIGNMENT,
    eBreakpoint = EXCEPTION_BREAKPOINT,
    eSingleStep = EXCEPTION_SINGLE_STEP,
    eArrayBoundsExceeded = EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
    eFltDenormal = EXCEPTION_FLT_DENORMAL_OPERAND,
    eFltDivideByZero = EXCEPTION_FLT_DIVIDE_BY_ZERO,
    eFltInexactResult = EXCEPTION_FLT_INEXACT_RESULT,
    eFltInvalidOperation = EXCEPTION_FLT_INVALID_OPERATION,
    eFltOverflow = EXCEPTION_FLT_OVERFLOW,
    eFltStackCheck = EXCEPTION_FLT_STACK_CHECK,
    eFltUnderflow = EXCEPTION_FLT_UNDERFLOW,
    eIntDivideByZero = EXCEPTION_INT_DIVIDE_BY_ZERO,
    eIntOverflow = EXCEPTION_INT_OVERFLOW,
    ePrivilegedInstruction = EXCEPTION_PRIV_INSTRUCTION,
    ePageError = EXCEPTION_IN_PAGE_ERROR,
    eIllegalInstruction = EXCEPTION_ILLEGAL_INSTRUCTION,
    eNoncontinuableException = EXCEPTION_NONCONTINUABLE_EXCEPTION,
    eStackOverflow = EXCEPTION_STACK_OVERFLOW,
    eInvalidDisposition = EXCEPTION_INVALID_DISPOSITION,
    eGuardPage = EXCEPTION_GUARD_PAGE,
    eInvalidHandle = EXCEPTION_INVALID_HANDLE,
};

class DebugExceptionHandler final : public Observable<DebugExceptions, void, const DEBUG_EVENT &>
{
public:
    DebugExceptionHandler() = delete;
    DebugExceptionHandler(Debugger *pDebugger);

    ~DebugExceptionHandler() = default;

    const DWORD ContinueStatus() const;

private:
    void Initialize();
    void SetContinueStatus(const DWORD dwContinueStatus);

    Debugger * const m_pDebugger;
    DWORD m_dwContinueStatus;
};

}