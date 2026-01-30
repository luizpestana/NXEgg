#pragma once

#include "../fsm_screen.hpp"

struct TypeScreen : FsmScreen
{
    int selectedIndex = 0;

    void draw() {
        Gui::DrawMenu(context->gui->consoles[2], context->searchTypes, selectedIndex);
    }

    void entry() override {
        selectedIndex = context->debugger->m_searchType;
        draw();
    };

    void react(EvInputUp const &) override {
        if (selectedIndex == 0)
            return;
        selectedIndex -= 1;
        draw();
    };

    void react(EvInputDown const &) override {
        if (selectedIndex >= (int) context->searchTypes.size() - 1)
            return;
        selectedIndex += 1;
        draw();
    };

    void react(EvInputAction const &) override {
        Gui::ClearConsole();
        context->debugger->m_searchType = (searchType_t) selectedIndex;
        context->DrawStatus();
        transit<MenuScreen>();
    };
};
