#include "plugin.h"
#include <dinput.h>

using namespace plugin;

class DebugConsole {
public:
    static void AddDebugMessage(std::string message, bool addNewLine = false) {
        if (addNewLine && !EndsWith(message, "\n"))
            message += "\n";
        DWORD written;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(hConsole, message.c_str(), message.size(), &written, nullptr);
    }

    static void WINAPI DebugOutputDebugString(LPCSTR lpOutputString) {
        AddDebugMessage(lpOutputString, true);
    }

    static void DebugPrintFormatted(char const *format, ...) {
        va_list myargs;
        va_start(myargs, format);
        static char buf[2048];
        buf[0] = '\0';
        vsprintf(buf, format, myargs);
        AddDebugMessage(buf, true);
        va_end(myargs);
    }

    DebugConsole() {
        if (!CheckPluginName(Obfuscate(L"DebugConsole.asi")))
            return;
        auto v = FIFA::GetAppVersion();
        if (v.id() == ID_FIFA07_1100_RLD) {
            if (AllocConsole()) {
                HWND hw = GetConsoleWindow();
                if (hw) {
                    HMENU hMenu = GetSystemMenu(hw, FALSE);
                    if (hMenu)
                        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
                    SetWindowPos(hw, HWND_TOPMOST, 20, 20, 500, 400, SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    SetForegroundWindow(hw);
                }
                freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
                freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
                freopen_s((FILE **)stdin, "CONIN$", "r", stdin);
                HANDLE hConOut = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                HANDLE hConIn = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                if (hConOut != INVALID_HANDLE_VALUE) SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
                if (hConIn != INVALID_HANDLE_VALUE) SetStdHandle(STD_INPUT_HANDLE, hConIn);
                DWORD mode = 0;
                HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
                if (hStdin != INVALID_HANDLE_VALUE) {
                    GetConsoleMode(hStdin, &mode);
                    mode &= ~ENABLE_QUICK_EDIT_MODE;
                    mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
                    SetConsoleMode(hStdin, mode);
                }

                //patch::SetUChar(0x5F1A2F + 1, 0);
                //patch::NopCall(0x79EA63, 2);
                //patch::NopCall(0x79EA7B, 2);

                patch::SetPointer(0x848218, DebugOutputDebugString);
                patch::RedirectJump(0x740210, DebugPrintFormatted);
            }
        }
    }
} debugConsole;

extern "C" __declspec(dllexport) void DebugPrint(char const *message) {
    DebugConsole::AddDebugMessage(message, false);
}
