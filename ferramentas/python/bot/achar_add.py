# python achar_add.py --pid 32048
# # opcional:
# python achar_add.py --pid 32048 --pattern "C5 FA 11 44 02 24"
# python achar_add.py --pid 32048 --pattern "C4 ?? ?? 11 44 02 24"
# python achar_add.py --pid 32048 --pattern "89 E5 6A 08 56 48 83 EC 30 48 8B 5E 17"
# python achar_add.py --pid 32048 --max-matches 3
# Para procurar pela assinatura do prólogo da função que analisamos:
# python achar_add.py --pid 25452 --pattern "89 E5 6A 08 56 48 83 EC 30 48 8B 5E 17"

# Para procurar com curinga (wildcard):
# python achar_add.py --pid 25452 --pattern "C4 ?? ?? 11 44 02 24"

# Para encontrar até 3 ocorrências:
# python achar_add.py --pid 25452 --pattern "89 E5 6A 08 56 48 83 EC 30" --max-matches 3

```py
#!/usr/bin/env python3
# achar_add.py
# Procura por VMOVSS [rdx+rax+24],xmm0 em páginas executáveis de um processo (Windows).
# Requer: pymem (pip install pymem). Execute em Python 64-bit para ler processos 64-bit.

import argparse
import ctypes as ct
from ctypes import wintypes as wt
import sys
from typing import List, Tuple, Optional

import pymem

# --- Constantes de proteção/estado de memória ---
PAGE_EXECUTE            = 0x10
PAGE_EXECUTE_READ       = 0x20
PAGE_EXECUTE_READWRITE  = 0x40
PAGE_EXECUTE_WRITECOPY  = 0x80

MEM_COMMIT   = 0x1000

EXEC_PROTS = {
    PAGE_EXECUTE, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY
}

# --- Structs Win32 ---
class MEMORY_BASIC_INFORMATION64(ct.Structure):
    _fields_ = [
        ("BaseAddress",      wt.LPVOID),
        ("AllocationBase",   wt.LPVOID),
        ("AllocationProtect", wt.DWORD),
        ("RegionSize",       ct.c_size_t),
        ("State",            wt.DWORD),
        ("Protect",          wt.DWORD),
        ("Type",             wt.DWORD),
    ]

class SYSTEM_INFO(ct.Structure):
    _fields_ = [
        ("wProcessorArchitecture", wt.WORD),
        ("wReserved", wt.WORD),
        ("dwPageSize", wt.DWORD),
        ("lpMinimumApplicationAddress", wt.LPVOID),
        ("lpMaximumApplicationAddress", wt.LPVOID),
        ("dwActiveProcessorMask", ct.c_void_p),  # não usado
        ("dwNumberOfProcessors", wt.DWORD),
        ("dwProcessorType", wt.DWORD),
        ("dwAllocationGranularity", wt.DWORD),
        ("wProcessorLevel", wt.WORD),
        ("wProcessorRevision", wt.WORD),
    ]

kernel32 = ct.WinDLL("kernel32", use_last_error=True)
VirtualQueryEx = kernel32.VirtualQueryEx
VirtualQueryEx.argtypes = [wt.HANDLE, wt.LPCVOID, ct.POINTER(MEMORY_BASIC_INFORMATION64), ct.c_size_t]
VirtualQueryEx.restype  = ct.c_size_t

GetSystemInfo = kernel32.GetSystemInfo
GetSystemInfo.argtypes = [ct.POINTER(SYSTEM_INFO)]
GetSystemInfo.restype  = None


def _ptr_val(p) -> int:
    """Converte LPVOID/c_void_p ou int em int (endereço)."""
    if isinstance(p, int):
        return p
    return int(p.value) if p else 0

def _is_exec(prot: int) -> bool:
    """Checa se a proteção tem algum bit de execução."""
    EXEC_MASK = (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                 PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
    return (prot & EXEC_MASK) != 0

# ----------------- util -----------------
def hexdump(b: bytes, around: int = 32) -> str:
    return " ".join(f"{x:02X}" for x in b[:around])

def parse_pattern(pat_str: str) -> Tuple[bytes, List[bool]]:
    """
    Converte "C5 FA 11 44 02 24" ou "C4 ?? ?? 11 44 02 24" em (bytes, mask_bool_list).
    mask[i] = True se byte é significativo; False se curinga '?'
    """
    tokens = pat_str.replace(",", " ").split()
    bs = bytearray()
    mask: List[bool] = []
    for t in tokens:
        if t == "??" or t == "?":
            bs.append(0x00)
            mask.append(False)
        else:
            if len(t) != 2:
                raise ValueError(f"Byte inválido: {t}")
            bs.append(int(t, 16))
            mask.append(True)
    return bytes(bs), mask

def find_with_mask(buf: bytes, pat: bytes, mask: List[bool]) -> List[int]:
    """
    Busca com curingas. Retorna offsets relativos dentro de 'buf'.
    """
    n = len(pat)
    if n == 0 or n > len(buf):
        return []
    res = []
    # aceleração: se não há curingas, usa .find em loop
    if all(mask):
        start = 0
        while True:
            i = buf.find(pat, start)
            if i == -1:
                break
            res.append(i)
            start = i + 1
        return res
    # com curingas
    for i in range(0, len(buf) - n + 1):
        ok = True
        # comparação byte a byte respeitando máscara
        for j in range(n):
            if mask[j] and buf[i + j] != pat[j]:
                ok = False
                break
        if ok:
            res.append(i)
    return res

def iter_executable_regions(h_process: int):
    """
    Itera (base, size) de regiões COMMIT com proteção executável.
    Usa VirtualQueryEx corretamente em x64.
    """
    si = SYSTEM_INFO()
    GetSystemInfo(ct.byref(si))
    min_addr = _ptr_val(si.lpMinimumApplicationAddress)
    max_addr = _ptr_val(si.lpMaximumApplicationAddress)

    addr = min_addr
    mbi = MEMORY_BASIC_INFORMATION64()
    sizeof_mbi = ct.sizeof(mbi)
    page = 0x1000

    while addr < max_addr:
        ret = VirtualQueryEx(h_process, ct.c_void_p(addr), ct.byref(mbi), sizeof_mbi)
        if ret == 0:
            # avança pelo menos 1 página para não travar
            addr += page
            continue

        base = _ptr_val(mbi.BaseAddress)
        size = int(mbi.RegionSize)
        state = int(mbi.State)
        prot  = int(mbi.Protect)

        if state == MEM_COMMIT and size > 0 and _is_exec(prot):
            yield (base, size)

        # avança para próxima região com salvaguarda
        next_addr = base + size if base and size else (addr + page)
        if next_addr <= addr:    # proteção contra loops em casos anômalos
            next_addr = addr + page
        addr = next_addr

def scan_region_chunked(pm: pymem.Pymem, base: int, size: int, pat: bytes, mask: List[bool], max_matches: int) -> List[int]:
    """
    Lê a região em chunks e procura o padrão com overlap para não perder matches na fronteira.
    Retorna endereços absolutos.
    """
    CHUNK = 4 * 1024 * 1024  # 4 MB
    OVERLAP = max(0, len(pat) - 1)
    found_abs: List[int] = []
    offset = 0
    carry = b""
    while offset < size and (max_matches <= 0 or len(found_abs) < max_matches):
        to_read = min(CHUNK, size - offset)
        try:
            data = pm.read_bytes(base + offset, to_read)
        except Exception:
            # algumas páginas dentro da região podem não ser legíveis
            offset += to_read
            carry = b""
            continue
        buf = carry + data
        # procurar
        hits = find_with_mask(buf, pat, mask)
        for h in hits:
            abs_addr = base + offset - len(carry) + h
            if abs_addr not in found_abs:
                found_abs.append(abs_addr)
                if max_matches > 0 and len(found_abs) >= max_matches:
                    return found_abs
        # preparar overlap
        carry = buf[-OVERLAP:] if OVERLAP > 0 else b""
        offset += to_read
    return found_abs

def try_patterns(pm: pymem.Pymem, patterns: List[str], max_matches: int) -> List[Tuple[str, int]]:
    """
    Testa várias assinaturas (string) e retorna lista de (assinatura_str, endereço_encontrado)
    """
    results: List[Tuple[str, int]] = []
    for pat_str in patterns:
        pat_bytes, mask = parse_pattern(pat_str)
        print(f"Procurando pela assinatura: {pat_str}  ({pat_bytes.hex().upper()})")
        for base, size in iter_executable_regions(pm.process_handle):
            hits = scan_region_chunked(pm, base, size, pat_bytes, mask, max_matches=(max_matches - len(results)) if max_matches > 0 else 0)
            for addr in hits:
                results.append((pat_str, addr))
                print(f"  -> match em 0x{addr:016X} (região 0x{base:016X}..+0x{size:X})")
                if max_matches > 0 and len(results) >= max_matches:
                    return results
    return results

def main():
    parser = argparse.ArgumentParser(description="Acha VMOVSS [rdx+rax+24],xmm0 por assinatura em um processo (PID).")
    parser.add_argument("--pid", type=int, required=True, help="PID do processo alvo.")
    parser.add_argument("--pattern", type=str, default=None,
                        help='Assinatura hex (use "??" como curinga). Ex.: "C5 FA 11 44 02 24"')
    parser.add_argument("--max-matches", type=int, default=1, help="Máximo de matches a retornar (0 = sem limite).")
    args = parser.parse_args()

    print(f"Tentando anexar ao processo com PID: {args.pid}...")
    try:
        pm = pymem.Pymem(args.pid)
        print(f"Sucesso! Anexado ao processo com PID: {pm.process_id}.")
    except pymem.exception.ProcessNotFound:
        print(f"Erro: processo {args.pid} não encontrado.")
        return
    except pymem.exception.CouldNotOpenProcess:
        print("Erro: não foi possível abrir o processo. Rode como Administrador e verifique arquitetura (x86/x64).")
        return

    # Assinaturas padrão:
    # 1) AVX VEX2: C5 FA 11 44 02 24  -> vmovss [rdx+rax+24],xmm0
    # 2) SSE F3:  F3 0F 11 44 02 24   -> movss [rdx+rax+24],xmm0 (fallback se AVX off)
    # 3) AVX VEX3: C4 ?? ?? 11 44 02 24 (curingas para os dois bytes do VEX3 variáveis)
    default_patterns = [
        "C5 FA 11 44 02 24",
        "F3 0F 11 44 02 24",
        "C4 ?? ?? 11 44 02 24",
    ]

    patterns = [args.pattern] if args.pattern else default_patterns

    hits = try_patterns(pm, patterns, max_matches=args.max_matches)

    if not hits:
        print("\n--- FALHA ---")
        print("Nenhuma assinatura encontrada nas páginas executáveis.")
        print("Dicas: confirme arquitetura (x86/x64), tente a variação SSE/AVX, ou ajuste a assinatura com '??'.")
        return

    print("\n--- SUCESSO ---")
    for pat_str, addr in hits:
        print(f"Assinatura '{pat_str}' encontrada em 0x{addr:016X}")
        # Mostra alguns bytes ao redor (para conferência rápida)
        try:
            context = pm.read_bytes(addr, 16)
            print(f"Bytes @0x{addr:016X}: {hexdump(context, around=len(context))}")
        except Exception:
            pass

if __name__ == "__main__":
    main()


```

