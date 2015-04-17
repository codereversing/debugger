#pragma once

#include <Windows.h>

namespace CodeReversing
{

class Breakpoint
{
public:
    enum class eType
    {
        eHardware = 1,
        eSoftware = 2,
        eInterrupt = 3
    };

    Breakpoint() = delete;
    Breakpoint(const HANDLE hProcess, const DWORD_PTR dwAddress, const eType eBreakpointType);

    Breakpoint(const Breakpoint &copy) = delete;
    Breakpoint &operator=(const Breakpoint &copy) = delete;

    virtual ~Breakpoint() = default;

    const eType Type() const;
    const DWORD_PTR Address() const;
    const bool IsEnabled() const;

    const bool Enable();
    const bool Disable();

    bool operator==(const Breakpoint &rhs) const;
    bool operator!=(const Breakpoint &rhs) const;

private:
    enum class eState
    {
        eDisabled = 0,
        eEnabled = 1
    };

    virtual const bool EnableBreakpoint() = 0;
    virtual const bool DisableBreakpoint() = 0;

    eType m_eType;
    eState m_eState;

protected:
    HANDLE m_hProcess;
    DWORD_PTR m_dwAddress;

};

}

