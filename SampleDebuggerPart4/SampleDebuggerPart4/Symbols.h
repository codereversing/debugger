#pragma once

#include <list>
#include <map>
#include <memory>
#include <vector>

#include <Windows.h>
#include <Dbghelp.h>

namespace CodeReversing
{

struct SymbolInfo
{
    SymbolInfo() : dwAddress{ 0 }, dwLineNumber{ 0 }, dwDisplacement{ 0 }
    {
        strName.clear();
        strSourceFile.clear();
    }

    SymbolInfo(const SymbolInfo &copy) = delete;
    SymbolInfo &operator=(const SymbolInfo &copy) = delete;

    SymbolInfo(SymbolInfo &&obj)
    {
        *this = std::move(obj);
    }

    SymbolInfo &operator=(SymbolInfo &&obj)
    {
        strName = std::move(obj.strName);
        dwAddress = obj.dwAddress;
        dwLineNumber = obj.dwLineNumber;
        dwDisplacement = obj.dwDisplacement;
        strSourceFile = std::move(obj.strSourceFile);
        return *this;
    }

    ~SymbolInfo() = default;

    std::vector<char> strName;
    DWORD_PTR dwAddress;
    DWORD dwLineNumber;
    DWORD dwDisplacement;
    std::vector<char> strSourceFile;
};

struct ModuleSymbolInfo
{
    ModuleSymbolInfo() : dwModuleBaseAddress{ 0 }
    {
        strName.clear();
    }

    ModuleSymbolInfo(const ModuleSymbolInfo &copy) = delete;
    ModuleSymbolInfo &operator=(const ModuleSymbolInfo &copy) = delete;

    ModuleSymbolInfo(ModuleSymbolInfo &&obj)
    {
        *this = std::move(obj);
    }

    ModuleSymbolInfo &operator=(ModuleSymbolInfo &&obj)
    {
        dwModuleBaseAddress = obj.dwModuleBaseAddress;
        strName = std::move(obj.strName);
        symbolInfo = std::move(obj.symbolInfo);
        return *this;
    }

    ~ModuleSymbolInfo() = default;

    DWORD_PTR dwModuleBaseAddress;
    std::vector<char> strName;
    SymbolInfo symbolInfo;
};

class Symbols final
{
public:
    Symbols() = delete;
    Symbols(const HANDLE hProcess, const HANDLE hFile, const bool bLoadAll = false);

    Symbols(const Symbols &copy) = delete;
    Symbols &operator=(const Symbols &copy) = delete;

    ~Symbols();

    const bool EnumerateAllModulesWithSymbols();
    const bool EnumerateModuleSymbols(const char * const pModulePath, const DWORD64 dwBaseAddress);

    const bool SymbolFromAddress(const DWORD64 dwAddress, const SymbolInfo **pFullSymbolInfo);
    const bool SymbolFromName(const char * const pName, const SymbolInfo **pFullSymbolInfo);

    const bool SymbolLineFromAddress(const DWORD64 dwAddress);
    const bool SymbolAddressFromLine(const char * const pName, const char * const pFileName,
        const DWORD dwLineNumber);

    const bool ListSourceFiles();
    const bool DumpSourceFileInfo(const DWORD64 dwBaseAddress, const char * const pFilePath);

    const std::multimap<DWORD_PTR, ModuleSymbolInfo> &SymbolList() const;
    const SymbolInfo * const FindSymbolByName(const char * const pName) const;
    const SymbolInfo * const FindSymbolByAddress(const DWORD_PTR dwAddress) const;

    void PrintSymbolsForModule(const char * const pModuleName) const;
    void PrintSymbol(const SymbolInfo * const pSymbol) const;

private:

    struct UserContext
    {
        Symbols *pThis;
        const char *pName;
    };

    static BOOL CALLBACK SymEnumCallback(PCSTR strModuleName, DWORD64 dwBaseOfDll, PVOID pUserContext);
    static BOOL CALLBACK SymEnumCallback(PSYMBOL_INFO pSymInfo, ULONG ulSymbolSize, PVOID pUserContext);

    static BOOL CALLBACK SymEnumSourceFilesCallback(PSOURCEFILE pSourceFile, PVOID pUserContext);
    static BOOL CALLBACK SymEnumLinesCallback(PSRCCODEINFO pLineInfo, PVOID pUserContext);

    const IMAGEHLP_LINE64 GetSymbolLineInfo(const DWORD64 dwAddress, DWORD &dwDisplacement, bool &bSuccess);
    const IMAGEHLP_LINE64 GetSymbolLineInfo(const char * const pName, const char * const pFileName,
        const DWORD dwLineNumber, LONG &lDisplacement, bool &bSuccess);

    const bool SymbolModuleExists(const DWORD_PTR dwAddress) const;

    HANDLE m_hProcess;
    HANDLE m_hFile;

    std::multimap<DWORD_PTR /*dwBaseAddress*/, ModuleSymbolInfo> m_mapSymbols;
};

}