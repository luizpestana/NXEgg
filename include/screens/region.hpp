#pragma once

#include "../fsm_screen.hpp"

struct RegionScreen : FsmScreen
{
    int selectedIndex = 0;

    void draw() {
        Gui::DrawMenu(context->gui->consoles[2], context->searchRegions, selectedIndex);
    }

    void entry() override {
        selectedIndex = context->debugger->m_searchRegion;
        draw();
    };

    void react(EvInputUp const &) override {
        if (selectedIndex == 0)
            return;
        selectedIndex -= 1;
        draw();
    };

    void react(EvInputDown const &) override {
        if (selectedIndex >= (int) context->searchRegions.size() - 1)
            return;
        selectedIndex += 1;
        draw();
    };

    void react(EvInputAction const &) override {
        consoleClear();
        context->debugger->m_searchRegion = (searchRegion_t) selectedIndex;
        context->DrawStatus();
        transit<MenuScreen>();
    };
};
