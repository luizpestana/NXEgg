#include "debugger.hpp"
#include "dmntcht.h"
#include "types.hpp"

#include <cstring>
#include <memory>

Debugger::Debugger() {

}

Debugger::~Debugger() {
    if (m_initialized) {
        pminfoExit();
        pmdmntExit();
        dmntchtExit();
        nsExit();
    }
}

bool Debugger::AttachToProcess() {
    if (appletGetAppletType() == AppletType_Application) {
        return false;
    }

    if (!m_initialized) {
        nsInitialize();
        dmntchtInitialize();
        pmdmntInitialize();
        pminfoInitialize();
        m_initialized = true;
    }

    Result rc = pmdmntGetApplicationProcessId(&m_pid);
    if (R_FAILED(rc)) {
        return false;
    }

    rc = pminfoGetProgramId(&m_tid, m_pid);
    if (R_FAILED(rc)) {
        return false;
    }

    if (m_tid <= 0) {
        return false;
    }

    std::unique_ptr <NsApplicationControlData> buf = std::make_unique<NsApplicationControlData>();
    size_t outsize = 0;

    NacpLanguageEntry *langentry = nullptr;
    if (buf == nullptr) {
        return false;
    }
    memset(buf.get(), 0, sizeof(NsApplicationControlData));

    rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, m_tid, buf.get(),
                                     sizeof(NsApplicationControlData), &outsize);
    if (R_FAILED(rc)) {
        return false;
    }

    if (outsize < sizeof(buf->nacp)) {
        return false;
    }

    rc = nacpGetLanguageEntry(&buf->nacp, &langentry);
    if (R_FAILED(rc) || langentry == nullptr) {
        return false;
    }

    m_titleName = std::string(langentry->name);
    m_titleAuthor = std::string(langentry->author);
    m_titleVersion = std::string(buf->nacp.display_version);

    rc = dmntchtForceOpenCheatProcess();
    if (R_FAILED(rc)) {
        return false;
    }

    DmntCheatProcessMetadata metadata;
    dmntchtGetCheatProcessMetadata(&metadata);

    m_addressSpaceBaseAddr = metadata.address_space_extents.base;
    m_heapBaseAddr = metadata.heap_extents.base;
    m_mainBaseAddr = metadata.main_nso_extents.base;

    m_heapSize = metadata.heap_extents.size;
    m_mainSize = metadata.main_nso_extents.size;

    MemoryInfo meminfo = {0};
    u64 lastAddr = 0;

    do {
        lastAddr = meminfo.addr;
        meminfo = QueryMemory(meminfo.addr + meminfo.size);

        m_memoryInfo.push_back(meminfo);
    } while (lastAddr < meminfo.addr + meminfo.size);

    for (MemoryInfo meminfo: m_memoryInfo) {
        if (m_mainBaseAddr == 0x00 && (meminfo.type == MemType_CodeStatic))
            m_mainBaseAddr = meminfo.addr;
    }

    UpdateFrozenAddresses();

    m_attached = true;
    return m_attached;
}

bool Debugger::IsAttached() {
    return m_attached;
}

void Debugger::UpdateFrozenAddresses() {
    m_frozenAddresses.clear();

    u64 frozenAddressCnt = 0;
    dmntchtGetFrozenAddressCount(&frozenAddressCnt);

    if (frozenAddressCnt != 0) {
        DmntFrozenAddressEntry frozenAddresses[frozenAddressCnt];
        dmntchtGetFrozenAddresses(frozenAddresses, frozenAddressCnt, 0, nullptr);

        for (u16 i = 0; i < frozenAddressCnt; i++) {
            m_frozenAddresses.insert({frozenAddresses[i].address, frozenAddresses[i].value.value});
        }
    }
}

bool Debugger::IsFrozenAddress(uintptr_t address) {
    DmntFrozenAddressEntry *es;
    u64 cnt = 0;
    bool frozen = false;
    dmntchtGetFrozenAddressCount(&cnt);
    if (cnt != 0) {
        es = new DmntFrozenAddressEntry[cnt];
        dmntchtGetFrozenAddresses(es, cnt, 0, nullptr);
        for (u64 i = 0; i < cnt; i++) {
            if (es[i].address == address) {
                frozen = true;
                break;
            }
        }
    }
    return frozen;
}

u64 Debugger::GetRunningApplicationTID() {
    return m_tid;
}

u64 Debugger::GetRunningApplicationPID() {
    return m_pid;
}

std::string Debugger::GetRunningTitleName() {
    return m_titleName;
}

std::string Debugger::GetRunningTitleAuthor() {
    return m_titleAuthor;
}

std::string Debugger::GetRunningTitleVersion() {
    return m_titleVersion;
}

void Debugger::SetOverclockEnabled(bool enabled) {
    if (hosversionBefore(8, 0, 0)) {
        pcvSetClockRate(PcvModule_CpuBus, enabled ? 1785 MHz : 1020
        MHz);  // Set CPU clock
        pcvSetClockRate(PcvModule_EMC, enabled ? 1600 MHz : 1331
        MHz);     // Set memory clock
    } else {
        ClkrstSession clkrstSession;
        clkrstOpenSession(&clkrstSession, PcvModuleId_CpuBus, 3);
        clkrstSetClockRate(&clkrstSession, enabled ? 1785 MHz : 1020
        MHz); // Set CPU clock
        clkrstCloseSession(&clkrstSession);

        clkrstOpenSession(&clkrstSession, PcvModuleId_EMC, 3);
        clkrstSetClockRate(&clkrstSession, enabled ? 1600 MHz : 1331
        MHz); // Set memory clock
        clkrstCloseSession(&clkrstSession);
    }
}

u64 Debugger::PeekMemory(u64 address) {
    u64 out;
    dmntchtReadCheatProcessMemory(address, &out, sizeof(u64));

    return out;
}

Result Debugger::PokeMemory(size_t varSize, u64 address, u64 value) {
    return dmntchtWriteCheatProcessMemory(address, &value, varSize);
}

Result Debugger::ReadMemory(void *buffer, size_t bufferSize, u64 address) {
    return dmntchtReadCheatProcessMemory(address, buffer, bufferSize);
}

Result Debugger::WriteMemory(void *buffer, size_t bufferSize, u64 address) {
    return dmntchtWriteCheatProcessMemory(address, buffer, bufferSize);
}

MemoryInfo Debugger::QueryMemory(u64 address) {
    MemoryInfo memInfo = {0};

    dmntchtQueryCheatProcessMemory(&memInfo, address);

    return memInfo;
}

/**
 * Primary:
 *  Initial full memory dump regardless of type
 *  Differentiate between different regions and types
 *
 * Secondary:
 *  Second full memory dump regardless of type
 *  Differentiate between regions and types. (both fix now)
 *
 *  Compare both memory dumps based on type and mode
 *   Store match addresses into additional file
 *   Matches should be stored as [MEMADDR][DUMPADDR] for fast comparing later on
 *
 * Tertiary (Loop infinitely):
 *  Iterate over match addrs file
 *   Compare value in memory at [MEMADDR] with value in second memory dump at [DUMPADDR]
 *   Store match addresses into file (displayDump)
 *   Dump all values from changed addresses into a file
 *   Matches should be stored as [MEMADDR][DUMPADDR] for fast comparing later on
 */

void Debugger::SearchMemoryAddressesPrimary(searchValue_t searchValue1, searchValue_t searchValue2,
                                            searchType_t searchType, searchMode_t searchMode,
                                            searchRegion_t searchRegion,
                                            MemoryDump **displayDump, std::vector <MemoryInfo> memInfos,
                                            u64 addressSpaceBaseAddr,
                                            u64 heapBaseAddr, u64 mainBaseAddr, u64 heapSize, u64 mainSize) {
    (*displayDump) = new MemoryDump("memdump1.dat", DumpType::ADDR, true);
    (*displayDump)->SetBaseAddresses(addressSpaceBaseAddr, heapBaseAddr, mainBaseAddr, heapSize, mainSize);
    (*displayDump)->SetSearchParams(searchType, searchMode, searchRegion, searchValue1, searchValue2);

    for (MemoryInfo meminfo: memInfos) {
        if (searchRegion == SEARCH_REGION_HEAP && meminfo.type != MemType_Heap)
            continue;
        else if (searchRegion == SEARCH_REGION_MAIN &&
                 (meminfo.type != MemType_CodeWritable && meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_HEAP_AND_MAIN &&
                 (meminfo.type != MemType_Heap && meminfo.type != MemType_CodeWritable &&
                  meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_RAM && (meminfo.perm & Perm_Rw) != Perm_Rw)
            continue;

        u64 offset = 0;
        u64 bufferSize = BUFFER_SIZE;
        u8 *buffer = new u8[bufferSize];
        while (offset < meminfo.size) {

            if (meminfo.size - offset < bufferSize)
                bufferSize = meminfo.size - offset;

            ReadMemory(buffer, bufferSize, meminfo.addr + offset);

            searchValue_t realValue = {0};
            for (u32 i = 0; i < bufferSize; i += m_dataTypeSizes[searchType]) {
                u64 address = meminfo.addr + offset + i;
                memset(&realValue, 0, 8);
                memcpy(&realValue, buffer + i, m_dataTypeSizes[searchType]);

                switch (searchMode) {
                    case SEARCH_MODE_EQ:
                        if (realValue._s64 == searchValue1._s64) {
                            (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        }
                        break;
                    case SEARCH_MODE_NEQ:
                        if (realValue._s64 != searchValue1._s64)
                            (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        break;
                    case SEARCH_MODE_GT:
                        if (searchType &
                            (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                             SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                            if (realValue._s64 > searchValue1._s64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        } else {
                            if (realValue._u64 > searchValue1._u64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        }
                        break;
                    case SEARCH_MODE_GTE:
                        if (searchType &
                            (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                             SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                            if (realValue._s64 >= searchValue1._s64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        } else {
                            if (realValue._u64 >= searchValue1._u64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        }
                        break;
                    case SEARCH_MODE_LT:
                        if (searchType &
                            (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                             SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                            if (realValue._s64 < searchValue1._s64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        } else {
                            if (realValue._u64 < searchValue1._u64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        }
                        break;
                    case SEARCH_MODE_LTE:
                        if (searchType &
                            (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                             SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                            if (realValue._s64 <= searchValue1._s64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        } else {
                            if (realValue._u64 <= searchValue1._u64)
                                (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        }
                        break;
                    case SEARCH_MODE_RANGE:
                        if (realValue._s64 >= searchValue1._s64 && realValue._s64 <= searchValue2._s64)
                            (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
                        break;
                    default:
                        break;
                }
            }

            offset += bufferSize;
        }

        delete[] buffer;
    }

    (*displayDump)->FlushBuffer();
}

void Debugger::SearchMemoryAddressesSecondary(searchValue_t searchValue1, searchValue_t searchValue2,
                                              searchType_t searchType, searchMode_t searchMode,
                                              MemoryDump **displayDump) {
    MemoryDump *newDump = new MemoryDump("memdump2.dat", DumpType::ADDR, true);

    for (size_t i = 0; i < ((*displayDump)->Size() / sizeof(u64)); i++) {
        searchValue_t value = {0};
        u64 address = 0;
        (*displayDump)->GetData(i * sizeof(u64), &address, sizeof(u64));

        ReadMemory(&value, m_dataTypeSizes[searchType], address);

        switch (searchMode) {
            case SEARCH_MODE_EQ:
                if (value._s64 == searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_NEQ:
                if (value._s64 != searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_GT:
                if (value._s64 > searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_GTE:
                if (value._s64 >= searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_LT:
                if (value._s64 < searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_LTE:
                if (value._s64 <= searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_RANGE:
                if (value._s64 >= searchValue1._s64 && value._s64 <= searchValue2._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_SAME:
                if (value._s64 == searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_DIFF:
                if (value._s64 != searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_INC:
                if (value._s64 > searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_DEC:
                if (value._s64 < searchValue1._s64)
                    newDump->AddData((u8 * ) & address, sizeof(u64));
                break;
            case SEARCH_MODE_NONE:
                break;
        }
    }

    if (newDump->Size() > 0) {
        (*displayDump)->Clear();
        (*displayDump)->SetSearchParams(searchType, searchMode, (*displayDump)->GetDumpInfo().searchRegion,
                                        searchValue1, searchValue2);
        (*displayDump)->SetDumpType(DumpType::ADDR);

        for (size_t i = 0; i < (newDump->Size() / sizeof(u64)); i++) {
            u64 address = 0;

            newDump->GetData(i * sizeof(u64), &address, sizeof(u64));
            (*displayDump)->AddData((u8 * ) & address, sizeof(u64));
        }

        (*displayDump)->FlushBuffer();
    } else {
        printf("None of values changed to the entered one!");
    }

    delete newDump;
    newDump = nullptr;

    remove("memdump2.dat");
}

void Debugger::SearchMemoryValuesPrimary(searchType_t searchType, searchMode_t searchMode,
                                         searchRegion_t searchRegion, MemoryDump **displayDump,
                                         std::vector <MemoryInfo> memInfos, u64 addressSpaceBaseAddr, u64 heapBaseAddr,
                                         u64 mainBaseAddr, u64 heapSize, u64 mainSize) {
    (*displayDump) = new MemoryDump("memdump1.dat", DumpType::DATA, true);
    (*displayDump)->SetBaseAddresses(addressSpaceBaseAddr, heapBaseAddr, mainBaseAddr, heapSize, mainSize);
    (*displayDump)->SetSearchParams(searchType, searchMode, searchRegion, {0}, {0});

    for (MemoryInfo meminfo: memInfos) {
        if (searchRegion == SEARCH_REGION_HEAP && meminfo.type != MemType_Heap)
            continue;
        else if (searchRegion == SEARCH_REGION_MAIN &&
                 (meminfo.type != MemType_CodeWritable && meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_HEAP_AND_MAIN &&
                 (meminfo.type != MemType_Heap && meminfo.type != MemType_CodeWritable &&
                  meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_RAM && (meminfo.perm & Perm_Rw) != Perm_Rw)
            continue;

        u64 offset = 0;
        u64 bufferSize = 0x40000;
        u8 *buffer = new u8[bufferSize];
        while (offset < meminfo.size) {
            if (meminfo.size - offset < bufferSize)
                bufferSize = meminfo.size - offset;

            ReadMemory(buffer, bufferSize, meminfo.addr + offset);
            (*displayDump)->AddData(buffer, bufferSize);

            offset += bufferSize;
        }

        delete[] buffer;
    }
}

void Debugger::SearchMemoryValuesSecondary(searchType_t searchType, searchMode_t searchMode,
                                           searchRegion_t searchRegion, MemoryDump **displayDump,
                                           std::vector <MemoryInfo> memInfos) {
    MemoryDump *newMemDump = new MemoryDump("memdump2.dat", DumpType::DATA, true);
    MemoryDump *addrDump = new MemoryDump("memdump3.dat", DumpType::ADDR, true);

    for (MemoryInfo meminfo: memInfos) {
        if (searchRegion == SEARCH_REGION_HEAP && meminfo.type != MemType_Heap)
            continue;
        else if (searchRegion == SEARCH_REGION_MAIN &&
                 (meminfo.type != MemType_CodeWritable && meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_HEAP_AND_MAIN &&
                 (meminfo.type != MemType_Heap && meminfo.type != MemType_CodeWritable &&
                  meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_RAM && (meminfo.perm & Perm_Rw) != Perm_Rw)
            continue;

        u64 offset = 0;
        u64 bufferSize = 0x40000;
        u8 *buffer = new u8[bufferSize];
        while (offset < meminfo.size) {

            if (meminfo.size - offset < bufferSize)
                bufferSize = meminfo.size - offset;

            ReadMemory(buffer, bufferSize, meminfo.addr + offset);
            newMemDump->AddData(buffer, bufferSize);

            offset += bufferSize;
        }

        delete[] buffer;
    }

    // Bigger buffers
    for (u64 addr = 0;
         addr < std::min((*displayDump)->Size(), newMemDump->Size()); addr += m_dataTypeSizes[searchType]) {
        searchValue_t oldValue = {0};
        searchValue_t newValue = {0};
        (*displayDump)->GetData(addr, &oldValue, m_dataTypeSizes[searchType]);
        newMemDump->GetData(addr, &newValue, m_dataTypeSizes[searchType]);

        switch (searchMode) {
            case SEARCH_MODE_SAME:
                if (newValue._u64 == oldValue._u64)
                    addrDump->AddData((u8 * ) & addr, sizeof(u64));
                break;
            case SEARCH_MODE_DIFF:
                if (newValue._u64 != oldValue._u64)
                    addrDump->AddData((u8 * ) & addr, sizeof(u64));
                break;
            case SEARCH_MODE_INC:
                if (searchType & (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                                  SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                    if (newValue._s64 > oldValue._s64)
                        (*displayDump)->AddData((u8 * ) & addr, sizeof(u64));
                } else {
                    if (newValue._u64 > oldValue._u64)
                        (*displayDump)->AddData((u8 * ) & addr, sizeof(u64));
                }
                break;
            case SEARCH_MODE_DEC:
                if (searchType & (SEARCH_TYPE_SIGNED_8BIT | SEARCH_TYPE_SIGNED_16BIT | SEARCH_TYPE_SIGNED_32BIT |
                                  SEARCH_TYPE_SIGNED_64BIT | SEARCH_TYPE_FLOAT_32BIT | SEARCH_TYPE_FLOAT_64BIT)) {
                    if (newValue._s64 < oldValue._s64)
                        (*displayDump)->AddData((u8 * ) & addr, sizeof(u64));
                } else {
                    if (newValue._u64 < oldValue._u64)
                        (*displayDump)->AddData((u8 * ) & addr, sizeof(u64));
                }
                break;
            default:
                break;
        }

    }
}

void Debugger::SearchMemoryValuesTertiary(searchType_t searchType, searchMode_t searchMode,
                                          searchRegion_t searchRegion, MemoryDump **displayDump,
                                          std::vector <MemoryInfo> memInfos) {
    new MemoryDump("memdump2.dat", DumpType::DATA, true);
    new MemoryDump("memdump3.dat", DumpType::ADDR, true);

    for (MemoryInfo meminfo: memInfos) {
        if (searchRegion == SEARCH_REGION_HEAP && meminfo.type != MemType_Heap)
            continue;
        else if (searchRegion == SEARCH_REGION_MAIN &&
                 (meminfo.type != MemType_CodeWritable && meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_HEAP_AND_MAIN &&
                 (meminfo.type != MemType_Heap && meminfo.type != MemType_CodeWritable &&
                  meminfo.type != MemType_CodeMutable))
            continue;
        else if (searchRegion == SEARCH_REGION_RAM && (meminfo.perm & Perm_Rw) != Perm_Rw)
            continue;

        u64 offset = 0;
        u64 bufferSize = 0x40000;
        u8 *buffer = new u8[bufferSize];
        while (offset < meminfo.size) {

            if (meminfo.size - offset < bufferSize)
                bufferSize = meminfo.size - offset;

            ReadMemory(buffer, bufferSize, meminfo.addr + offset);
            (*displayDump)->AddData(buffer, bufferSize);

            offset += bufferSize;
        }

        delete[] buffer;
    }
}
