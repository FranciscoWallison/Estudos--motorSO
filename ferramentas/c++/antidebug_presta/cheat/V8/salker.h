#pragma once
// salker.h - Stack walking x64 a partir do endereço de retorno (caller site),
// com resolução de símbolos/linhas (DbgHelp).
//
// ✔ Compatível com MSVC e MinGW-w64 (GCC/Clang)
// ✔ Somente x64
// ✔ Log customizável (use SLR_USE_LOG para redirecionar p/ seu macro LOG)
// ✔ Seed de RSP configurável (defina SLR_SEED_RSP_FROM_CONTEXT=1 se preferir usar o RSP do contexto)
// ✔ Opcional: SLR_JSON_LOG para emitir frames em JSON (simples)
//
// Requisitos de link:
//   - MSVC: link com Dbghelp.lib e Psapi.lib (ntdll é opcional)
//   - MinGW: adicione -ldbghelp -lpsapi -lntdll ao comando (ordem no fim)
// Requisitos de compilação (GCC/Clang):
//   -fno-omit-frame-pointer  (para _AddressOfReturnAddress() via builtin)
//   (ex.: g++ ... -fno-omit-frame-pointer -ldbghelp -lpsapi -lntdll)
//
// Opcional (melhor resolução de símbolos/linhas):
//   setx _NT_SYMBOL_PATH "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols"

#if !defined(_M_X64) && !defined(_AMD64_) && !defined(__x86_64__)
#  error "salker.h: este stack walker é somente para x64 (AMD64)."
#endif

#define WIN32_LEAN_AND_MEAN
#ifndef _AMD64_
#define _AMD64_
#endif

#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "logger.h"

#ifdef _MSC_VER
  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "psapi.lib")
#endif

// ======= Compat intrinsics MSVC vs GCC/Clang =======
#if defined(__GNUC__) || defined(__clang__)
  #ifndef _ReturnAddress
    #define _ReturnAddress() __builtin_return_address(0)
  #endif
  #ifndef _AddressOfReturnAddress
    // requer -fno-omit-frame-pointer
    #define _AddressOfReturnAddress() __builtin_frame_address(0)
  #endif
#endif

// ======= Configs opcionais =======
// Defina SLR_USE_LOG antes de incluir para usar seu macro LOG(fmt,...)
// Defina SLR_JSON_LOG para frames em JSON.
// Defina SLR_SEED_RSP_FROM_CONTEXT=1 para usar o RSP capturado por RtlCaptureContext.
#ifndef SLR_MAX_NAME
#define SLR_MAX_NAME 256
#endif

// ======= Símbolos (DbgHelp) =======
static HANDLE _slrProc = GetCurrentProcess();
static BOOL   _slrSymsReady = FALSE;

static void _slrInitSyms()
{
    static volatile LONG once = 0;
    if (InterlockedCompareExchange(&once, 1, 0) == 0) {
        DWORD opts = SymGetOptions();
        opts |= SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES;
        SymSetOptions(opts);
        // Se quiser, fixe manualmente o path:
        // SymSetSearchPathA(_slrProc, "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
        _slrSymsReady = SymInitialize(_slrProc, NULL, TRUE);
    }
}

// ======= Utilidades =======
static void FormatModuleFromAddr(DWORD64 addr, char* out, size_t outsz)
{
    HMODULE mod = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)(uintptr_t)addr, &mod))
    {
        char path[MAX_PATH] = {0};
        GetModuleFileNameA(mod, path, MAX_PATH);
        const char* base = strrchr(path, '\\');
        _snprintf_s(out, outsz, _TRUNCATE, "%s", base ? base + 1 : path);
    } else {
        _snprintf_s(out, outsz, _TRUNCATE, "???.dll");
    }
}

static void FormatSymbol(DWORD64 addr, char* out, size_t outsz,
                         DWORD* pLine /*opt*/, char* file /*opt*/, size_t filesz /*opt*/)
{
    _slrInitSyms();
    if (pLine) *pLine = 0;
    if (file && filesz) file[0] = 0;

    if (!_slrSymsReady) {
        _snprintf_s(out, outsz, _TRUNCATE, "0x%016llx", addr);
        return;
    }

    char buf[sizeof(SYMBOL_INFO) + SLR_MAX_NAME] = {0};
    PSYMBOL_INFO si = (PSYMBOL_INFO)buf;
    si->SizeOfStruct = sizeof(SYMBOL_INFO);
    si->MaxNameLen   = SLR_MAX_NAME - 1;
    DWORD64 disp = 0;

    if (SymFromAddr(_slrProc, addr, &disp, si)) {
        if (disp)
            _snprintf_s(out, outsz, _TRUNCATE, "%s+0x%llx", si->Name, disp);
        else
            _snprintf_s(out, outsz, _TRUNCATE, "%s", si->Name);
    } else {
        _snprintf_s(out, outsz, _TRUNCATE, "0x%016llx", addr);
    }

    IMAGEHLP_LINE64 line = {0}; line.SizeOfStruct = sizeof(line);
    DWORD dwDisp = 0;
    if (SymGetLineFromAddr64(_slrProc, addr, &dwDisp, &line)) {
        if (pLine) *pLine = line.LineNumber;
        if (file && filesz) _snprintf_s(file, filesz, _TRUNCATE, "%s", line.FileName);
    }
}

// ======= Stack walk (x64) =======
static void WalkStackFromReturnAddress(void* returnAddress, size_t maxFrames = 64)
{
    _slrInitSyms();

    CONTEXT ctx = {};
    RtlCaptureContext(&ctx);

    // Seed no call-site (onde o malware retornará)
    ctx.Rip = (DWORD64)returnAddress;

    // Seed do RSP:
#if SLR_SEED_RSP_FROM_CONTEXT
    // Usa o RSP do momento do RtlCaptureContext (às vezes gera pilha mais curta, porém estável)
    // Útil se seu ambiente não preserva frame pointer.
    // ctx.Rsp permanece como capturado.
#else
    // Usa slot do retorno atual neste hook (costuma dar a "trilha" do caller imediato)
    ctx.Rsp = (DWORD64)_AddressOfReturnAddress();
#endif

#ifndef SLR_JSON_LOG
    LOG("[STACK] ---- Inicio (from returnAddress=%p) ----", returnAddress);
#else
    LOG("{\"event\":\"stack_begin\",\"returnAddress\":\"%p\"}", returnAddress);
#endif

    for (size_t frame = 0; frame < maxFrames; ++frame)
    {
        DWORD64 ip = ctx.Rip;
        if (!ip) break;

        char mod[64]; FormatModuleFromAddr(ip, mod, sizeof(mod));
        char sym[SLR_MAX_NAME]; DWORD line = 0; char file[260];
        FormatSymbol(ip, sym, sizeof(sym), &line, file, sizeof(file));

#ifndef SLR_JSON_LOG
        if (line && file[0])
            LOG("[STACK] #%02zu  %s!%s  (0x%016llx)  %s:%lu",
                    frame, mod, sym, ip, file, line);
        else
            LOG("[STACK] #%02zu  %s!%s  (0x%016llx)",
                    frame, mod, sym, ip);
#else
        if (line && file[0])
            LOG("{\"event\":\"frame\",\"idx\":%zu,\"mod\":\"%s\",\"sym\":\"%s\",\"ip\":\"0x%016llx\",\"file\":\"%s\",\"line\":%lu}",
                    frame, mod, sym, ip, file, line);
        else
            LOG("{\"event\":\"frame\",\"idx\":%zu,\"mod\":\"%s\",\"sym\":\"%s\",\"ip\":\"0x%016llx\"}",
                    frame, mod, sym, ip);
#endif

        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION rf = RtlLookupFunctionEntry(ip, &imageBase, NULL);
        if (rf) {
            PVOID    handlerData = NULL;
            DWORD64  establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ip, rf,
                             &ctx, &handlerData, &establisher, NULL);
        } else {
            // Leaf function sem unwind info: suba pela pilha
            // (isso assume stack não corrompida e frame pointer ativo)
            ctx.Rip = *(DWORD64*)ctx.Rsp;
            ctx.Rsp += 8;
            if (!ctx.Rip) break;
        }

        // Heurística opcional de parada:
        // if (_stricmp(mod, "ntdll.dll") == 0 && frame > 0) break;
    }

#ifndef SLR_JSON_LOG
    LOG("[STACK] ---- Fim ----");
#else
    LOG("{\"event\":\"stack_end\"}");
#endif
}
