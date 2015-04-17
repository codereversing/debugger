#pragma once

#include <Windows.h>
#include "Breakpoint.h"

namespace CodeReversing
{

class InterruptBreakpoint final : public Breakpoint
{
public:
    InterruptBreakpoint() = delete;
    InterruptBreakpoint(const HANDLE hProcess, const DWORD_PTR dwAddress);

    InterruptBreakpoint(const InterruptBreakpoint &copy) = delete;
    InterruptBreakpoint &operator=(const InterruptBreakpoint &copy) = delete;

    ~InterruptBreakpoint() = default;

    const bool EnableBreakpoint();
    const bool DisableBreakpoint();

private:
    const static unsigned char m_breakpointOpcode = 0xCC;
    unsigned char m_originalByte;

};

}