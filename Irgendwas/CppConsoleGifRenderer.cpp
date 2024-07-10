#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include "gifdec.h"
#include "gif.h"
#include <cmath>
#include <unordered_map>
#include <chrono>
#include <conio.h>

struct ConsoleColor {
    int r, g, b;

    bool operator==(const ConsoleColor& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
};

std::vector<ConsoleColor> consoleColors;

COLORREF GetPaletteEntry(int index) {
    CONSOLE_SCREEN_BUFFER_INFOEX csbi;
    csbi.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    GetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    return csbi.ColorTable[index];
}

std::vector<ConsoleColor> GetConsoleColors() {
    std::vector<ConsoleColor> consoleColors;

    for (int i = 0; i < 16; ++i) {
        COLORREF colorRef = GetPaletteEntry(i);

        ConsoleColor color;
        color.r = GetBValue(colorRef);
        color.g = GetGValue(colorRef) * 0.9;
        color.b = GetRValue(colorRef);

        consoleColors.push_back(color);
    }

    return consoleColors;
}

namespace std {
    template <>
    struct hash<ConsoleColor> {
        size_t operator()(const ConsoleColor& color) const {
            return hash<int>()(color.r) ^ hash<int>()(color.g) ^ hash<int>()(color.b);
        }
    };
}

double multip = 2;

double colorDistance(const ConsoleColor& c1, const ConsoleColor& c2) {
    double dr = c1.r - c2.r;
    double dg = c1.g - c2.g;
    double db = c1.b - c2.b;

    double distance = pow(dr, multip) + pow(dg, multip) + pow(db, multip);

    return distance;
}

ConsoleColor findNearestConsoleColor(const ConsoleColor& inputColor) {
    ConsoleColor nearestColor = consoleColors[0];
    double minDistance = colorDistance(inputColor, nearestColor);

    for (const ConsoleColor& consoleColor : consoleColors) {
        double distance = colorDistance(inputColor, consoleColor);
        if (distance < minDistance) {
            minDistance = distance;
            nearestColor = consoleColor;
        }
    }

    return nearestColor;
}

std::unordered_map<ConsoleColor, int> colors;

int consoleColorToNumeral(const ConsoleColor& ocolor) {

    ConsoleColor color = findNearestConsoleColor(ocolor);

    for (int i = 0; i < consoleColors.size(); ++i) {
        if (color == consoleColors[i]) {
            return i;
        }
    }

    return -1;
}

bool bg = false;
char** previ;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void printCharacterAtPosition(int x, int y, const ConsoleColor& color) {
    COORD coord = { x * 2, y };
    coord.X = x * 2;
    coord.Y = y;

    auto colorCodeIterator = colors.find(color);
    int c = (colorCodeIterator != colors.end()) ? colorCodeIterator->second : -1;

    if (c == 17) {
        c = 0;
    } else if (c == 0) {
        if (previ[x][y] != 0) {
            bg = true;
            c = -1;
        }
    }

    if (c == previ[x][y]) {
        bg = false;
        return;
    }

    SetConsoleTextAttribute(hConsole, c);
    SetConsoleCursorPosition(hConsole, coord);

    const wchar_t* blackSquare = L"██";
    DWORD bytesWritten;

    if (!WriteConsoleW(hConsole, bg ? L"  " : blackSquare, 2, &bytesWritten, NULL)) {
        std::cerr << "Failed to write to the console." << std::endl;
    }

    SetConsoleTextAttribute(hConsole, 0);
    previ[x][y] = c;
    bg = false;
}

std::wstring textColor(int colorCode) {
    if (colorCode >= 0 && colorCode <= 15) {
        if (colorCode >= 8) {
            return L"\033[1;" + std::to_wstring(colorCode - 8 + 30) + L"m██";
        }
        else if (colorCode == 0) {
            return L"\033[30m██";
        }
        else {
            return L"\033[" + std::to_wstring(colorCode + 30) + L"m██";
        }
    }
    else {
        return nullptr;
    }
}

std::wstring gifBuffer = L"";
void addToBuffer(int x, int y, const ConsoleColor& color) {

    auto colorCodeIterator = colors.find(color);
    int c = (colorCodeIterator != colors.end()) ? colorCodeIterator->second : -1;

    if (c == -1) {
        bg = true;
    }

    gifBuffer += (bg ? L"  " : L"\033[0m" + textColor(c));

    bg = false;
}

void renderBuffer() {
    COORD coord = { 0,0 };
    SetConsoleCursorPosition(hConsole, coord);

    DWORD bytesWritten;
    WriteConsoleW(hConsole, gifBuffer.c_str(), gifBuffer.size(), &bytesWritten, NULL);
    std::cout << "";
}

CONSOLE_CURSOR_INFO cursorInfo;

void HideConsoleCursor() {
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void renderGifFrame(gd_GIF*& gif, uint8_t*& frame_buffer) {
    gd_render_frame(gif, frame_buffer);
    gifBuffer.clear();

    for (int y = 0; y < gif->height; y++) {
        for (int x = 0; x < gif->width; x++) {
            int pixelIndex = (y * gif->width + x) * 3;
            ConsoleColor color = {
                frame_buffer[pixelIndex],
                frame_buffer[pixelIndex + 1],
                frame_buffer[pixelIndex + 2]
            };
            addToBuffer(x, y, color);
        }
        gifBuffer += L"\n";
    }

    renderBuffer();
}

void input() {
    if (_kbhit()) {
        char a = _getch();
        
    }

}

void animationRenderGif(const std::string& currentgif) {
    gd_GIF* gif = gd_open_gif(currentgif.c_str());
    if (gif == NULL) {
        std::cout << "Error opening file " << currentgif << std::endl;
        return;
    }

    for (int i = 0; i < 768; i += 3) {
        ConsoleColor color = {
            gif->palette->colors[i + 0],
            gif->palette->colors[i + 1],
            gif->palette->colors[i + 2]
        };

        uint8_t c[3];
        c[0] = color.r;
        c[1] = color.g;
        c[2] = color.b;

        if (gd_is_bgcolor(gif, c)) {
            colors[color] = -1;
        } else {
            colors[color] = consoleColorToNumeral(color);
        }
    }

    size_t buffer_size = gif->width * gif->height * 3;
    uint8_t* frame_buffer = (uint8_t*)malloc(buffer_size);
    previ = (char**)malloc(gif->width * sizeof(char*));
    

    for (int i = 0; i < gif->width; i++) {
        previ[i] = (char*)malloc(gif->height);
    }

    for (unsigned looped = 1;; looped++) {
        while (gd_get_frame(gif)) {
            auto targetTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(gif->gce.delay * 10);

            input();
            renderGifFrame(gif, frame_buffer);

            std::this_thread::sleep_until(targetTime);
        }
        if (looped == gif->loop_count) {
            break;
        }
        gd_rewind(gif);
    }

    free(frame_buffer);
    for (int i = 0; i < gif->width; i++) {
        free(previ[i]);
    }
    free(previ);

}

int main() {
    consoleColors = GetConsoleColors();
    std::string path = "C:\\Users\\Tom\\Desktop\\OUT\\";
    std::string name = "Chandelure\\";
    bool shiny = 1;
    bool front = 1;
    std::string rest = (std::string)(shiny ? "Shiny\\" : "NonShiny\\") + (front ? "Front.gif" : "Back.gif");

    HideConsoleCursor();

    animationRenderGif(path + name + rest);
}
