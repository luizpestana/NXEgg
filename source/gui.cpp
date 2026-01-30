#include "gui.hpp"

#include <cstring>

Gui::Gui(std::string title) {
    this->title = title;

    PrintConsole *console = consoleInit(NULL);
    consoleUpdate(console);
    consoles.push_back(console);
}

Gui::~Gui() {
    for (PrintConsole *console: consoles) {
        consoleExit(console);
        delete console;
    }
}

void Gui::UpdateConsoles() {
    for (PrintConsole *console: consoles) {
        consoleUpdate(console);
    }
}

void Gui::InitSimpleLayout() {
    PrintConsole *console = consoles[0];

    DrawWindow(console, 0, 0, console->consoleWidth, console->consoleHeight, title);
    consoleSetWindow(console, 3, 3, console->consoleWidth - 4, console->consoleHeight - 4);
    consoleUpdate(console);
}

void Gui::InitGridLayout() {
    PrintConsole *console = consoles[0];

    int screenWidth = console->consoleWidth;
    int screenHeight = console->consoleHeight;
    int middleSeparator = 25;

    // LEFT
    console = new PrintConsole();
    consoleInit(console);
    consoleSetWindow(console, 3, 9, middleSeparator - 3, screenHeight - 19);
    consoleUpdate(console);
    consoles.push_back(console);

    // RIGHT
    console = new PrintConsole();
    consoleInit(console);
    consoleSetWindow(console, middleSeparator + 3, 9, screenWidth - middleSeparator - 4, screenHeight - 12);
    consoleUpdate(console);
    consoles.push_back(console);

    // BOTTOM
    console = new PrintConsole();
    consoleInit(console);
    consoleSetWindow(console, 2, screenHeight - 1, screenWidth - 2, 1);
    consoleUpdate(console);
    consoles.push_back(console);

    console = consoles[0];

    DrawWindow(console, 0, 0, console->consoleWidth, console->consoleHeight, title);
    DrawYSeparator(console, 0, 6, screenWidth);
    DrawYSeparator(console, 0, screenHeight - 3, screenWidth);
    DrawXSeparator(console, middleSeparator, 6, screenHeight - 8);
    DrawYSeparator(console, 0, screenHeight - 10, middleSeparator + 1);
    console->cursorX = 3;
    console->cursorY = screenHeight - 7;
    printf("(A) Select\n");
    console->cursorX = 3;
    printf("(B) Return\n");
    console->cursorX = 3;
    printf("(Y) Freeze / Unfreeze\n");
    console->cursorX = 3;
    printf("(+) Exit");
    consoleSetWindow(console, 3, 3, screenWidth - 4, 3);
    consoleUpdate(console);
}

bool Gui::ShowKeyboard(std::string header, std::string subHeader, std::string initial, std::string placeholder,
                       std::string button, char *out, size_t maxLength, SwkbdType type) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) return false;

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetHeaderText(&kbd, header.c_str());
    swkbdConfigSetSubText(&kbd, subHeader.c_str());
    swkbdConfigSetInitialText(&kbd, initial.c_str());
    swkbdConfigSetGuideText(&kbd, placeholder.c_str());
    swkbdConfigSetOkButtonText(&kbd, button.c_str());
    swkbdConfigSetType(&kbd, type);

    kbd.arg.arg.arg.leftButtonText = '.';
    kbd.arg.arg.arg.rightButtonText = '-';
    kbd.arg.arg.arg.stringLenMax = maxLength;
    kbd.arg.arg.arg.textDrawType = SwkbdTextDrawType_Line;
    kbd.arg.arg.arg.returnButtonFlag = false;
    kbd.arg.arg.arg.type = type;
    kbd.arg.arg.arg.keySetDisableBitmask =
            SwkbdKeyDisableBitmask_At | SwkbdKeyDisableBitmask_Percent | SwkbdKeyDisableBitmask_ForwardSlash |
            SwkbdKeyDisableBitmask_Backslash;

    rc = swkbdShow(&kbd, out, maxLength + 1);
    if (R_FAILED(rc)) return false;
    swkbdClose(&kbd);

    return std::strcmp(out, "") != 0;
}

void Gui::DrawWindow(PrintConsole *console, int x, int y, int width, int height, std::string title) {
    consoleSelect(console);
    ClearConsole();

    // Top Line
    console->cursorX = 1;
    console->cursorY = 1;
    printf("%c", 218);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 191);

    // Middle Vertical Lines
    console->cursorY = 2;
    for (int i = 0; i < height - 2; i++) {
        console->cursorX = 1;
        printf("%c", 179);
        console->cursorX = width;
        printf("%c", 179);
        console->cursorY++;
    }

    // Bottom Line
    console->cursorX = 1;
    printf("%c", 192);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 217);

    // Title
    if (!title.empty()) {
        console->cursorX = 3;
        console->cursorY = 1;
        printf("%s", title.c_str());
    }
}

void Gui::DrawWindow(PrintConsole *console, int x, int y, int width, int height) {
    DrawWindow(console, x, y, width, height, "");
}

void Gui::DrawXSeparator(PrintConsole *console, int x, int y, int height) {
    consoleSelect(console);

    // Line
    console->cursorX = x + 1;
    console->cursorY = y + 1;
    printf("%c", 194);
    for (int i = 2; i < height; i++) {
        console->cursorX = x + 1;
        console->cursorY = y + i;
        printf("%c", 179);
    }
    console->cursorX = x + 1;
    console->cursorY = y + height;
    printf("%c", 193);
}

void Gui::DrawYSeparator(PrintConsole *console, int x, int y, int width) {
    consoleSelect(console);

    // Line
    console->cursorX = x + 1;
    console->cursorY = y + 1;
    printf("%c", 195);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 180);
}

void Gui::DrawMenu(PrintConsole *console, std::vector<std::string> options, int selectedIndex, bool enabled) {
    consoleSelect(console);
    ClearConsole();
    console->cursorX = 1;
    console->cursorY = 1;
    DrawMenuInline(console, options, selectedIndex, enabled);
}

void Gui::DrawMenu(PrintConsole *console, std::vector<std::string> options, int selectedIndex) {
    DrawMenu(console, options, selectedIndex, true);
}

void Gui::DrawMenuInline(PrintConsole *console, std::vector<std::string> options, int selectedIndex, bool enabled) {
    int width = console->windowWidth;
    int i = 0;
    for (auto option : options) {
        if (i == selectedIndex) {
            if (enabled) {
                console->bg = 2;
            } else {
                console->bg = 8;
            }
        }
        printf("%s", option.c_str());
        for (int c = 0; c < width - (int) option.length(); c++) {
            printf(" ");
        }
        console->bg = 0;
        i++;
    }
}

void Gui::DrawMenuInline(PrintConsole *console, std::vector<std::string> options, int selectedIndex) {
    DrawMenuInline(console, options, selectedIndex, true);
}

void Gui::ClearConsole() {
    printf("\x1b[3J\x1b[2J\x1b[H");
    consoleUpdate(NULL);
}
