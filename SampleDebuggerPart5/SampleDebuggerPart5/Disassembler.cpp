#include "Disassembler.h"

#include <cstdio>
#include <cmath>

#include "Common.h"

namespace CodeReversing
{

HMODULE Disassembler::m_hDll = nullptr;
pDisasm Disassembler::m_pDisasm = nullptr;

Disassembler::Disassembler(HANDLE hProcess) : m_hProcess{ hProcess }
{
    memset(&m_disassembler, 0, sizeof(DISASM));
#ifdef _M_IX86
    m_disassembler.Archi = 0;
    if (m_hDll == nullptr)
    {
        m_hDll = LoadLibrary(L"BeaEngine_x86.dll");
        m_pDisasm = (pDisasm)GetProcAddress(m_hDll, "_Disasm@4");
    }
#elif defined _M_AMD64
    m_disassembler.Archi = 64;
    if(m_hDll == nullptr)
    {
        m_hDll = LoadLibrary(L"BeaEngine_x64.dll");
        m_pDisasm = (pDisasm)GetProcAddress(m_hDll, "Disasm");
    }
#else
#error "Unsupported architecture"
#endif
}

Disassembler::~Disassembler()
{
    if (m_hDll != nullptr)
    {
        FreeLibrary(m_hDll);
    }
}

const bool Disassembler::BytesAtAddress(DWORD_PTR dwAddress, size_t ulInstructionsToDisassemble /*= 15*/)
{
    if (IsInitialized())
    {
        SetDisassembler(dwAddress);
        bool bFailed = false;
        while (!bFailed && ulInstructionsToDisassemble-- > 0)
        {
            int iDisasmLength = m_pDisasm(&m_disassembler);
            if (iDisasmLength != UNKNOWN_OPCODE)
            {
                fprintf(stderr, "0x%p - %s\n", dwAddress, m_disassembler.CompleteInstr);
                m_disassembler.EIP += iDisasmLength;
                dwAddress += iDisasmLength;
            }
            else
            {
                fprintf(stderr, "Error: Reached unknown opcode in disassembly.\n");
                bFailed = true;
            }
        }
    }
    else
    {
        fprintf(stderr, "Could not show disassembly at address. Disassembler Dll was not loaded properly.\n");
        return false;
    }

    return true;
}

DWORD_PTR Disassembler::GetNextInstruction(const DWORD_PTR dwAddress, bool &bIsUnconditionalBranch)
{
    DWORD_PTR dwNextAddress = 0;
    if (IsInitialized())
    {
        SetDisassembler(dwAddress);
        int iDisasmLength = m_pDisasm(&m_disassembler);
        if (iDisasmLength != UNKNOWN_OPCODE)
        {
            if (m_disassembler.Instruction.BranchType == RetType || m_disassembler.Instruction.BranchType == JmpType)
            {
                bIsUnconditionalBranch = true;
            }
            else
            {
                dwNextAddress = (dwAddress + iDisasmLength);
            }
        }
        else
        {
            fprintf(stderr, "Could not get next instruction. Unknown opcode at %p.\n");
        }
    }
    else
    {
        fprintf(stderr, "Could not get next instruction. Disassembler Dll was not loaded propertly.\n");
    }

    return dwNextAddress;
}

void Disassembler::SetDisassembler(const DWORD_PTR dwAddress)
{
    bool bIsCached = ((dwAddress - m_dwStartAddress) < m_bytes.size());
    bIsCached &= (dwAddress < m_dwStartAddress);
    if (!bIsCached)
    {
        (void)TransferBytes(dwAddress);
        m_disassembler.EIP = (UIntPtr)m_bytes.data();
        m_dwStartAddress = dwAddress;
    }
    else
    {
        m_disassembler.EIP = (UIntPtr)&m_bytes.data()[dwAddress - m_dwStartAddress];
    }
}

const bool Disassembler::TransferBytes(const DWORD_PTR dwAddress)
{
    SIZE_T ulBytesRead = 0;
    bool bSuccess = BOOLIFY(ReadProcessMemory(m_hProcess, (LPCVOID)dwAddress, m_bytes.data(), m_bytes.size(), &ulBytesRead));
    if (bSuccess && ulBytesRead == m_bytes.size())
    {
        return true;
    }
    else
    {
        fprintf(stderr, "Could not read from %p. Error = %X\n", dwAddress, GetLastError());
    }

    return false;
}

const bool Disassembler::IsInitialized()
{
    return (m_hDll != nullptr) && (m_pDisasm != nullptr);
}

}