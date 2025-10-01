#pragma once

#include "../fsm_screen.hpp"
#include "../dmntcht.h"

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <switch.h>

struct DebugModifyScreen : FsmScreen
{
    std::vector <std::string> options;
    int selectedIndex = 0;

    void draw() {
        if (options.size() > 0) {
            Gui::DrawMenu(context->gui->consoles[2], options, selectedIndex);
        } else {
            consoleSelect(context->gui->consoles[2]);
            consoleClear();
            printf("Memory dump is empty, please scan first.");
        }
    }

    void read() {
        options.clear();

        u64 address = 0;
        std::stringstream ss;
        for (int i = 0; i < 30; i++) {
            if (i >= (int) (context->memoryDump->Size() / sizeof(u64))) break;
            ss.str("");

            context->memoryDump->GetData(i * sizeof(u64), &address, sizeof(u64));

            if (address >= context->memoryDump->GetDumpInfo().heapBaseAddress && address <
                                                                                 (context->memoryDump->GetDumpInfo().heapBaseAddress +
                                                                                  context->memoryDump->GetDumpInfo().heapSize)) {
                ss << "HEAP + 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10)
                   << (address - context->memoryDump->GetDumpInfo().heapBaseAddress);
            } else if (address >= context->memoryDump->GetDumpInfo().mainBaseAddress && address <
                                                                                        (context->memoryDump->GetDumpInfo().mainBaseAddress +
                                                                                         context->memoryDump->GetDumpInfo().mainSize)) {
                ss << "MAIN + 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10)
                   << (address - context->memoryDump->GetDumpInfo().mainBaseAddress);
            } else {
                ss << "BASE + 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10)
                   << (address - context->memoryDump->GetDumpInfo().addrSpaceBaseAddress);
            }
            ss << "  ( "
               << context->GetAddressValueString(address)
               << " )";

            if (context->debugger->m_frozenAddresses.find(address) != context->debugger->m_frozenAddresses.end())
                ss << " [FROZEN]";

            options.push_back(ss.str());
        }

        if (options.size() > 1) {
            options.push_back(std::string("MODIFY ALL ").append(std::to_string(context->memoryDump->Size() / sizeof(u64))));
        }
    }

    void entry() override {
        selectedIndex = 0;

        read();
        draw();
    };

    void react(EvInputUp const &) override {
        if (selectedIndex == 0)
            return;
        selectedIndex -= 1;
        draw();
    };

    void react(EvInputDown const &) override {
        if (selectedIndex >= (int) options.size() - 1)
            return;
        selectedIndex += 1;
        draw();
    };

    void react(EvInputAction const &) override {
        if (options.size() > 0) {
            char str[0x21];
            if (Gui::ShowKeyboard("Enter the value you want to write", "", "", context->GetValueString(), "Write", str,
                                  0x20, SwkbdType_NumPad)) {
                if (std::string(str) != "") {
                    u64 address = 0;
                    int modifiedCount = 0;
                    if (options.size() > 1 && selectedIndex == (int) options.size() - 1) {
                        for (u64 i = 0; i < context->memoryDump->Size(); i += sizeof(u64)) {
                            context->memoryDump->GetData(i, &address, sizeof(u64));
                            if (context->SetStringToMemory(address, str)) {
                                modifiedCount++;
                            }
                        }
                    } else {
                        context->memoryDump->GetData(selectedIndex * sizeof(u64), &address, sizeof(u64));
                        if (context->SetStringToMemory(address, str)) {
                            modifiedCount++;
                        }
                    }
                    if (modifiedCount > 0) {
                        read();
                        draw();
                    }
                }
            }
        }
    };

    void react(EvInputOption const &) override {
        if (options.size() == 1 || (selectedIndex < (int) options.size() - 1)) {
            u64 address = 0;
            context->memoryDump->GetData(selectedIndex * sizeof(u64), &address, sizeof(u64));
            bool modified = false;
            if (!context->debugger->IsFrozenAddress(address)) {
                u64 outValue;
                modified = R_SUCCEEDED(dmntchtEnableFrozenAddress(address, context->debugger->m_dataTypeSizes[context->debugger->m_searchType], &outValue));
                if (modified) {
                    context->debugger->m_frozenAddresses.insert({address, outValue});
                }
            } else {
                modified = R_SUCCEEDED(dmntchtDisableFrozenAddress(address));
                if (modified) {
                    context->debugger->m_frozenAddresses.erase(address);
                }
            }
            if (modified) {
                read();
                draw();
            }
        }
    };

    void react(EvInputBack const &) override {
        consoleClear();
        transit<MenuScreen>();
    }
};
