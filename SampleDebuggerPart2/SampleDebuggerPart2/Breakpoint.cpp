#include "Breakpoint.h"

namespace CodeReversing
{

Breakpoint::Breakpoint(const HANDLE hProcess, const DWORD_PTR dwAddress, const eType eBreakpointType)
    : m_hProcess{ hProcess }, m_dwAddress{ dwAddress }, m_eType{ eBreakpointType },
    m_eState{Breakpoint::eState::eDisabled}
{
}

const bool Breakpoint::Enable()
{
    if (EnableBreakpoint())
    {
        m_eState = Breakpoint::eState::eEnabled;
        return true;
    }
    return false;
}

const bool Breakpoint::Disable()
{
    if (DisableBreakpoint())
    {
        m_eState = Breakpoint::eState::eDisabled;
        return true;
    }
    return false;
}

const Breakpoint::eType Breakpoint::Type() const
{
    return m_eType;
}

const DWORD_PTR Breakpoint::Address() const
{
    return m_dwAddress;
}

const bool Breakpoint::IsEnabled() const
{
    return m_eState == Breakpoint::eState::eEnabled;
}

bool Breakpoint::operator==(const Breakpoint &rhs) const
{
    return (m_eType == rhs.m_eType) && (m_dwAddress == rhs.m_dwAddress);
}

}