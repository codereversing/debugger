#pragma once

#include "BeaEngine.h"

#include <array>

#include <Windows.h>

namespace CodeReversing
{
typedef int(__stdcall *pDisasm)(LPDISASM pDisAsm);

class Disassembler final
{ 
public:
    Disassembler() = delete;

    Disassembler(HANDLE hProcess);

    Disassembler(const Disassembler &copy) = delete;
    Disassembler &operator=(const Disassembler &copy) = delete;

    ~Disassembler();

    const bool BytesAtAddress(DWORD_PTR dwAddress, size_t ulInstructionsToDisassemble = 15);
    DWORD_PTR GetNextInstruction(const DWORD_PTR dwAddress, bool &bIsUnconditionalBranch);

private:
    static HMODULE m_hDll;
    static pDisasm m_pDisasm;

    static const bool IsInitialized();

    void SetDisassembler(const DWORD_PTR dwAddress);
    const bool TransferBytes(const DWORD_PTR dwAddress);

    HANDLE m_hProcess;
    DISASM m_disassembler;

    DWORD_PTR m_dwStartAddress;
    std::array<char, 4096> m_bytes;
};

}