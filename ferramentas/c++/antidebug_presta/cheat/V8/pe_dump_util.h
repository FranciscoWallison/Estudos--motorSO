#pragma once
// pe_dump_util.h  —  Rebuild/scan & dump de PE em memória
// Uso típico: Reconstruir MEM_IMAGE (mapeado) OU varrer MEM_PRIVATE em busca de PE manualmente mapeado.
// MSVC/MinGW x64/x86. Requer <windows.h>.
//
// Opcional: SHA-256 com Windows CNG (bcrypt). Ative/desative com PEDU_ENABLE_SHA256.
//
// Link:
//   MSVC:   #pragma comment(lib,"bcrypt.lib")  (se SHA-256 habilitado)
//   MinGW:  -lbcrypt  (se SHA-256 habilitado)

#include <windows.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <stdint.h>

#ifndef PEDU_ENABLE_SHA256
#define PEDU_ENABLE_SHA256 1
#endif

#if PEDU_ENABLE_SHA256
  #include <bcrypt.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "bcrypt.lib")
  #endif
#endif

namespace pedu {

// ---------- helpers básicos ----------
inline DWORD AlignUp(DWORD v, DWORD a)   { return (v + a - 1) / a * a; }
inline SIZE_T AlignUpST(SIZE_T v, SIZE_T a){ return (v + a - 1) / a * a; }

inline std::wstring BasenameW(std::wstring path) {
    auto pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
}

inline bool WriteAll(const std::wstring& path, const void* data, DWORD size) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD wrote = 0;
    BOOL ok = WriteFile(h, data, size, &wrote, NULL);
    CloseHandle(h);
    return ok && wrote == size;
}

inline bool TryRPM(const void* src, void* dst, SIZE_T sz) {
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), src, dst, sz, &got) && got == sz;
}

// ---------- SHA-256 opcional ----------
#if PEDU_ENABLE_SHA256
inline bool ComputeSHA256(const BYTE* data, SIZE_T len, std::wstring& hexOut) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (st < 0) return false;

    DWORD objLen=0, cb=0, hashLen=0;
    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &cb, 0) < 0) { BCryptCloseAlgorithmProvider(hAlg,0); return false; }
    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,   (PUCHAR)&hashLen, sizeof(hashLen), &cb, 0) < 0) { BCryptCloseAlgorithmProvider(hAlg,0); return false; }

    std::vector<BYTE> obj(objLen), hash(hashLen);
    BCRYPT_HASH_HANDLE hHash = NULL;
    if (BCryptCreateHash(hAlg, &hHash, obj.data(), (ULONG)obj.size(), NULL, 0, 0) < 0) { BCryptCloseAlgorithmProvider(hAlg,0); return false; }
    if (BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0) < 0) { BCryptDestroyHash(hHash); BCryptCloseAlgorithmProvider(hAlg,0); return false; }
    if (BCryptFinishHash(hHash, hash.data(), hashLen, 0) < 0) { BCryptDestroyHash(hHash); BCryptCloseAlgorithmProvider(hAlg,0); return false; }
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    static const wchar_t* kHex = L"0123456789abcdef";
    hexOut.clear();
    hexOut.reserve(hashLen*2);
    for (DWORD i=0;i<hashLen;++i) {
        hexOut.push_back(kHex[(hash[i]>>4)&0xF]);
        hexOut.push_back(kHex[(hash[i]    )&0xF]);
    }
    return true;
}
#else
inline bool ComputeSHA256(const BYTE*, SIZE_T, std::wstring&) { return false; }
#endif

// ---------- Nome padrão de dump (diretório %TEMP%\MemDumps) ----------
inline std::wstring ComposeDumpPathPE(void* base, const wchar_t* fileSuffix /*ex: L"rebuilt.exe"*/) {
    wchar_t tmp[MAX_PATH]; GetTempPathW(MAX_PATH, tmp);
    std::wstring dir = std::wstring(tmp) + L"MemDumps\\";
    CreateDirectoryW(dir.c_str(), NULL);

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    SYSTEMTIME st; GetLocalTime(&st);

    wchar_t name[512];
    swprintf(name, 512, L"pe_pid%lu_tid%lu_base_%p_%04u%02u%02u_%02u%02u%02u_%s",
             pid, tid, base,
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
             fileSuffix ? fileSuffix : L"dump.exe");
    return dir + name;
}

static std::wstring ComposeDumpPath(void* base, SIZE_T total, DWORD protBefore, DWORD protAfter, const wchar_t* label) {
    wchar_t tmp[MAX_PATH]; GetTempPathW(MAX_PATH, tmp);
    std::wstring dir = std::wstring(tmp) + L"MemDumps\\";
    CreateDirectoryW(dir.c_str(), NULL);

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    SYSTEMTIME st; GetLocalTime(&st);

    wchar_t name[512];
    swprintf(name, 512,
             L"vp_pid%lu_tid%lu_base_%p_size_%zu_%04u%02u%02u_%02u%02u%02u_%08X_to_%08X_%s.bin",
             pid, tid, base, (size_t)total,
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
             protBefore, protAfter, label ? label : L"snapshot");
    return dir + name;
}

// ---------- Validação rápida de PE na memória ----------
struct PeProbe {
    bool   is64 = false;
    DWORD  sizeOfImage = 0;
    DWORD  sizeOfHeaders = 0;
    WORD   numSections = 0;
    DWORD  fileAlignment = 0;
    DWORD  sectionAlignment = 0;
};

inline bool ProbePEHeaders(const BYTE* mem, PeProbe& out) {
    IMAGE_DOS_HEADER dos{};
    if (!TryRPM(mem, &dos, sizeof(dos))) return false;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos.e_lfanew < sizeof(IMAGE_DOS_HEADER) || dos.e_lfanew > 0x1000) return false;

    DWORD sig = 0;
    if (!TryRPM(mem + dos.e_lfanew, &sig, sizeof(sig))) return false;
    if (sig != IMAGE_NT_SIGNATURE) return false;

    // Leia Machine para decidir 32/64
    IMAGE_FILE_HEADER fh{};
    if (!TryRPM(mem + dos.e_lfanew + sizeof(DWORD), &fh, sizeof(fh))) return false;
    out.numSections = fh.NumberOfSections;
    if (out.numSections == 0 || out.numSections > 96) return false;

    // Leia mínimo do OptionalHeader para SizeOfImage, alignments
    WORD magic = 0;
    if (!TryRPM(mem + dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &magic, sizeof(magic))) return false;

    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        IMAGE_OPTIONAL_HEADER64 opt{};
        if (!TryRPM(mem + dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &opt, sizeof(opt))) return false;
        out.is64 = true;
        out.sizeOfImage = opt.SizeOfImage;
        out.sizeOfHeaders = opt.SizeOfHeaders;
        out.fileAlignment = opt.FileAlignment;
        out.sectionAlignment = opt.SectionAlignment;
    } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        IMAGE_OPTIONAL_HEADER32 opt{};
        if (!TryRPM(mem + dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &opt, sizeof(opt))) return false;
        out.is64 = false;
        out.sizeOfImage = opt.SizeOfImage;
        out.sizeOfHeaders = opt.SizeOfHeaders;
        out.fileAlignment = opt.FileAlignment;
        out.sectionAlignment = opt.SectionAlignment;
    } else {
        return false;
    }

    if (out.sizeOfImage < out.sizeOfHeaders) return false;
    if (out.fileAlignment == 0 || out.fileAlignment > 0x10000) return false;
    if (out.sectionAlignment == 0) return false;
    return true;
}

// ---------- Reconstrução de PE mapeado (MEM_IMAGE) ----------
inline bool DumpPeMappedImageToFile(void* allocationBase, const std::wstring& outPath, std::wstring* outSha256=nullptr)
{
    BYTE* base = (BYTE*)allocationBase;

    IMAGE_DOS_HEADER dos{};
    if (!TryRPM(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE) return false;

    DWORD sig=0; if (!TryRPM(base + dos.e_lfanew, &sig, sizeof(sig)) || sig != IMAGE_NT_SIGNATURE) return false;

    IMAGE_FILE_HEADER fh{};
    if (!TryRPM(base + dos.e_lfanew + sizeof(DWORD), &fh, sizeof(fh))) return false;

    WORD magic=0;
    if (!TryRPM(base + dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &magic, sizeof(magic))) return false;

    DWORD sizeOfHeaders=0, fileAlignment=0;
    WORD  numSec = fh.NumberOfSections;
    if (numSec == 0 || numSec > 96) return false;

    // Local copies of NT headers (for section table offset)
    DWORD ohOff = dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        IMAGE_NT_HEADERS64 nth{};
        if (!TryRPM(base + dos.e_lfanew, &nth, sizeof(nth))) return false;
        sizeOfHeaders = nth.OptionalHeader.SizeOfHeaders;
        fileAlignment = nth.OptionalHeader.FileAlignment;

        // Compute output file size
        DWORD fileSize = std::max<DWORD>(sizeOfHeaders, 0x200);
        std::vector<IMAGE_SECTION_HEADER> secs(numSec);
        if (!TryRPM(base + dos.e_lfanew + sizeof(IMAGE_NT_HEADERS64), secs.data(), sizeof(IMAGE_SECTION_HEADER)*numSec)) return false;

        for (WORD i=0;i<numSec;++i) {
            DWORD rawOff  = secs[i].PointerToRawData;
            DWORD rawSize = secs[i].SizeOfRawData;
            DWORD virtSz  = secs[i].Misc.VirtualSize;
            if (rawSize == 0) rawSize = AlignUp(virtSz, fileAlignment);
            fileSize = std::max(fileSize, rawOff + rawSize);
        }

        std::vector<BYTE> out(fileSize, 0);

        // copy headers
        DWORD hdrCopy = std::min<DWORD>(sizeOfHeaders, fileSize);
        TryRPM(base, out.data(), hdrCopy);

        // copy sections
        for (WORD i=0;i<numSec;++i) {
            DWORD va      = secs[i].VirtualAddress;
            DWORD virtSz  = secs[i].Misc.VirtualSize;
            DWORD rawOff  = secs[i].PointerToRawData;
            DWORD rawSize = secs[i].SizeOfRawData;
            if (rawSize == 0) rawSize = AlignUp(virtSz, fileAlignment);

            DWORD toCopy = std::min(rawSize, virtSz);
            if (rawOff >= fileSize || toCopy == 0) continue;
            if (rawOff + toCopy > fileSize) toCopy = (fileSize - rawOff);

            // read section bytes safely
            SIZE_T got = 0;
            if (ReadProcessMemory(GetCurrentProcess(), base + va, out.data() + rawOff, toCopy, &got) && got) {
                if (got < toCopy) std::fill(out.begin() + rawOff + got, out.begin() + rawOff + toCopy, 0);
            } else {
                std::fill(out.begin() + rawOff, out.begin() + rawOff + toCopy, 0);
            }
        }

        if (!WriteAll(outPath, out.data(), (DWORD)out.size())) return false;
        if (outSha256) ComputeSHA256(out.data(), out.size(), *outSha256);
        return true;

    } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        IMAGE_NT_HEADERS32 nth{};
        if (!TryRPM(base + dos.e_lfanew, &nth, sizeof(nth))) return false;
        sizeOfHeaders = nth.OptionalHeader.SizeOfHeaders;
        fileAlignment = nth.OptionalHeader.FileAlignment;

        DWORD fileSize = std::max<DWORD>(sizeOfHeaders, 0x200);
        std::vector<IMAGE_SECTION_HEADER> secs(numSec);
        if (!TryRPM(base + dos.e_lfanew + sizeof(IMAGE_NT_HEADERS32), secs.data(), sizeof(IMAGE_SECTION_HEADER)*numSec)) return false;

        for (WORD i=0;i<numSec;++i) {
            DWORD rawOff  = secs[i].PointerToRawData;
            DWORD rawSize = secs[i].SizeOfRawData;
            DWORD virtSz  = secs[i].Misc.VirtualSize;
            if (rawSize == 0) rawSize = AlignUp(virtSz, fileAlignment);
            fileSize = std::max(fileSize, rawOff + rawSize);
        }

        std::vector<BYTE> out(fileSize, 0);
        DWORD hdrCopy = std::min<DWORD>(sizeOfHeaders, fileSize);
        TryRPM(base, out.data(), hdrCopy);

        for (WORD i=0;i<numSec;++i) {
            DWORD va      = secs[i].VirtualAddress;
            DWORD virtSz  = secs[i].Misc.VirtualSize;
            DWORD rawOff  = secs[i].PointerToRawData;
            DWORD rawSize = secs[i].SizeOfRawData;
            if (rawSize == 0) rawSize = AlignUp(virtSz, fileAlignment);

            DWORD toCopy = std::min(rawSize, virtSz);
            if (rawOff >= fileSize || toCopy == 0) continue;
            if (rawOff + toCopy > fileSize) toCopy = (fileSize - rawOff);

            SIZE_T got = 0;
            if (ReadProcessMemory(GetCurrentProcess(), base + va, out.data() + rawOff, toCopy, &got) && got) {
                if (got < toCopy) std::fill(out.begin() + rawOff + got, out.begin() + rawOff + toCopy, 0);
            } else {
                std::fill(out.begin() + rawOff, out.begin() + rawOff + toCopy, 0);
            }
        }

        if (!WriteAll(outPath, out.data(), (DWORD)out.size())) return false;
        if (outSha256) ComputeSHA256(out.data(), out.size(), *outSha256);
        return true;
    }

    return false;
}

// ---------- Scan de região (MEM_PRIVATE) procurando PE válido ----------
inline bool HasValidPeAt(const BYTE* p) {
    PeProbe pr{};
    return ProbePEHeaders(p, pr);
}

// Varre com passo pequeno; ao encontrar cabeçalho plausível, reconstrói usando o mesmo algoritmo.
inline bool ScanAndDumpPeInRange(void* start, SIZE_T size, const std::wstring& outPath, void** foundBaseOut=nullptr, std::wstring* outSha256=nullptr)
{
    BYTE* p = (BYTE*)start;
    BYTE* end = p + size;
    const SIZE_T step = 0x10; // granularidade fina

    for (; p + 0x200 < end; p += step) {
        if (HasValidPeAt(p)) {
            if (foundBaseOut) *foundBaseOut = p;
            bool ok = DumpPeMappedImageToFile(p, outPath, outSha256);
            return ok;
        }
    }
    return false;
}

} // namespace pedu
