#pragma once
#include <cstdio>
#include <cstdarg>
#include <Windows.h>

class Logger {
public:
    static void Init(const wchar_t* file = L"vmp_cheat.log") {
        if (!instance_) instance_ = new Logger(file);
    }
    static void Log(const char* fmt, ...) {
        if (!s_) return;
        va_list args;
        va_start(args, fmt);
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        OutputDebugStringA(buf);
        fprintf(s_->m_file, "[%lu] %s\n", GetTickCount64(), buf);
        fflush(s_->m_file);
    }
    static void Shutdown() { if (s_) { fclose(s_->m_file); delete s_; s_ = nullptr; } }
private:
    Logger(const wchar_t* file) { m_file = _wfopen(file, L"a"); }
    static Logger* s_;
    FILE* m_file;
};
// Instantiate in .cpp
inline Logger* Logger::s_ = nullptr;