#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Utilities/Debug.h>

#include <Logger.h>
#include <Utils.h>

namespace
{
static constexpr auto HELPERBOX_CHAN = GW::Chat::Channel::CHANNEL_GWCA2;
static constexpr auto HELPERBOX_SENDER = L"HelperBox++";
static constexpr auto HELPERBOX_SENDER_COL = 0x00ccff;
static constexpr auto HELPERBOX_WARNING_COL = 0xFFFF44;
static constexpr auto HELPERBOX_ERROR_COL = 0xFF4444;
static constexpr auto HELPERBOX_INFO_COL = 0xFFFFFF;

FILE *logfile = nullptr;
FILE *stdout_file = nullptr;
FILE *stderr_file = nullptr;

enum LogType : uint8_t
{
    LogType_Info,
    LogType_Warning,
    LogType_Error
};
} // namespace

static void GWCALogHandler(void *, GW::LogLevel, const char *msg, const char *, unsigned int, const char *)
{
    Log::Log("[GWCA] %s", msg);
}

// === Setup and cleanup ====
bool Log::InitializeLog()
{
    logfile = stdout;
#ifdef _DEBUG
    AllocConsole();
    freopen_s(&stdout_file, "CONOUT$", "w", stdout);
    freopen_s(&stderr_file, "CONOUT$", "w", stderr);
    SetConsoleTitle("HelperBox Debug Console");
#endif
    GW::RegisterLogHandler(GWCALogHandler, nullptr);
    return true;
}

void Log::InitializeChat()
{
    GW::Chat::SetSenderColor(HELPERBOX_CHAN, 0xFF000000 | HELPERBOX_SENDER_COL);
    GW::Chat::SetMessageColor(HELPERBOX_CHAN, 0xFF000000 | HELPERBOX_INFO_COL);
}

void Log::Terminate()
{
    GW::RegisterLogHandler(nullptr, nullptr);
    GW::RegisterPanicHandler(nullptr, nullptr);

    if (stdout_file)
        fclose(stdout_file);
    if (stderr_file)
        fclose(stderr_file);

    FreeConsole();

    logfile = nullptr;
}

// === File/console logging ===
static void PrintTimestamp()
{
    time_t rawtime;
    time(&rawtime);

    struct tm timeinfo;
    localtime_s(&timeinfo, &rawtime);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

    fprintf(logfile, "[%s] ", buffer);
}

void Log::Log(const char *msg, ...)
{
    if (!logfile)
        return;
    PrintTimestamp();

    va_list args;
    va_start(args, msg);
    vfprintf(logfile, msg, args);
    va_end(args);
    if (msg[strlen(msg) - 1] != '\n')
        fprintf(logfile, "\n");
}

void Log::LogW(const wchar_t *msg, ...)
{
    if (!logfile)
        return;
    PrintTimestamp();

    va_list args;
    va_start(args, msg);
    vfwprintf(logfile, msg, args);
    va_end(args);
    if (msg[wcslen(msg) - 1] != '\n')
        fprintf(logfile, "\n");
}

// === Game chat logging ===
static void _chatlog(LogType log_type, const wchar_t *message)
{
    auto color = uint32_t{0};
    switch (log_type)
    {
    case LogType::LogType_Error:
        color = HELPERBOX_ERROR_COL;
        break;
    case LogType::LogType_Warning:
        color = HELPERBOX_WARNING_COL;
        break;
    default:
        color = HELPERBOX_INFO_COL;
        break;
    }
    size_t len = 5 + wcslen(HELPERBOX_SENDER) + 4 + 13 + wcslen(message) + 4 + 1;
    wchar_t *to_send = new wchar_t[len];
    swprintf(to_send, len - 1, L"<a=1>%s</a><c=#%6X>: %s</c>", HELPERBOX_SENDER, color, message);

    GW::GameThread::Enqueue([to_send]() {
        GW::Chat::WriteChat(HELPERBOX_CHAN, to_send, nullptr);
        delete[] to_send;
    });

    Log::LogW(L"[HB++] %s\n", message);
}

static void _vchatlog(LogType log_type, const char *format, va_list argv)
{
    char buf1[512];
    vsnprintf(buf1, 512, format, argv);
    std::wstring sbuf2 = std::wstring(buf1, buf1 + 512);
    _chatlog(log_type, sbuf2.c_str());
}

void Log::Info(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    _vchatlog(LogType::LogType_Info, format, vl);
    va_end(vl);
}

void Log::Error(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    _vchatlog(LogType::LogType_Error, format, vl);
    va_end(vl);
}

void Log::Warning(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    _vchatlog(LogType::LogType_Warning, format, vl);
    va_end(vl);
}
