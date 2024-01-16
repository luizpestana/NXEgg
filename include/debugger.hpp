#pragma once

#include <string>
#include <vector>
#include <map>
#include <switch.h>

#include "memory_dump.hpp"

class Debugger {
public:
    Debugger();

    ~Debugger();

    bool AttachToProcess();

    bool IsAttached();

    void UpdateFrozenAddresses();

    bool IsFrozenAddress(uintptr_t address);

    u64 GetRunningApplicationTID();

    u64 GetRunningApplicationPID();

    std::string GetRunningTitleName();

    std::string GetRunningTitleAuthor();

    std::string GetRunningTitleVersion();

    void SetOverclockEnabled(bool enabled);

    u64 PeekMemory(u64 address);

    Result PokeMemory(size_t varSize, u64 address, u64 value);

    MemoryInfo QueryMemory(u64 address);

    Result ReadMemory(void *buffer, size_t bufferSize, u64 address);

    Result WriteMemory(void *buffer, size_t bufferSize, u64 address);

    void SearchMemoryAddressesPrimary(searchValue_t searchValue1, searchValue_t searchValue2,
                                      searchType_t searchType, searchMode_t searchMode,
                                      searchRegion_t searchRegion,
                                      MemoryDump **displayDump, std::vector <MemoryInfo> memInfos,
                                      u64 addressSpaceBaseAddr, u64 heapBaseAddr, u64 mainBaseAddr, u64 heapSize,
                                      u64 mainSize);

    void SearchMemoryAddressesSecondary(searchValue_t searchValue1, searchValue_t searchValue2,
                                        searchType_t searchType, searchMode_t searchMode,
                                        MemoryDump **displayDump);

    void SearchMemoryValuesPrimary(searchType_t searchType, searchMode_t searchMode,
                                   searchRegion_t searchRegion, MemoryDump **displayDump,
                                   std::vector <MemoryInfo> memInfos,
                                   u64 addressSpaceBaseAddr, u64 heapBaseAddr, u64 mainBaseAddr, u64 heapSize,
                                   u64 mainSize);

    void SearchMemoryValuesSecondary(searchType_t searchType, searchMode_t searchMode,
                                     searchRegion_t searchRegion, MemoryDump **displayDump,
                                     std::vector <MemoryInfo> memInfos);

    void SearchMemoryValuesTertiary(searchType_t searchType, searchMode_t searchMode,
                                    searchRegion_t searchRegion, MemoryDump **displayDump,
                                    std::vector <MemoryInfo> memInfos);

    std::vector <u8> m_dataTypeSizes = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8};

    std::vector <MemoryInfo> m_memoryInfo;
    std::map<u64, u64> m_frozenAddresses;

    u64 m_addressSpaceBaseAddr = 0x00;
    u64 m_heapBaseAddr = 0x00;
    u64 m_mainBaseAddr = 0x00;
    u64 m_heapSize = 0;
    u64 m_mainSize = 0;

    searchType_t m_searchType = SEARCH_TYPE_UNSIGNED_32BIT;
    searchMode_t m_searchMode = SEARCH_MODE_EQ;
    searchRegion_t m_searchRegion = SEARCH_REGION_HEAP;
    searchValue_t m_searchValue[2];
private:
    bool m_initialized = false;
    bool m_attached = false;
    Handle m_debugHandle;
    u64 m_tid = 0, m_pid = 0;
    std::string m_titleName;
    std::string m_titleAuthor;
    std::string m_titleVersion;
};
