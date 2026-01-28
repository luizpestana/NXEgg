#pragma once

#include "tinyfsm.hpp"
#include "app_context.hpp"
#include "fsm_events.hpp"

struct MenuScreen;

struct FsmScreen : tinyfsm::Fsm<FsmScreen> {
    virtual void entry(void) { };
    void exit(void) { };
    void react(EvInputExit const &) {
        context->Close();
    };
    virtual void react(EvInputUp const &) { };
    virtual void react(EvInputDown const &) { };
    virtual void react(EvInputLeft const &) { };
    virtual void react(EvInputRight const &) { };
    virtual void react(EvInputAction const &) { };
    virtual void react(EvInputOption const &) { };
    virtual void react(EvInputBack const &) {
        consoleClear();
        transit<MenuScreen>();
    };
    static void start() {
        if (context->debugger->AttachToProcess()) {
            context->gui->InitGridLayout();
            context->LoadDump();
            context->DrawHeader();
            context->DrawStatus();
            Fsm::start();
        } else {
            context->gui->InitSimpleLayout();
            printf("No game is running!");
        }
    }
    static bool run() {
        if (context->ShouldClose())
            return false;
        context->gui->UpdateConsoles();
        return true;
    }
    static void stop() {
        delete context;
    }
    static AppContext *context;
};

#include "screens/debug.hpp"
#include "screens/debug_modify.hpp"
#include "screens/debug_select.hpp"
#include "screens/frozen.hpp"
#include "screens/mode.hpp"
#include "screens/region.hpp"
#include "screens/type.hpp"
#include "screens/menu.hpp"

FSM_INITIAL_STATE(FsmScreen, MenuScreen)
