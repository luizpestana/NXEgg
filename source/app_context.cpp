#include "app_context.hpp"

#include <sstream>

#include <switch.h>

AppContext::AppContext() {
    appletSetMediaPlaybackState(true);

    gui = new Gui("NXEgg App Debugger");
    debugger = new Debugger();
}

AppContext::~AppContext() {
    appletSetMediaPlaybackState(false);

    delete gui;
    delete memoryDump;
    delete debugger;
}

void AppContext::Close() {
    m_shouldClose = true;
}

bool AppContext::ShouldClose() {
    return m_shouldClose || !appletMainLoop();
}

void AppContext::DrawHeader() {
    consoleSelect(gui->consoles[0]);
    consoleClear();

    printf("Title: %s\n", debugger->GetRunningTitleName().c_str());
    printf("Author: %s\n", debugger->GetRunningTitleAuthor().c_str());
    printf("Version: %s", debugger->GetRunningTitleVersion().c_str());
}

void AppContext::LoadDump() {
    memoryDump = new MemoryDump("memdump1.dat", DumpType::UNDEFINED, false);

    m_existingDump = memoryDump->GetDumpInfo().heapBaseAddress == debugger->m_heapBaseAddr && memoryDump->Size() > 0;
    if (!m_existingDump) {
        memoryDump->Clear();

        remove("memdump2.dat");
        remove("memdump3.dat");

        debugger->m_searchType = SEARCH_TYPE_UNSIGNED_32BIT;
        debugger->m_searchMode = SEARCH_MODE_EQ;
        debugger->m_searchRegion = SEARCH_REGION_HEAP;
        debugger->m_searchValue[0]._u64 = 0;
        debugger->m_searchValue[1]._u64 = 0;
    } else {
        debugger->m_searchType = memoryDump->GetDumpInfo().searchDataType;
        if (debugger->m_searchType == SEARCH_TYPE_NONE) {
            debugger->m_searchType = SEARCH_TYPE_UNSIGNED_32BIT;
        }
        debugger->m_searchMode = memoryDump->GetDumpInfo().searchMode;
        if (debugger->m_searchMode == SEARCH_MODE_NONE) {
            debugger->m_searchMode = SEARCH_MODE_EQ;
        }
        debugger->m_searchRegion = memoryDump->GetDumpInfo().searchRegion;
        if (debugger->m_searchRegion == SEARCH_REGION_NONE) {
            debugger->m_searchRegion = SEARCH_REGION_HEAP;
        }
        debugger->m_searchValue[0] = memoryDump->GetDumpInfo().searchValue[0];
        debugger->m_searchValue[1] = memoryDump->GetDumpInfo().searchValue[1];
    }

    memoryDump->SetBaseAddresses(debugger->m_addressSpaceBaseAddr, debugger->m_heapBaseAddr, debugger->m_mainBaseAddr,
                                 debugger->m_heapSize, debugger->m_mainSize);
}

bool AppContext::DumpLoaded() {
    return m_existingDump;
}

void AppContext::ResetDump() {
    memoryDump->Clear();
    delete memoryDump;

    debugger->m_searchType = SEARCH_TYPE_UNSIGNED_32BIT;
    debugger->m_searchMode = SEARCH_MODE_EQ;
    debugger->m_searchRegion = SEARCH_REGION_HEAP;
    debugger->m_searchValue[0]._u64 = 0;
    debugger->m_searchValue[1]._u64 = 0;

    remove("memdump1.dat");
    remove("memdump2.dat");
    remove("memdump3.dat");

    memoryDump = new MemoryDump("memdump1.dat", DumpType::UNDEFINED, false);
    memoryDump->SetBaseAddresses(debugger->m_addressSpaceBaseAddr, debugger->m_heapBaseAddr, debugger->m_mainBaseAddr,
                                 debugger->m_heapSize, debugger->m_mainSize);
    m_existingDump = false;
}

void AppContext::SetStringToValue(char *str) {
    switch (debugger->m_searchType) {
        case SEARCH_TYPE_UNSIGNED_8BIT:
            debugger->m_searchValue[0]._u8 = static_cast<u8>(std::stoul(str, nullptr, 0));
            break;
        case SEARCH_TYPE_UNSIGNED_16BIT:
            debugger->m_searchValue[0]._u16 = static_cast<u16>(std::stoul(str, nullptr, 0));
            break;
        case SEARCH_TYPE_UNSIGNED_32BIT:
            debugger->m_searchValue[0]._u32 = static_cast<u32>(std::stoul(str, nullptr, 0));
            break;
        case SEARCH_TYPE_UNSIGNED_64BIT:
            debugger->m_searchValue[0]._u64 = static_cast<u64>(std::stoul(str, nullptr, 0));
            break;
        case SEARCH_TYPE_SIGNED_8BIT:
            debugger->m_searchValue[0]._s8 = static_cast<s8>(std::stol(str, nullptr, 0));
            break;
        case SEARCH_TYPE_SIGNED_16BIT:
            debugger->m_searchValue[0]._s16 = static_cast<s16>(std::stol(str, nullptr, 0));
            break;
        case SEARCH_TYPE_SIGNED_32BIT:
            debugger->m_searchValue[0]._s32 = static_cast<s32>(std::stol(str, nullptr, 0));
            break;
        case SEARCH_TYPE_SIGNED_64BIT:
            debugger->m_searchValue[0]._s64 = static_cast<s64>(std::stol(str, nullptr, 0));
            break;
        case SEARCH_TYPE_FLOAT_32BIT:
            debugger->m_searchValue[0]._f32 = static_cast<float>(std::stof(str));
            break;
        case SEARCH_TYPE_FLOAT_64BIT:
            debugger->m_searchValue[0]._f64 = static_cast<double>(std::stod(str));
            break;
        case SEARCH_TYPE_POINTER:
            debugger->m_searchValue[0]._u64 = static_cast<u64>(std::stol(str));
            break;
        case SEARCH_TYPE_NONE:
            return;
    }
}

bool AppContext::SetStringToMemory(u64 address, char *str) {
    Result rc = 0;
    if (debugger->m_searchType == SEARCH_TYPE_FLOAT_32BIT) {
        auto value = static_cast<float>(std::atof(str));
        rc = debugger->WriteMemory(&value, sizeof(value), address);
    } else if (debugger->m_searchType == SEARCH_TYPE_FLOAT_64BIT) {
        auto value = std::atof(str);
        rc = debugger->WriteMemory(&value, sizeof(value), address);
    } else if (debugger->m_searchType != SEARCH_TYPE_NONE) {
        auto value = std::atol(str);
        rc = debugger->WriteMemory((void *) &value, debugger->m_dataTypeSizes[debugger->m_searchType], address);
    }
    return R_SUCCEEDED(rc);
}

std::string AppContext::GetValueString() {
    std::stringstream ss;

    switch (debugger->m_searchType) {
        case SEARCH_TYPE_UNSIGNED_8BIT:
            ss << std::dec << static_cast<u64>(debugger->m_searchValue[0]._u8);
            break;
        case SEARCH_TYPE_UNSIGNED_16BIT:
            ss << std::dec << static_cast<u64>(debugger->m_searchValue[0]._u16);
            break;
        case SEARCH_TYPE_UNSIGNED_32BIT:
            ss << std::dec << static_cast<u64>(debugger->m_searchValue[0]._u32);
            break;
        case SEARCH_TYPE_UNSIGNED_64BIT:
            ss << std::dec << static_cast<u64>(debugger->m_searchValue[0]._u64);
            break;
        case SEARCH_TYPE_SIGNED_8BIT:
            ss << std::dec << static_cast<s64>(debugger->m_searchValue[0]._s8);
            break;
        case SEARCH_TYPE_SIGNED_16BIT:
            ss << std::dec << static_cast<s64>(debugger->m_searchValue[0]._s16);
            break;
        case SEARCH_TYPE_SIGNED_32BIT:
            ss << std::dec << static_cast<s64>(debugger->m_searchValue[0]._s32);
            break;
        case SEARCH_TYPE_SIGNED_64BIT:
            ss << std::dec << static_cast<s64>(debugger->m_searchValue[0]._s64);
            break;
        case SEARCH_TYPE_FLOAT_32BIT:
            ss.precision(15);
            ss << std::dec << debugger->m_searchValue[0]._f32;
            break;
        case SEARCH_TYPE_FLOAT_64BIT:
            ss.precision(15);
            ss << std::dec << debugger->m_searchValue[0]._f64;
            break;
        case SEARCH_TYPE_POINTER:
            ss << std::dec << debugger->m_searchValue[0]._u64;
            break;
        case SEARCH_TYPE_NONE:
            break;
    }

    return ss.str();
}

std::string AppContext::GetAddressValueString(u64 address) {
    std::stringstream ss;

    searchValue_t searchValue;
    searchValue._u64 = debugger->PeekMemory(address);

    switch (debugger->m_searchType) {
        case SEARCH_TYPE_UNSIGNED_8BIT:
            ss << std::dec << static_cast<u64>(searchValue._u8);
            break;
        case SEARCH_TYPE_UNSIGNED_16BIT:
            ss << std::dec << static_cast<u64>(searchValue._u16);
            break;
        case SEARCH_TYPE_UNSIGNED_32BIT:
            ss << std::dec << static_cast<u64>(searchValue._u32);
            break;
        case SEARCH_TYPE_UNSIGNED_64BIT:
            ss << std::dec << static_cast<u64>(searchValue._u64);
            break;
        case SEARCH_TYPE_SIGNED_8BIT:
            ss << std::dec << static_cast<s64>(searchValue._s8);
            break;
        case SEARCH_TYPE_SIGNED_16BIT:
            ss << std::dec << static_cast<s64>(searchValue._s16);
            break;
        case SEARCH_TYPE_SIGNED_32BIT:
            ss << std::dec << static_cast<s64>(searchValue._s32);
            break;
        case SEARCH_TYPE_SIGNED_64BIT:
            ss << std::dec << static_cast<s64>(searchValue._s64);
            break;
        case SEARCH_TYPE_FLOAT_32BIT:
            ss << std::dec << searchValue._f32;
            break;
        case SEARCH_TYPE_FLOAT_64BIT:
            ss << std::dec << searchValue._f64;
            break;
        case SEARCH_TYPE_POINTER:
            ss << std::dec << searchValue._u64;
            break;
        case SEARCH_TYPE_NONE:
            break;
    }

    return ss.str();
}

void AppContext::Search(char *str) {
    SetStringToValue(str);

    debugger->SetOverclockEnabled(true);

    if (debugger->m_searchMode == SEARCH_MODE_SAME ||
        debugger->m_searchMode == SEARCH_MODE_DIFF ||
        debugger->m_searchMode == SEARCH_MODE_INC ||
        debugger->m_searchMode == SEARCH_MODE_DEC) {
        if (memoryDump->Size() == 0) {
            delete memoryDump;
            memoryDump = nullptr;

            debugger->SearchMemoryValuesPrimary(debugger->m_searchType,
                                                debugger->m_searchMode,
                                                debugger->m_searchRegion,
                                                &memoryDump,
                                                debugger->m_memoryInfo,
                                                debugger->m_addressSpaceBaseAddr,
                                                debugger->m_heapBaseAddr,
                                                debugger->m_mainBaseAddr,
                                                debugger->m_heapSize,
                                                debugger->m_mainSize);
        } else if (memoryDump->GetDumpInfo().dumpType == DumpType::DATA) {
            debugger->SearchMemoryValuesSecondary(debugger->m_searchType,
                                                  debugger->m_searchMode,
                                                  debugger->m_searchRegion,
                                                  &memoryDump,
                                                  debugger->m_memoryInfo);
            delete memoryDump;
            memoryDump = nullptr;

            remove("memdump1.dat");
            rename("memdump3.dat", "memdump1.dat");

            memoryDump = new MemoryDump("memdump1.dat", DumpType::ADDR, false);
        } else if (memoryDump->GetDumpInfo().dumpType == DumpType::ADDR) {
            debugger->SearchMemoryValuesTertiary(debugger->m_searchType,
                                                 debugger->m_searchMode,
                                                 debugger->m_searchRegion,
                                                 &memoryDump,
                                                 debugger->m_memoryInfo);
        }
    } else {
        if (memoryDump->Size() == 0) {
            delete memoryDump;
            memoryDump = nullptr;

            debugger->SearchMemoryAddressesPrimary(debugger->m_searchValue[0],
                                                   debugger->m_searchValue[1],
                                                   debugger->m_searchType,
                                                   debugger->m_searchMode,
                                                   debugger->m_searchRegion,
                                                   &memoryDump,
                                                   debugger->m_memoryInfo,
                                                   debugger->m_addressSpaceBaseAddr,
                                                   debugger->m_heapBaseAddr,
                                                   debugger->m_mainBaseAddr,
                                                   debugger->m_heapSize,
                                                   debugger->m_mainSize);
        } else {
            debugger->SearchMemoryAddressesSecondary(debugger->m_searchValue[0],
                                                     debugger->m_searchValue[1],
                                                     debugger->m_searchType,
                                                     debugger->m_searchMode,
                                                     &memoryDump);
        }
    }

    debugger->SetOverclockEnabled(false);

    m_existingDump = memoryDump->Size() > 0;
}

void AppContext::DrawStatus() {
    consoleSelect(gui->consoles[3]);
    consoleClear();

    if (debugger->m_searchType >= 0) {
        printf("Type: %s", searchTypes[debugger->m_searchType].c_str());
    }
    if (debugger->m_searchMode >= 0) {
        gui->consoles[3]->cursorX = 25;
        printf("Mode: %s", searchModes[debugger->m_searchMode].c_str());
    }
    if (debugger->m_searchRegion >= 0) {
        gui->consoles[3]->cursorX = 50;
        printf("Region: %s", searchRegions[debugger->m_searchRegion].c_str());
    }
}
