#pragma once

#include "../fsm_screen.hpp"
#include "../dmntcht.h"

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <switch.h>

struct FrozenScreen : FsmScreen
{
    std::vector<std::string> options;
    int selectedIndex = 0;

    void draw() {
        if (options.size() > 0) {
            Gui::DrawMenu(context->gui->consoles[2], options, selectedIndex);
        } else {
            consoleSelect(context->gui->consoles[2]);
            consoleClear();
            printf("The frozen list is empty.");
        }
    }

    void read() {
        options.clear();

        std::stringstream ss;
        for (auto [addr, value] : context->debugger->m_frozenAddresses) {
            ss << "BASE + 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(10) << (addr - context->debugger->m_addressSpaceBaseAddr);
            ss << "( " << std::dec << value << " )";
            options.push_back(ss.str());
            ss.str("");
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

    void react(EvInputOption const &) override {
        if (options.size() > 0) {
            auto itr = context->debugger->m_frozenAddresses.begin();
            std::advance(itr, selectedIndex);

            if (R_SUCCEEDED(dmntchtDisableFrozenAddress(itr->first))) {
                context->debugger->m_frozenAddresses.erase(itr->first);
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
