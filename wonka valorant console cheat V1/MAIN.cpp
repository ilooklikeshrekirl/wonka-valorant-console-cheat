// screen_color_trigger.cpp
// ------------------------------------------------------------
// High-performance screen color monitor + keyboard trigger
// Windows 10/11 | Win32 API | C++17/20 | Single-file
//
// Compile (MSVC):
//   cl /O2 /std:c++17 screen_color_trigger.cpp user32.lib gdi32.lib
//
// Behavior:
//   - Captures a small centered screen region repeatedly
//   - Detects configured colors within tolerance
//   - Triggers randomized keyboard input when detected
//
// Configuration:
//   Edit constants in CONFIGURATION section below
// ------------------------------------------------------------

#include <windows.h>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>

// ignore
struct Config {
    int captureWidth = 40;
    int captureHeight = 40;
    int tolerance = 10;

    int delayMin = 10;
    int delayMax = 50;

    int holdMin = 20;
    int holdMax = 80;

    int cooldownMin = 500;
    int cooldownMax = 1500;
};

Config g_config;

// === CONFIGURATION ===
constexpr int CAPTURE_WIDTH = 40;
constexpr int CAPTURE_HEIGHT = 40;

constexpr int COLOR_TOLERANCE = 10;

constexpr DWORD TARGET_KEY = VK_MENU;
// === COMMON VIRTUAL KEYS REFERENCE ===

// Mouse
// VK_LBUTTON   // Left mouse button
// VK_RBUTTON   // Right mouse button
// VK_MBUTTON   // Middle mouse button (wheel click)
// VK_XBUTTON1  // Mouse Button 4 (Back thumb button)
// VK_XBUTTON2  // Mouse Button 5 (Forward thumb button)

// Control keys
// VK_BACK      // Backspace
// VK_TAB       // Tab
// VK_RETURN    // Enter
// VK_SHIFT     // Shift
// VK_CONTROL   // Ctrl
// VK_MENU      // Alt
// VK_PAUSE     // Pause/Break
// VK_CAPITAL   // Caps Lock
// VK_ESCAPE    // Escape (Esc)
// VK_SPACE     // Spacebar

// Navigation keys
// VK_PRIOR     // Page Up
// VK_NEXT      // Page Down
// VK_END       // End
// VK_HOME      // Home
// VK_LEFT      // Left Arrow
// VK_UP        // Up Arrow
// VK_RIGHT     // Right Arrow
// VK_DOWN      // Down Arrow
// VK_SELECT    // Select
// VK_PRINT     // Print
// VK_EXECUTE   // Execute
// VK_SNAPSHOT  // Print Screen
// VK_INSERT    // Insert
// VK_DELETE    // Delete
// VK_HELP      // Help

// Windows / system
// VK_LWIN      // Left Windows key
// VK_RWIN      // Right Windows key
// VK_APPS      // Context menu key (right-click key)
// VK_SLEEP     // Sleep key

// Numpad
// VK_NUMPAD0   // Numpad 0
// VK_NUMPAD1   // Numpad 1
// VK_NUMPAD2   // Numpad 2
// VK_NUMPAD3   // Numpad 3
// VK_NUMPAD4   // Numpad 4
// VK_NUMPAD5   // Numpad 5
// VK_NUMPAD6   // Numpad 6
// VK_NUMPAD7   // Numpad 7
// VK_NUMPAD8   // Numpad 8
// VK_NUMPAD9   // Numpad 9
// VK_MULTIPLY  // Numpad *
// VK_ADD       // Numpad +
// VK_SEPARATOR // Separator
// VK_SUBTRACT  // Numpad -
// VK_DECIMAL   // Numpad .
// VK_DIVIDE    // Numpad /

// Function keys
// VK_F1  - VK_F12   // F1–F12
// VK_F13 - VK_F24   // Extended function keys

// Lock keys
// VK_NUMLOCK  // Num Lock
// VK_SCROLL   // Scroll Lock

// Left/right specific modifiers
// VK_LSHIFT   // Left Shift
// VK_RSHIFT   // Right Shift
// VK_LCONTROL // Left Ctrl
// VK_RCONTROL // Right Ctrl
// VK_LMENU    // Left Alt
// VK_RMENU    // Right Alt (AltGr)

// Target colors (RGB)
struct RGB_COLOR {
    int r, g, b;
};

const RGB_COLOR TARGET_COLORS[] = {
    {222, 132, 255},
    {238, 143, 211},
    {253, 118, 255},
    {255, 150, 235}
};

// Humanization timing (ms)
constexpr int DELAY_BEFORE_PRESS_MIN = 10;
constexpr int DELAY_BEFORE_PRESS_MAX = 50;

constexpr int KEY_HOLD_MIN = 20;
constexpr int KEY_HOLD_MAX = 80;

constexpr int COOLDOWN_MIN = 500;
constexpr int COOLDOWN_MAX = 1500;

// Capture loop delay (ms)
constexpr int LOOP_SLEEP_MS = 5;

void applyPreset(const std::string& name) {
    if (name == "legit") {
        g_config.captureWidth = 20;
        g_config.captureHeight = 20;
        g_config.tolerance = 6;
    }
    else if (name == "semi") {
        g_config.captureWidth = 40;
        g_config.captureHeight = 40;
        g_config.tolerance = 10;
    }
    else if (name == "blatant") {
        g_config.captureWidth = 80;
        g_config.captureHeight = 80;
        g_config.tolerance = 20;
    }
}

// ============================================================

// Fast absolute difference
inline int absdiff(int a, int b) {
    return (a > b) ? (a - b) : (b - a);
}

// Color match check
inline bool matchesColor(int r, int g, int b) {
    for (const auto& c : TARGET_COLORS) {
        if (absdiff(r, c.r) <= COLOR_TOLERANCE &&
            absdiff(g, c.g) <= COLOR_TOLERANCE &&
            absdiff(b, c.b) <= COLOR_TOLERANCE) {
            return true;
        }
    }
    return false;
}

void printTutorial() {
    std::cout <<
        R"(=== COMMANDS ===
/tutorial           Show this menu
/preset legit       Small box, tight detection
/preset semi        Medium box
/preset blatant     Large box, loose detection
/pixel <size>       Set capture box size (square)
/tolerance <value>  Set color tolerance
==================
)";
}

#include <thread>
#include <sstream>

void commandLoop() {
    std::string line;

    while (true) {
        std::getline(std::cin, line);

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "/tutorial") {
            printTutorial();
        }
        else if (cmd == "/preset") {
            std::string name;
            iss >> name;
            applyPreset(name);
            std::cout << "[Preset applied: " << name << "]\n";
        }
        else if (cmd == "/pixel") {
            int size;
            if (iss >> size) {
                g_config.captureWidth = size;
                g_config.captureHeight = size;
                std::cout << "[Pixel box: " << size << "]\n";
            }
        }
        else if (cmd == "/tolerance") {
            int t;
            if (iss >> t) {
                g_config.tolerance = t;
                std::cout << "[Tolerance: " << t << "]\n";
            }
        }
    }
}

bool detectColor(int& outX, int& outY) {
    const int stride = width_ * 4;

    for (int y = 0; y < height_; ++y) {
        const BYTE* row = buffer_.data() + y * stride;

        for (int x = 0; x < width_; ++x) {
            const BYTE* px = row + x * 4;

            int b = px[0];
            int g = px[1];
            int r = px[2];

            for (const auto& c : TARGET_COLORS) {
                if (absdiff(r, c.r) <= g_config.tolerance &&
                    absdiff(g, c.g) <= g_config.tolerance &&
                    absdiff(b, c.b) <= g_config.tolerance) {

                    outX = x;
                    outY = y;
                    return true;
                }
            }
        }
    }
    return false;
}

void moveCursorHumanized(int targetX, int targetY, std::mt19937& rng) {
    POINT p;
    GetCursorPos(&p);

    std::uniform_int_distribution<int> jitter(-2, 2);

    int steps = 5 + (rng() % 5);

    for (int i = 1; i <= steps; ++i) {
        float t = i / (float)steps;

        int x = p.x + (int)((targetX - p.x) * t) + jitter(rng);
        int y = p.y + (int)((targetY - p.y) * t) + jitter(rng);

        SetCursorPos(x, y);
        Sleep(2 + (rng() % 3));
    }
}

std::thread cmdThread(commandLoop);
cmdThread.detach();

int px, py;

if (capturer.detectColor(px, py)) {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    int centerX = (screenW - g_config.captureWidth) / 2;
    int centerY = (screenH - g_config.captureHeight) / 2;

    int targetX = centerX + px;
    int targetY = centerY + py;

    moveCursorHumanized(targetX, targetY, rng);
}

HANDLE g_console = GetStdHandle(STD_OUTPUT_HANDLE);

// Move cursor
void setCursor(short x, short y) {
    COORD pos{ x, y };
    SetConsoleCursorPosition(g_console, pos);
}

// Clear console
void clearConsole() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_console, &csbi);

    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD count;

    FillConsoleOutputCharacter(g_console, ' ', cellCount, { 0, 0 }, &count);
    FillConsoleOutputAttribute(g_console, csbi.wAttributes, cellCount, { 0, 0 }, &count);

    setCursor(0, 0);
}

void drawStatus(bool enabled) {
    setCursor(0, 0); // ALWAYS same spot (top-left)

    if (enabled) {
        std::cout << "[STATUS: ON ]   ";
    }
    else {
        std::cout << "[STATUS: OFF]   ";
    }
}

void drawHeader() {
    setCursor(0, 1);
    std::cout << "F6 = Toggle | /tutorial for commands        ";
}

if (keyDown && !lastToggleState) {
    enabled = !enabled;
    drawStatus(enabled);
}
// Send keyboard input
void sendKeyPress(DWORD vk, int hold_ms) {
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, &inputs[0], sizeof(INPUT));
    Sleep(hold_ms);
    SendInput(1, &inputs[1], sizeof(INPUT));
}

// Capture class (BitBlt optimized for small regions)
// Reason for BitBlt:
// - Extremely fast for small areas
// - Lower overhead than Desktop Duplication for tiny capture zones
// - Minimal latency and simple implementation
class ScreenCapturer {
public:
    ScreenCapturer(int width, int height)
        : width_(width), height_(height)
    {
        hScreenDC_ = GetDC(nullptr);
        hMemDC_ = CreateCompatibleDC(hScreenDC_);

        hBitmap_ = CreateCompatibleBitmap(hScreenDC_, width_, height_);
        SelectObject(hMemDC_, hBitmap_);

        buffer_.resize(width_ * height_ * 4);
    }

    ~ScreenCapturer() {
        DeleteObject(hBitmap_);
        DeleteDC(hMemDC_);
        ReleaseDC(nullptr, hScreenDC_);
    }

    bool captureCenter() {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        int x = (screenW - width_) / 2;
        int y = (screenH - height_) / 2;

        if (!BitBlt(hMemDC_, 0, 0, width_, height_,
            hScreenDC_, x, y, SRCCOPY)) {
            return false;
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width_;
        bmi.bmiHeader.biHeight = -height_; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        if (!GetDIBits(hMemDC_, hBitmap_, 0, height_,
            buffer_.data(), &bmi, DIB_RGB_COLORS)) {
            return false;
        }

        return true;
    }

    bool detectColor() {
        const int stride = width_ * 4;

        for (int y = 0; y < height_; ++y) {
            const BYTE* row = buffer_.data() + y * stride;

            for (int x = 0; x < width_; ++x) {
                const BYTE* px = row + x * 4;

                int b = px[0];
                int g = px[1];
                int r = px[2];

                if (matchesColor(r, g, b)) {
                    return true;
                }
            }
        }
        return false;
    }

private:
    int width_, height_;
    HDC hScreenDC_ = nullptr;
    HDC hMemDC_ = nullptr;
    HBITMAP hBitmap_ = nullptr;
    std::vector<BYTE> buffer_;
};

else if (cmd == "/clear") {
    clearConsole();
    drawStatus(true);   // redraw status
    drawHeader();
    extern bool enabled;
    drawStatus(enabled);
}

int main() {
    ScreenCapturer capturer(CAPTURE_WIDTH, CAPTURE_HEIGHT);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> delayDist(DELAY_BEFORE_PRESS_MIN, DELAY_BEFORE_PRESS_MAX);
    std::uniform_int_distribution<int> holdDist(KEY_HOLD_MIN, KEY_HOLD_MAX);
    std::uniform_int_distribution<int> cooldownDist(COOLDOWN_MIN, COOLDOWN_MAX);

    auto lastTrigger = std::chrono::steady_clock::now() - std::chrono::milliseconds(COOLDOWN_MAX);

    bool enabled = true;
    bool lastToggleState = false;

    std::cout << "[ON]\n";

    while (true) {
        // --- Toggle handling (edge detection) ---
        bool keyDown = (GetAsyncKeyState(TARGET_KEY) & 0x8000) != 0;

        if (keyDown && !lastToggleState) {
            enabled = !enabled;
            std::cout << (enabled ? "[ON]\n" : "[OFF]\n");
        }
        lastToggleState = keyDown;

        if (!enabled) {
            Sleep(10);
            continue;
        }

        clearConsole();
        drawStatus(true);
        drawHeader();

        // --- Capture ---
        if (!capturer.captureCenter()) {
            Sleep(10);
            continue;
        }

        // --- Detection ---
        if (capturer.detectColor()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTrigger).count();

            if (elapsed >= COOLDOWN_MIN) {
                int preDelay = delayDist(rng);
                int holdTime = holdDist(rng);
                int cooldown = cooldownDist(rng);

                Sleep(preDelay);
                sendKeyPress(TARGET_KEY, holdTime);

                lastTrigger = std::chrono::steady_clock::now();
                Sleep(cooldown);
            }
        }

        Sleep(LOOP_SLEEP_MS);
    }

    return 0;
}
