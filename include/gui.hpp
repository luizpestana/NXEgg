#pragma once

#include <string>
#include <vector>
#include <switch.h>

class Gui {
public:
    Gui(std::string title);

    ~Gui();

    void UpdateConsoles();

    void InitSimpleLayout();

    void InitGridLayout();

    static bool ShowKeyboard(std::string header, std::string subHeader, std::string initial, std::string placeholder,
                      std::string button, char *out, size_t maxLength, SwkbdType type);

    static void DrawWindow(PrintConsole *console, int x, int y, int width, int height, std::string title);

    static void DrawWindow(PrintConsole *console, int x, int y, int width, int height);

    static void DrawXSeparator(PrintConsole *console, int x, int y, int height);

    static void DrawYSeparator(PrintConsole *console, int x, int y, int width);

    static void DrawMenuInline(PrintConsole *console, std::vector<std::string> options, int selectedIndex, bool enabled);

    static void DrawMenuInline(PrintConsole *console, std::vector<std::string> options, int selectedIndex);

    static void DrawMenu(PrintConsole *console, std::vector<std::string> options, int selectedIndex, bool enabled);

    static void DrawMenu(PrintConsole *console, std::vector<std::string> options, int selectedIndex);

    static void ClearConsole();

    std::vector<PrintConsole *> consoles;
    std::string title;
};
