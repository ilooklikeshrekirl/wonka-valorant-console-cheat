// screen_color_trigger_full.cpp
// Compile: cl /O2 /std:c++17 screen_color_trigger_full.cpp user32.lib gdi32.lib

#include <windows.h>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>
#include <fstream>

// ================= CONFIG =================
struct Config {
    int captureWidth = 40;
    int captureHeight = 40;
    int tolerance = 10;

    int radius = 40;
    int strength = 40;
    int smoothing = 5;
};

Config g_config;

DWORD g_targetKey = VK_MENU;
bool g_enabled = true;

// ================= COLORS =================
struct RGB_COLOR { int r, g, b; };

const RGB_COLOR TARGET_COLORS[] = {
    {222,132,255},{238,143,211},
    {253,118,255},{255,150,235}
};

// ================= CONSOLE =================
HANDLE g_console = GetStdHandle(STD_OUTPUT_HANDLE);

void setCursor(short x, short y) {
    SetConsoleCursorPosition(g_console, { x,y });
}

void clearConsole() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_console, &csbi);
    DWORD count;
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(g_console, ' ', cells, { 0,0 }, &count);
    setCursor(0, 0);
}

void drawStatus() {
    setCursor(0, 0);
    std::cout << (g_enabled ? "[STATUS: ON ]" : "[STATUS: OFF]");
}

void drawHeader() {
    setCursor(0, 1);
    std::cout << "F6 Toggle | /tutorial | /preset | /custompreset        ";
}

// ================= HELP =================
void printTutorial() {
    std::cout << R"(

/tutorial
  Show all commands

/preset legit|semi|blatant
  Built-in presets

/custompreset <name>
  Load preset from file

/savepreset <name>
  Save current config

/pixel <size>
  Change detection box size

/tolerance <value>
  Color match tolerance

/radius <px>
  Aim assist radius

/strength <1-100>
  Aim speed

/smooth <1-20>
  Aim smoothness

/keybind <vk>
  Change key (example: 32 = SPACE)

/clear
  Clear console

)";
}

// ================= PRESETS =================
void applyPreset(std::string name) {
    if (name == "legit") {
        g_config = { 20,20,6,20,20,8 };
    }
    else if (name == "semi") {
        g_config = { 40,40,10,40,40,5 };
    }
    else if (name == "blatant") {
        g_config = { 80,80,20,80,90,2 };
    }
}

void savePreset(std::string name) {
    std::ofstream f("presets/" + name + ".valwonk");
    f << g_config.captureWidth << " "
        << g_config.captureHeight << " "
        << g_config.tolerance << " "
        << g_config.radius << " "
        << g_config.strength << " "
        << g_config.smoothing;
}

void loadPreset(std::string name) {
    std::ifstream f("presets/" + name + ".valwonk");
    if (!f) return;

    f >> g_config.captureWidth
        >> g_config.captureHeight
        >> g_config.tolerance
        >> g_config.radius
        >> g_config.strength
        >> g_config.smoothing;
}

// ================= COMMAND LOOP =================
void commandLoop() {
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        std::istringstream iss(line);

        std::string cmd; iss >> cmd;

        if (cmd == "/tutorial") printTutorial();
        else if (cmd == "/preset") { std::string n;iss >> n;applyPreset(n); }
        else if (cmd == "/custompreset") { std::string n;iss >> n;loadPreset(n); }
        else if (cmd == "/savepreset") { std::string n;iss >> n;savePreset(n); }
        else if (cmd == "/pixel") { int s;if (iss >> s) { g_config.captureWidth = s;g_config.captureHeight = s; } }
        else if (cmd == "/tolerance") { int t;if (iss >> t)g_config.tolerance = t; }
        else if (cmd == "/radius") { int r;if (iss >> r)g_config.radius = r; }
        else if (cmd == "/strength") { int s;if (iss >> s)g_config.strength = s; }
        else if (cmd == "/smooth") { int s;if (iss >> s)g_config.smoothing = s; }
        else if (cmd == "/keybind") { int k;if (iss >> k)g_targetKey = k; }
        else if (cmd == "/clear") { clearConsole();drawStatus();drawHeader(); }
    }
}

// ================= CAPTURE =================
class ScreenCapturer {
public:
    ScreenCapturer(int w, int h) :w(w), h(h) {
        hScreen = GetDC(0);
        hMem = CreateCompatibleDC(hScreen);
        bmp = CreateCompatibleBitmap(hScreen, w, h);
        SelectObject(hMem, bmp);
        buffer.resize(w * h * 4);
    }
    ~ScreenCapturer() {
        DeleteObject(bmp);
        DeleteDC(hMem);
        ReleaseDC(0, hScreen);
    }

    bool capture() {
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);

        int x = (sw - w) / 2;
        int y = (sh - h) / 2;

        BitBlt(hMem, 0, 0, w, h, hScreen, x, y, SRCCOPY);

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;

        return GetDIBits(hMem, bmp, 0, h, buffer.data(), &bmi, DIB_RGB_COLORS);
    }

    bool detect(int& px, int& py) {
        int stride = w * 4;
        for (int y = 0;y < h;y++) {
            BYTE* row = buffer.data() + y * stride;
            for (int x = 0;x < w;x++) {
                BYTE* p = row + x * 4;
                int b = p[0], g = p[1], r = p[2];

                for (auto& c : TARGET_COLORS) {
                    if (abs(r - c.r) <= g_config.tolerance &&
                        abs(g - c.g) <= g_config.tolerance &&
                        abs(b - c.b) <= g_config.tolerance) {
                        px = x;py = y;return true;
                    }
                }
            }
        }
        return false;
    }

    int w, h;
private:
    HDC hScreen, hMem;
    HBITMAP bmp;
    std::vector<BYTE> buffer;
};

// ================= MAIN =================
int main() {
    system("mkdir presets >nul 2>nul");

    clearConsole();
    drawStatus();
    drawHeader();

    std::thread(commandLoop).detach();

    std::mt19937 rng(std::random_device{}());

    ScreenCapturer cap(g_config.captureWidth, g_config.captureHeight);

    bool last = false;

    while (true) {
        bool key = (GetAsyncKeyState(VK_F6) & 0x8000);

        if (key && !last) {
            g_enabled = !g_enabled;
            drawStatus();
        }
        last = key;

        if (!g_enabled) { Sleep(10);continue; }

        cap.w = g_config.captureWidth;
        cap.h = g_config.captureHeight;

        if (!cap.capture()) continue;

        int px, py;
        if (cap.detect(px, py)) {
            POINT p; GetCursorPos(&p);

            int cx = g_config.captureWidth / 2;
            int cy = g_config.captureHeight / 2;

            int dx = px - cx;
            int dy = py - cy;

            int dist = dx * dx + dy * dy;
            if (dist > g_config.radius * g_config.radius) continue;

            float str = g_config.strength / 100.0f;

            int mx = (int)(dx * str / g_config.smoothing);
            int my = (int)(dy * str / g_config.smoothing);

            SetCursorPos(p.x + mx, p.y + my);
        }

        Sleep(5);
    }
}