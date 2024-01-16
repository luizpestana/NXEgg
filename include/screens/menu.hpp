#pragma once

#include "../fsm_screen.hpp"

struct MenuScreen : FsmScreen
{
    std::vector<std::string> options {"SCAN MEMORY", "MODIFY MEMORY", "CHANGE TYPE", "CHANGE MODE", "CHANGE REGION", "LIST FROZEN"};
    int selectedIndex = 0;

    void draw(bool enabled) {
        Gui::DrawMenu(context->gui->consoles[1], options, selectedIndex, enabled);
    }

    void entry() override {
        draw(true);
    };

    void react(EvInputUp const &) override {
        if (selectedIndex == 0)
            return;
        selectedIndex -= 1;
        draw(true);
    };

    void react(EvInputDown const &) override {
        if (selectedIndex >= (int) options.size() - 1)
            return;
        selectedIndex += 1;
        draw(true);
    };

    void react(EvInputAction const &) override {
        draw(false);
        switch (selectedIndex) {
            case 0:
                if (context->DumpLoaded()) {
                    transit<DebugSelectScreen>();
                } else {
                    transit<DebugScreen>();
                }
                break;
            case 1:
                transit<DebugModifyScreen>();
                break;
            case 2:
                transit<TypeScreen>();
                break;
            case 3:
                transit<ModeScreen>();
                break;
            case 4:
                transit<RegionScreen>();
                break;
            case 5:
                transit<FrozenScreen>();
                break;
        }
    };

    void react(EvInputBack const &) override {
        context->Close();
    };
};
