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
    consoleSetWindow(console, 2, 2, console->consoleWidth - 4, console->consoleHeight - 4);
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
    consoleSetWindow(console, 2, 8, middleSeparator - 3, screenHeight - 19);
    consoleUpdate(console);
    consoles.push_back(console);

    // RIGHT
    console = new PrintConsole();
    consoleInit(console);
    consoleSetWindow(console, middleSeparator + 2, 8, screenWidth - middleSeparator - 4, screenHeight - 12);
    consoleUpdate(console);
    consoles.push_back(console);

    // BOTTOM
    console = new PrintConsole();
    consoleInit(console);
    consoleSetWindow(console, 1, screenHeight - 2, screenWidth - 2, 1);
    consoleUpdate(console);
    consoles.push_back(console);

    console = consoles[0];

    DrawWindow(console, 0, 0, console->consoleWidth, console->consoleHeight, title);
    DrawYSeparator(console, 0, 6, screenWidth);
    DrawYSeparator(console, 0, screenHeight - 3, screenWidth);
    DrawXSeparator(console, middleSeparator, 6, screenHeight - 8);
    DrawYSeparator(console, 0, screenHeight - 10, middleSeparator + 1);
    console->cursorX = 2;
    console->cursorY = screenHeight - 8;
    printf("(A) Select\n");
    console->cursorX = 2;
    printf("(B) Return\n");
    console->cursorX = 2;
    printf("(Y) Freeze / Unfreeze\n");
    console->cursorX = 2;
    printf("(+) Exit");
    consoleSetWindow(console, 2, 2, screenWidth - 4, 3);
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
    consoleClear();

    // Top Line
    console->cursorX = 0;
    console->cursorY = 0;
    printf("%c", 218);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 191);

    // Middle Vertical Lines
    console->cursorY = 1;
    for (int i = 0; i < height - 2; i++) {
        console->cursorX = 0;
        printf("%c", 179);
        console->cursorX = width - 1;
        printf("%c", 179);
        console->cursorY++;
    }

    // Bottom Line
    console->cursorX = 0;
    printf("%c", 192);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 217);

    // Title
    if (!title.empty()) {
        console->cursorX = 2;
        console->cursorY = 0;
        printf("%s", title.c_str());
    }
}

void Gui::DrawWindow(PrintConsole *console, int x, int y, int width, int height) {
    DrawWindow(console, x, y, width, height, "");
}

void Gui::DrawXSeparator(PrintConsole *console, int x, int y, int height) {
    consoleSelect(console);

    // Line
    console->cursorX = x;
    console->cursorY = y;
    printf("%c", 194);
    for (int i = 1; i < height - 1; i++) {
        console->cursorX = x;
        console->cursorY = y + i;
        printf("%c", 179);
    }
    console->cursorX = x;
    console->cursorY = y + height - 1;
    printf("%c", 193);
}

void Gui::DrawYSeparator(PrintConsole *console, int x, int y, int width) {
    consoleSelect(console);

    // Line
    console->cursorX = x;
    console->cursorY = y;
    printf("%c", 195);
    for (int i = 0; i < width - 2; i++) {
        printf("%c", 196);
    }
    printf("%c", 180);
}

void Gui::DrawMenu(PrintConsole *console, std::vector<std::string> options, int selectedIndex, bool enabled) {
    consoleSelect(console);
    consoleClear();
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
        if (i == selectedIndex) {
            for (int c = 0; c < width - (int) option.length(); c++) {
                printf(" ");
            }
            console->bg = 0;
        } else {
            printf("\n");
        }
        i++;
    }
}

void Gui::DrawMenuInline(PrintConsole *console, std::vector<std::string> options, int selectedIndex) {
    DrawMenuInline(console, options, selectedIndex, true);
}
