#pragma comment(lib, "Dbghelp.lib")

#include "Symbols.h"

#include <cstdio>

#include "Common.h"

namespace CodeReversing
{

Symbols::Symbols(const HANDLE hProcess, const HANDLE hFile, const bool bLoadAll /*= false*/)
    : m_hProcess{ hProcess }, m_hFile{ hFile }
{
    (void)SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_DEFERRED_LOADS |
        SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    const bool bSuccess = BOOLIFY(SymInitialize(hProcess, nullptr, bLoadAll));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not initialize symbol handler. Error = %X.\n",
            GetLastError());
    }
}

Symbols::~Symbols()
{
    const bool bSuccess = BOOLIFY(SymCleanup(m_hProcess));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not terminate symbol handler. Error = %X.\n",
            GetLastError());
    }
}

BOOL CALLBACK Symbols::SymEnumCallback(PCSTR strModuleName, DWORD64 dwBaseOfDll, PVOID pUserContext)
{
    fprintf(stderr, "Symbols exist for module: %s. Base address of module: %p\n",
        strModuleName, (DWORD_PTR)dwBaseOfDll);
    return TRUE;
}

const bool Symbols::EnumerateAllModulesWithSymbols()
{
    const bool bSuccess = BOOLIFY(SymEnumerateModules64(m_hProcess, SymEnumCallback, this));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not enumerate symbols. Error = %X.\n",
            GetLastError());
    }

    return bSuccess;
}

BOOL CALLBACK Symbols::SymEnumCallback(PSYMBOL_INFO pSymInfo, ULONG ulSymbolSize, PVOID pUserContext)
{
    UserContext *pContext = (UserContext *)pUserContext;
    Symbols *pThisPtr = (Symbols *)pContext->pThis;
    DWORD_PTR dwAddress = (DWORD_PTR)pSymInfo->Address;
    DWORD_PTR dwModBase = (DWORD_PTR)pSymInfo->ModBase;

    //fprintf(stderr, "Symbol found at %p. Name: %.*s. Base address of module: %p\n",
    //    dwAddress, pSymInfo->NameLen, pSymInfo->Name, dwModBase);

    bool bSuccess = false;
    SymbolInfo symbolInfo;
    symbolInfo.dwAddress = dwAddress;
    symbolInfo.strName = std::move(std::vector<char>(pSymInfo->Name, pSymInfo->Name + strlen(pSymInfo->Name)));
    symbolInfo.strName.emplace_back(0);

    IMAGEHLP_LINE64 lineInfo = pThisPtr->GetSymbolLineInfo(dwAddress, symbolInfo.dwDisplacement, bSuccess);
    if (bSuccess)
    {
        symbolInfo.dwLineNumber = lineInfo.LineNumber;
        symbolInfo.strSourceFile = std::move(std::vector<char>(lineInfo.FileName, lineInfo.FileName + strlen(lineInfo.FileName)));
        symbolInfo.strSourceFile.emplace_back(0);
    }

    ModuleSymbolInfo moduleSymbol;
    moduleSymbol.dwModuleBaseAddress = dwModBase;
    moduleSymbol.strName = std::move(std::vector<char>(pContext->pName, pContext->pName + strlen(pContext->pName)));
    moduleSymbol.strName.emplace_back(0);
    moduleSymbol.symbolInfo = std::move(symbolInfo);

    pThisPtr->m_mapSymbols.insert(std::make_pair(dwModBase, std::move(moduleSymbol)));

    return TRUE;
}

const bool Symbols::EnumerateModuleSymbols(const char * const pModulePath, const DWORD64 dwBaseAddress)
{
    DWORD64 dwBaseOfDll = SymLoadModuleEx(m_hProcess, m_hFile, pModulePath, nullptr,
        dwBaseAddress, 0, nullptr, 0);
    if (dwBaseOfDll == 0)
    {
        fprintf(stderr, "Could not load modules for %s. Error = %X.\n",
            pModulePath, GetLastError());
        return false;
    }

    UserContext userContext = { this, pModulePath };
    const bool bSuccess = 
       BOOLIFY(SymEnumSymbols(m_hProcess, dwBaseOfDll, "*!*", SymEnumCallback, &userContext));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not enumerate symbols for %s. Error = %X.\n",
            pModulePath, GetLastError());
    }

    return bSuccess;
}

const bool Symbols::SymbolFromAddress(const DWORD64 dwAddress, const SymbolInfo **pFullSymbolInfo)
{
    char pBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)] = { 0 };
    PSYMBOL_INFO pSymInfo = (PSYMBOL_INFO)pBuffer;

    pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymInfo->MaxNameLen = MAX_SYM_NAME;

    DWORD64 dwDisplacement = 0;
    const bool bSuccess = BOOLIFY(SymFromAddr(m_hProcess, dwAddress, &dwDisplacement, pSymInfo));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not retrieve symbol from address %p. Error = %X.\n",
            (DWORD_PTR)dwAddress, GetLastError());
        return false;
    }

    fprintf(stderr, "Symbol found at %p. Name: %.*s. Base address of module: %p\n",
        (DWORD_PTR)dwAddress, pSymInfo->NameLen, pSymInfo->Name, (DWORD_PTR)pSymInfo->ModBase);

    *pFullSymbolInfo = FindSymbolByName(pSymInfo->Name);

    return bSuccess;
}

const bool Symbols::SymbolFromName(const char * const pName, const SymbolInfo **pFullSymbolInfo)
{
    char pBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)
        + sizeof(ULONG64) - 1 / sizeof(ULONG64)] = { 0 };
    PSYMBOL_INFO pSymInfo = (PSYMBOL_INFO)pBuffer;
    
    pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymInfo->MaxNameLen = MAX_SYM_NAME;

    const bool bSuccess = BOOLIFY(SymFromName(m_hProcess, pName, pSymInfo));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not retrieve symbol for name %s. Error = %X.\n",
            pName, GetLastError());
        return false;
    }

    fprintf(stderr, "Symbol found for %s. Name: %.*s. Address: %p. Base address of module: %p\n",
        pName, pSymInfo->NameLen, pSymInfo->Name, (DWORD_PTR)pSymInfo->Address,
        (DWORD_PTR)pSymInfo->ModBase);

    *pFullSymbolInfo = FindSymbolByAddress((DWORD_PTR)pSymInfo->Address);

    return bSuccess;
}

const bool Symbols::SymbolLineFromAddress(const DWORD64 dwAddress)
{
    DWORD dwDisplacement = 0;
    bool bSuccess = false;
    IMAGEHLP_LINE64 lineInfo = GetSymbolLineInfo(dwAddress, dwDisplacement, bSuccess);
    if (bSuccess)
    {
        fprintf(stderr, "Address: %p.\n"
            "Displacement: 0x%X\n"
            "File name: %s\n"
            "Line number: %i\n",
            (DWORD_PTR)dwAddress, dwDisplacement, lineInfo.FileName, lineInfo.LineNumber);
    }

    return bSuccess;
}

const bool Symbols::SymbolAddressFromLine(const char * const pName, const char * const pFileName,
    const DWORD dwLineNumber)
{
    LONG lDisplacement = 0;
    bool bSuccess = false;
    IMAGEHLP_LINE64 lineInfo = GetSymbolLineInfo(pName, pFileName, dwLineNumber, lDisplacement, bSuccess);
    if (bSuccess)
    {
        fprintf(stderr, "Address: %p\n"
            "Displacement: 0x%X\n"
            "File name: %s\n"
            "Line number: %i\n",
            (DWORD_PTR)lineInfo.Address, lDisplacement, lineInfo.FileName, lineInfo.LineNumber);
    }

    return bSuccess;
}

BOOL CALLBACK Symbols::SymEnumSourceFilesCallback(PSOURCEFILE pSourceFile, PVOID pUserContext)
{
    fprintf(stderr, "Module base address: %p -- Source file: %s.\n",
        (DWORD_PTR)pSourceFile->ModBase, pSourceFile->FileName);
    return TRUE;
}

const bool Symbols::ListSourceFiles()
{
    const bool bSuccess = BOOLIFY(SymEnumSourceFiles(m_hProcess, 0, nullptr,
        SymEnumSourceFilesCallback, nullptr));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not enumerate source files for process. Error = %X\n", GetLastError());
    }

    return bSuccess;
}

BOOL CALLBACK Symbols::SymEnumLinesCallback(PSRCCODEINFO pLineInfo, PVOID pUserContext)
{
    fprintf(stderr, "Module base address: %p -- File name: %s\n"
        "Line number: %i -- Virtual address: %p",
        (DWORD_PTR)pLineInfo->ModBase, pLineInfo->FileName, pLineInfo->LineNumber,
        (DWORD_PTR)pLineInfo->Address);
    return TRUE;
}

const bool Symbols::DumpSourceFileInfo(const DWORD64 dwBaseAddress, const char * const pFilePath)
{
    const bool bSuccess = BOOLIFY(SymEnumLines(m_hProcess, dwBaseAddress, nullptr, pFilePath,
        SymEnumLinesCallback, nullptr));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not dump source file %s with base address %p.\n",
            pFilePath, dwBaseAddress);
    }

    return bSuccess;
}

const IMAGEHLP_LINE64 Symbols::GetSymbolLineInfo(const DWORD64 dwAddress, DWORD &dwDisplacement, bool &bSuccess)
{
    dwDisplacement = 0;
    IMAGEHLP_LINE64 lineInfo = { 0 };
    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    bSuccess = BOOLIFY(SymGetLineFromAddr64(m_hProcess, dwAddress, &dwDisplacement, &lineInfo));
    if (!bSuccess)
    {
        //fprintf(stderr, "Could not get line from address %p. Error = %X.\n",
        //    (DWORD_PTR)dwAddress, GetLastError());
    }

    return lineInfo;
}

const IMAGEHLP_LINE64 Symbols::GetSymbolLineInfo(const char * const pName, const char * const pFileName,
    const DWORD dwLineNumber, LONG &lDisplacement, bool &bSuccess)
{
    IMAGEHLP_LINE64 lineInfo = { 0 };
    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    bSuccess = BOOLIFY(SymGetLineFromName64(m_hProcess, pName, pFileName, dwLineNumber,
        &lDisplacement, &lineInfo));
    if (!bSuccess)
    {
        fprintf(stderr, "Could not get virtual address for symbol %s:%s on line %i.\n"
            "Error = %X.\n", pFileName, pName, dwLineNumber, GetLastError());
    }

    return lineInfo;
}

const bool Symbols::SymbolModuleExists(const DWORD_PTR dwAddress) const
{
    for (auto &symbolInfo : m_mapSymbols)
    {
        if (symbolInfo.second.dwModuleBaseAddress == dwAddress)
        {
            return true;
        }
    }

    return false;
}

const std::multimap<DWORD_PTR, ModuleSymbolInfo> &Symbols::SymbolList() const
{
    return m_mapSymbols;
}

void Symbols::PrintSymbolsForModule(const char * const pModuleName) const
{
    for (auto &symbolInfo : m_mapSymbols)
    {
        if (strstr(symbolInfo.second.strName.data(), pModuleName) != nullptr)
        {
            const auto &symbol = symbolInfo.second.symbolInfo;
            PrintSymbol(&symbol);
        }
    }
}

void Symbols::PrintSymbol(const SymbolInfo * const pSymbol) const
{
    fprintf(stderr, "Symbol name: %s\n"
        "Symbol address: %p\n"
        "Address displacement: %X\n"
        "Source file: %s\n"
        "Line number: %i\n",
        pSymbol->strName.data(), pSymbol->dwAddress,
        pSymbol->dwDisplacement, pSymbol->strSourceFile.data(),
        pSymbol->dwLineNumber);
}

const SymbolInfo * const Symbols::FindSymbolByName(const char * const pName) const
{
    for (auto &symbolInfo : m_mapSymbols)
    {
        auto &symbol = symbolInfo.second.symbolInfo;
        if (strcmp(pName, symbol.strName.data()) == 0)
        {
            return &symbol;
        }
    }

    return nullptr;
}

const SymbolInfo * const Symbols::FindSymbolByAddress(const DWORD_PTR dwAddress) const
{
    for (auto &symbolInfo : m_mapSymbols)
    {
        auto &symbol = symbolInfo.second.symbolInfo;
        if (dwAddress == symbol.dwAddress)
        {
            return &symbol;
        }
    }

    return nullptr;
}

}