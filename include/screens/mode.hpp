#pragma once

#include "../fsm_screen.hpp"

struct ModeScreen : FsmScreen
{
    int selectedIndex = 0;

    void draw() {
        Gui::DrawMenu(context->gui->consoles[2], context->searchModes, selectedIndex);
    }

    void entry() override {
        selectedIndex = context->debugger->m_searchMode;
        draw();
    };

    void react(EvInputUp const &) override {
        if (selectedIndex == 0)
            return;
        selectedIndex -= 1;
        draw();
    };

    void react(EvInputDown const &) override {
        if (selectedIndex >= (int) context->searchModes.size() - 1)
            return;
        selectedIndex += 1;
        draw();
    };

    void react(EvInputAction const &) override {
        consoleClear();
        context->debugger->m_searchMode = (searchMode_t) selectedIndex;
        context->DrawStatus();
        transit<MenuScreen>();
    };
};