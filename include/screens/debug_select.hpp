#pragma once

#include "../fsm_screen.hpp"

#include <vector>
#include <string>

struct DebugSelectScreen : FsmScreen
{
    std::vector<std::string> options {"CONTINUE SEARCH", "NEW SEARCH"};
    int selectedIndex = 0;

    void draw() {
        consoleSelect(context->gui->consoles[2]);
        consoleClear();
        printf("Last search: %s\n", context->GetValueString().c_str());
        printf("Found: %zu\n\n", context->memoryDump->Size() / sizeof(u64));
        Gui::DrawMenuInline(context->gui->consoles[2], options, selectedIndex);
    }

    void entry() override {
        selectedIndex = 0;
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
        if (selectedIndex == 1) {
            context->ResetDump();
        }
        transit<DebugScreen>();
    };
};
