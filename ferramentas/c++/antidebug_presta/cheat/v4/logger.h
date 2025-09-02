// ===== logger.h (portable) =====
#pragma once
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>   // snprintf, vsnprintf
#include <string.h>  // strlen

namespace logger {
    static HANDLE gLogFile = INVALID_HANDLE_VALUE;
    static CRITICAL_SECTION gCs;
    static volatile LONG gReady = 0;

    static void Init() {
        if (InterlockedCompareExchange(&gReady, 1, 0) != 0) return;

        InitializeCriticalSection(&gCs);

        wchar_t path[MAX_PATH] = {0};
        DWORD n = GetTempPathW(MAX_PATH, path);
        if (n == 0 || n >= MAX_PATH) {
            // fallback: C:\mhooks.log
            wcscpy_s(path, L"C:\\mhooks.log");
        } else {
            wcscat_s(path, L"mhooks.log");
        }

        gLogFile = CreateFileW(path,
                               GENERIC_WRITE, FILE_SHARE_READ,
                               nullptr, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (gLogFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(gLogFile, 0, nullptr, FILE_END);
        }
    }

    static void Close() {
        if (gReady) {
            if (gLogFile != INVALID_HANDLE_VALUE) {
                CloseHandle(gLogFile);
                gLogFile = INVALID_HANDLE_VALUE;
            }
            DeleteCriticalSection(&gCs);
            InterlockedExchange(&gReady, 0);
        }
    }

    static void VLog(const char* fmt, va_list ap) {
        if (!gReady) return;

        char line[2048];
        line[0] = '\0';

        // timestamp + tid
        DWORD tid = GetCurrentThreadId();
        ULONGLONG ms = GetTickCount64();

        int hdr = snprintf(
            line, sizeof(line),
            "[%10llu][T%04lu] ",
            (unsigned long long)ms, (unsigned long)tid
        );
        if (hdr < 0 || hdr >= (int)sizeof(line)) return;

        int body = vsnprintf(
            line + hdr, sizeof(line) - (size_t)hdr,
            fmt, ap
        );
        if (body < 0) return;

        // garantir espaço para \r\n
        size_t len = strlen(line);
        if (len + 2 < sizeof(line)) {
            line[len]   = '\r';
            line[len+1] = '\n';
            line[len+2] = '\0';
            len += 2;
        } else {
            // se não couber, mantém sem CRLF
        }

        EnterCriticalSection(&gCs);
        if (gLogFile != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(gLogFile, line, (DWORD)len, &written, nullptr);
        }
        LeaveCriticalSection(&gCs);

        OutputDebugStringA(line);
    }

    static void Log(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        VLog(fmt, ap);
        va_end(ap);
    }

    static void LogNtStatus(const char* api, LONG st) {
        Log("[%s] NTSTATUS=0x%08lX", api, (unsigned long)st);
    }

    static void LogLastErr(const char* api) {
        DWORD e = GetLastError();
        Log("[%s] GetLastError=%lu (0x%08lX)", api, (unsigned long)e, (unsigned long)e);
    }
}

#define LOG(...)           logger::Log(__VA_ARGS__)
#define LOG_STATUS(api, s) logger::LogNtStatus((api), (s))
#define LOG_LASTERR(api)   logger::LogLastErr((api))
