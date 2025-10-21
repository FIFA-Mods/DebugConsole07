#include "plugin.h"

using namespace plugin;

AddrType OrigAptLinkerLoad[5] = {};

class DebugConsole {
public:
    enum MessageFlags {
        MSG_PRINTF = 1,
        MSG_OUTPUT_DEBUG_STRING = 2,
        MSG_APT = 4,
        MSG_PRINT_STRING = 8,
        MSG_EAGL = 16,
        MSG_SGRD = 32,
        MSG_SCREENS = 64,
        MSG_USER = 128,
        MSG_ALL = 0xFFFFFFFF
    };

    static unsigned int &Flags() {
        static unsigned int flags = GetIniFlags();
        return flags;
    }

    static void SetFlag(unsigned int &flags, MessageFlags flag, bool enabled) {
        if (enabled)
            flags |= flag;
        else
            flags &= ~flag;
    }

    static unsigned int GetIniFlags() {
        wchar_t buf[1024];
        GetPrivateProfileStringW(L"Debug", L"Messages", L"", buf, 1024,
            FIFA::GameDirPath(L"plugins\\DebugConsole.ini").c_str());
        auto messages = ToLower(buf);
        Trim(messages);
        if (messages.empty())
            return MSG_ALL;
        auto parts = Split(ToLower(buf), L',', true, true, false);
        unsigned int flags = 0;
        for (auto p : parts) {
            bool enable = true;
            if (StartsWith(p, L"~")) {
                p = p.substr(1);
                enable = false;
            } 
            if (p == L"all")
                SetFlag(flags, MSG_ALL, enable);
            else if (p == L"printf")
                SetFlag(flags, MSG_PRINTF, enable);
            else if (p == L"outputdebugstring")
                SetFlag(flags, MSG_OUTPUT_DEBUG_STRING, enable);
            else if (p == L"apt")
                SetFlag(flags, MSG_APT, enable);
            else if (p == L"printstring")
                SetFlag(flags, MSG_PRINT_STRING, enable);
            else if (p == L"eagl")
                SetFlag(flags, MSG_EAGL, enable);
            else if (p == L"sgrd")
                SetFlag(flags, MSG_SGRD, enable);
            else if (p == L"screens")
                SetFlag(flags, MSG_SCREENS, enable);
            else if (p == L"user")
                SetFlag(flags, MSG_USER, enable);
        }
        return flags;
    }

    static void AddDebugMessage(std::string message, bool addNewLine = false) {
        if (addNewLine && !EndsWith(message, "\n"))
            message += "\n";
        DWORD written;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(hConsole, message.c_str(), message.size(), &written, nullptr);
    }

    static void WINAPI DebugOutputDebugString(LPCSTR lpOutputString) {
        if (Flags() & MSG_OUTPUT_DEBUG_STRING)
            AddDebugMessage(lpOutputString, true);
    }

    static void DebugPrintf(char const *format, ...) {
        if (Flags() & MSG_PRINTF) {
            va_list myargs;
            va_start(myargs, format);
            static char buf[2048];
            buf[0] = '\0';
            vsprintf(buf, format, myargs);
            AddDebugMessage(buf, false);
            va_end(myargs);
        }
    }

    static void AptDebugPrint(char const *format, ...) {
        if (Flags() & MSG_APT) {
            va_list myargs;
            va_start(myargs, format);
            static char buf[2048];
            buf[0] = '\0';
            vsprintf(buf, format, myargs);
            AddDebugMessage(buf, true);
            va_end(myargs);
        }
    }

    static void EAGL_PrintMessage(int, char const *format, ...) {
        if (Flags() & MSG_EAGL) {
            va_list myargs;
            va_start(myargs, format);
            static char buf[2048];
            buf[0] = '\0';
            vsprintf(buf, format, myargs);
            AddDebugMessage(buf, true);
            va_end(myargs);
        }
    }

    static void PRINT_string(char const *format, ...) {
        if (Flags() & MSG_PRINT_STRING) {
            va_list myargs;
            va_start(myargs, format);
            static char buf[2048];
            buf[0] = '\0';
            vsprintf(buf, format, myargs);
            AddDebugMessage(buf, true);
            va_end(myargs);
        }
    }

    static void SGRD_console(char const *format, ...) {
        if (Flags() & MSG_SGRD) {
            va_list myargs;
            va_start(myargs, format);
            static char buf[2048];
            buf[0] = '\0';
            vsprintf(buf, format, myargs);
            AddDebugMessage(buf, true);
            va_end(myargs);
        }
    }

    static std::string EAStringToString(void *eastring) {
        if (!eastring)
            return std::string();
        char const *data = raw_ptr<char const>(eastring, 8);
        unsigned short length = *raw_ptr<unsigned short>(eastring, 2);
        return std::string(data, length);
    }

    static std::string EAStringPtrToString(void *ptr) {
        if (!ptr)
            return std::string();
        return EAStringToString(*raw_ptr<void *>(ptr, 0));
    }

    template<unsigned int ID>
    static void METHOD OnAptLinkerLoad(void *t, DUMMY_ARG, void *sFilename, void *sTarget) {
        static char const *assetTypes[] = {
            "Loading Animation",
            "Loading Url",
            "Loading Url",
            "Loading Movie",
            "Loading Movie"
        };
        AddDebugMessage(Format("%s from \"%s\" to \"%s\"",
            assetTypes[ID], EAStringPtrToString(sFilename).c_str(), EAStringToString(sTarget).c_str()), true);
        CallMethodDynGlobal(OrigAptLinkerLoad[ID], t, sFilename, sTarget);
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
                    SetWindowPos(hw, HWND_TOPMOST, 20, 20, 530, 400, SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    SetForegroundWindow(hw);
                }
                patch::SetPointer(0x848218, DebugOutputDebugString);
                patch::RedirectJump(0x81E525, DebugPrintf);
                patch::RedirectJump(0x740210, AptDebugPrint);
                patch::RedirectJump(0x4A0CE0, AptDebugPrint);
                patch::RedirectJump(0x753400, EAGL_PrintMessage);
                patch::RedirectJump(0x79CB90, PRINT_string);
                patch::RedirectJump(0x5D8870, SGRD_console);
                patch::Nop(0x4A37C1, 10);
                if (Flags() & MSG_SCREENS) {
                    OrigAptLinkerLoad[0] = patch::RedirectCall(0x73D66C, OnAptLinkerLoad<0>);
                    OrigAptLinkerLoad[1] = patch::RedirectCall(0x73D7B0, OnAptLinkerLoad<1>);
                    OrigAptLinkerLoad[2] = patch::RedirectCall(0x73DA47, OnAptLinkerLoad<2>);
                    OrigAptLinkerLoad[3] = patch::RedirectCall(0x73DB85, OnAptLinkerLoad<3>);
                    OrigAptLinkerLoad[4] = patch::RedirectCall(0x73DC1B, OnAptLinkerLoad<4>);
                }
            }
        }
    }
} debugConsole;

extern "C" __declspec(dllexport) void DebugPrint(char const *message) {
    if (DebugConsole::Flags() & DebugConsole::MSG_USER)
        DebugConsole::AddDebugMessage(message, false);
}
