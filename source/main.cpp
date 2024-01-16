#include "fsm_screen.hpp"

AppContext *FsmScreen::context = new AppContext();

int main(int argc, char* argv[])
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    FsmScreen::start();
    while (FsmScreen::run())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus) {
            FsmScreen::dispatch(EvInputExit());
        } else if (kDown & HidNpadButton_Up) {
            FsmScreen::dispatch(EvInputUp());
        } else if (kDown & HidNpadButton_Down) {
            FsmScreen::dispatch(EvInputDown());
        } else if (kDown & HidNpadButton_Left) {
            FsmScreen::dispatch(EvInputLeft());
        } else if (kDown & HidNpadButton_Right) {
            FsmScreen::dispatch(EvInputRight());
        } else if (kDown & HidNpadButton_A) {
            FsmScreen::dispatch(EvInputAction());
        } else if (kDown & HidNpadButton_X) {
            FsmScreen::dispatch(EvInputOption());
        } else if (kDown & HidNpadButton_B) {
            FsmScreen::dispatch(EvInputBack());
        }
    }

    FsmScreen::stop();

    return EXIT_SUCCESS;
}
