#pragma once

#include "../fsm_screen.hpp"

#include <string>

struct DebugScreen : FsmScreen
{
    void search() {
        char str[0x21];
        if (Gui::ShowKeyboard("Enter the value you want to search for", "", "",
                              context->GetValueString(), "Search", str, 0x20,
                              SwkbdType_NumPad)) {
            if (std::string(str) == "") {
                printf("Invalid input\n");
            } else {
                printf("Searching for: %s\n", str);
                context->gui->UpdateConsoles();
                context->Search(str);
                printf("Found: %zu\n", context->memoryDump->Size() / sizeof(u64));
            }
            printf("Press A to search again\n");
        }
    }

    void entry() override {
        consoleSelect(context->gui->consoles[2]);
        consoleClear();
        search();
    };

    void react(EvInputAction const &) override {
        search();
    };

    void react(EvInputBack const &) override {
        consoleClear();
        transit<MenuScreen>();
    }
};
