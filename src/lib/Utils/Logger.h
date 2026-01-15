#pragma once

#include <GWCA/Managers/ChatMgr.h>

namespace Log
{
// === Setup and cleanup ====
bool InitializeLog();
void InitializeChat();
void Terminate();

// === File/console logging ===
void Log(const char *msg, ...);
void LogW(const wchar_t *msg, ...);

// === Game chat logging ===
void Info(const char *format, ...);
void Error(const char *format, ...);
void Warning(const char *format, ...);
}; // namespace Log
