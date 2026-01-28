#pragma once

#include "gui.hpp"
#include "debugger.hpp"
#include "memory_dump.hpp"

#include <vector>
#include <string>

class AppContext {
public:
    AppContext();

    ~AppContext();

    void Close();

    bool ShouldClose();

    void DrawHeader();

    void LoadDump();

    bool DumpLoaded();

    void ResetDump();

    void SetStringToValue(char *str);

    bool SetStringToMemory(u64 address, char *str);

    std::string GetValueString();

    std::string GetAddressValueString(u64 address);

    void Search(char *str);

    void DrawStatus();

    Gui *gui = nullptr;
    Debugger *debugger = nullptr;
    MemoryDump *memoryDump = nullptr;

    std::vector<std::string> searchTypes {
        "UINT 8",
        "SINT 8",
        "UINT 16",
        "SINT 16",
        "UINT 32",
        "SINT 32",
        "UINT 64",
        "SINT 64",
        "FLOAT 32",
        "FLOAT 64",
        "POINTER"
    };

    std::vector<std::string> searchModes {
        "EQUAL",
        "NOT EQUAL",
        "GREATER",
        "GREATER OR EQUAL",
        "LOWER",
        "LOWER OR EQUAL",
        "RANGE",
        "SAME",
        "DIFFERENT",
        "INCREMENTED",
        "DECREMENTED"
    };

    std::vector<std::string> searchRegions {
        "HEAP",
        "MAIN",
        "HEAP AND MAIN",
        "RAM"
    };
private:
    bool m_shouldClose = false;
    bool m_existingDump = false;
};
