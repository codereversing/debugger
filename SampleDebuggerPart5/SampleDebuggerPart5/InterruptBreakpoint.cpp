#include "InterruptBreakpoint.h"

#include <cstdio>

#include "Common.h"

namespace CodeReversing
{

InterruptBreakpoint::InterruptBreakpoint(const HANDLE hProcess, const DWORD_PTR dwAddress)
    : Breakpoint(hProcess, dwAddress, Breakpoint::eType::eInterrupt),
    m_originalByte{ 0 }
{
}

const bool InterruptBreakpoint::EnableBreakpoint()
{
    SIZE_T ulBytes = 0;
    bool bSuccess = BOOLIFY(ReadProcessMemory(m_hProcess, (LPCVOID)m_dwAddress, &m_originalByte, sizeof(unsigned char), &ulBytes));
    if (bSuccess && ulBytes == sizeof(unsigned char))
    {
        bSuccess = BOOLIFY(WriteProcessMemory(m_hProcess, (LPVOID)m_dwAddress, &m_breakpointOpcode, sizeof(unsigned char), &ulBytes));
        return bSuccess && (ulBytes == sizeof(unsigned char));
    }
    else
    {
        fprintf(stderr, "Could not read from address %p. Error = %X\n", m_dwAddress, GetLastError());
    }

    return false;
}

const bool InterruptBreakpoint::DisableBreakpoint()
{
    SIZE_T ulBytes = 0;
    const bool bSuccess = BOOLIFY(WriteProcessMemory(m_hProcess, (LPVOID)m_dwAddress, &m_originalByte, sizeof(unsigned char), &ulBytes));
    if (bSuccess && ulBytes == sizeof(unsigned char))
    {
        return true;
    }
    fprintf(stderr, "Could not write back original opcode to address %p. Error = %X\n", m_dwAddress, GetLastError());

    return false;
}

void InterruptBreakpoint::ChangeAddress(const DWORD_PTR dwNewAddress)
{
    m_dwAddress = dwNewAddress;
}

}