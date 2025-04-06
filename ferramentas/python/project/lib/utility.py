import ctypes 
import psutil 
from libs import winapi
from ctypes import wintypes
import logging



if not hasattr(wintypes, 'SIZE_T'): 
    wintypes.SIZE_T = ctypes.c_size_t


def getPid(name):
    """
    Retorna o PID do processo com nome especificado.
    """
    for process in psutil.process_iter(): 
        try: 
            if name.lower() == process.name().lower(): 
                return process.pid 
        except (psutil.NoSuchProcess, psutil.AccessDenied): 
            continue
    return None


def modBase(pid, moduleName): 
    baseAddress = None 

    hSnap = winapi.CreateToolhelp32Snapshot(
        winapi.TH32CS_SNAPMODULE | winapi.TH32CS_SNAPMODULE32,
        pid
    )

    if hSnap != winapi.INVALID_HANDLE_VALUE:
        modEntry = winapi.MODULEENTRY32()
        modEntry.dwSize = ctypes.sizeof(winapi.MODULEENTRY32)

        if winapi.Module32First(hSnap, ctypes.byref(modEntry)):
            while True:
                modName = modEntry.szModule.decode("utf-8", errors="ignore")
                if modName.lower() == moduleName.lower():
                    baseAddress = ctypes.addressof(modEntry.modBaseAddr.contents)
                    break
                if not winapi.Module32Next(hSnap, ctypes.byref(modEntry)):
                    break

        winapi.CloseHandle(hSnap)
    
    return baseAddress


winapi.api.ReadProcessMemory.argtypes = [ 
    wintypes.HANDLE,       # hProcess
    wintypes.LPCVOID,      # lpBaseAddress
    wintypes.LPVOID,       # lpBuffer
    wintypes.SIZE_T,       # nSize
    wintypes.LPVOID        # lpNumberOfBytesRead (ideal: POINTER(SIZE_T), mas LPVOID funciona também)
] 

winapi.api.ReadProcessMemory.restype = wintypes.BOOL


def read_remote_memory(process_handle, address, size): 
    buf = ctypes.create_string_buffer(size) 
    bytes_read = wintypes.SIZE_T(0) 
    success = winapi.api.ReadProcessMemory ( 
        process_handle, 
        wintypes.LPCVOID (address), 
        buf, 
        size, 
        ctypes.byref(bytes_read) 
    ) 
    if not success: 
        return None 
    return buf.raw


def read_remote_string(process_handle, address, max_length=128): 
    """
    Lê uma string null-terminated da memória remota.

    :param process_handle: Handle do processo alvo
    :param address: Endereço base da string
    :param max_length: Tamanho máximo para leitura
    :return: String decodificada
    """
    raw_data = read_remote_memory(process_handle, address, max_length)
    if not raw_data:
        return ""
    
    null_terminated = raw_data.split(b'\x00', 1)[0]
    return null_terminated.decode("utf-8", errors="ignore")


def getRemoteProcAddress(process_handle, module_base, func_name): 
    """
    process_handle: handle do processo remoto, obtido por exemplo com OpenProcess.
    module_base: endereço base do módulo (DLL) já carregado no processo remoto.
    func_name: nome da função que queremos localizar dentro do módulo remoto.
    """
    print("[Depuração] obterEnderecoFuncaoRemota foi chamado para '{}' no base_modulo=0x{:X}".format(func_name, module_base)) 
    
    if module_base == 0: 
        print("[Depuração] base_modulo é 0, não é possível continuar.") 
        return 0 

    # 🧠 Lê os primeiros 0x40 bytes do módulo, onde está o cabeçalho DOS (IMAGE_DOS_HEADER)
    # A estrutura contém o campo 'e_lfanew' (offset 0x3C), que aponta para o cabeçalho PE
    dos_header = read_remote_memory(process_handle, module_base, 0x40) 
    if not dos_header: 
        print("[Depuração] Falha ao ler o cabeçalho DOS.") 
        return 0

    e_lfanew = int.from_bytes(dos_header[0x3C:0x3C+4], byteorder='little',signed=False) 
    print("[Depuração] e_lfanew = {}".format(e_lfanew))

    # Read PE header (we'll grab 0x200 bytes to cover file header + optional header) 
    pe_header  = read_remote_memory(process_handle, module_base + e_lfanew, 0x200) 
    if not pe_header: 
        print("[Depuração] Failed to read PE header at e_lfanew.") 
        return 0

    # Check the PE signature (first 4 bytes should be 0x00004550 => "PE\0\0") 
    signature = int.from_bytes (pe_header [0:4], byteorder='little', signed=False) 
    if signature != 0x00004550: 
        print("[Depuração] Invalid PE signature: 0x{:X}".format(signature)) 
        return 0


    # Optional header offset is 0x18 from the start of this region 
    optional_header_offset = 0x18 
    magic = int.from_bytes(pe_header [optional_header_offset:optional_header_offset+2],"little")

    print("[Depuração] OptionalHeader magic 0x{:X}".format(magic))
    is32 = (magic == 0x108) 
    is64 = (magic == 0x20B) 

    if not (is32 or is64): 
        print("[Depuração] Not a valid PE32 or PE32+ file, magic={}".format(magic)) 
        return 0

    # For 32-bit, data directory starts at offset 0x60 from optional header 
    # For 64-bit, it's at offset 0x70 
    data_dir_offset = 0x60 if is32 else 0x70 
    export_rva = int.from_bytes(pe_header[optional_header_offset + data_dir_offset: optional_header_offset + data_dir_offset + 4], byteorder='little', signed=False) 
    
    print("[Depuração] exportRVA = 0x{:X}".format(export_rva))

    if export_rva == 0: 
        print("[Depuração] exportRVA is 0, no export table found.") 
        return 0

    export_table_ptr = module_base + export_rva 
    export_table = read_remote_memory(process_handle, export_table_ptr, 40) 
    if not export_table: 
        print("[Depuração] Failed to read export table at 0x{:X}".format(export_table_ptr)) 
        return 0


    number_of_functions = int.from_bytes(export_table [0x14:0x14+4], byteorder='little', signed=True) 
    number_of_names = int.from_bytes (export_table[0x18:0x18+4], byteorder='little', signed=True) 
    address_of_functions_rva = int.from_bytes(export_table[0x1C:0x1C+4], byteorder='little', signed=False) 
    address_of_names_rva = int.from_bytes(export_table [0x20:0x20+4], byteorder='little', signed=False) 
    address_of_ordinals_rva = int.from_bytes(export_table[0x24:0x24+4], byteorder='little', signed=False)
    
    print("[Depuração] numberOfFunctions={}, numberOfNames={}".format(number_of_functions, number_of_names))
    print("[Depuração] addressOfFunctionsRVA=0x{:X}, addressOfNamesRVA=0x{:X}, addressOfNameOrdinalsRVA=0x{:X}".format( 
    address_of_functions_rva, address_of_names_rva, address_of_ordinals_rva 
    ))


    if number_of_functions < 0 or number_of_names < 0: 
        print("[Depuração] Negative function/name count. Possibly invalid data or mismatched architecture.") 
        return 0

    
    func_addr_array = module_base + address_of_functions_rva 
    name_array = module_base + address_of_names_rva 
    ord_array = module_base + address_of_ordinals_rva 
    max_names_to_check = min(number_of_names, 4096)


    for i in range(max_names_to_check): 
        name_ptr_pos = name_array + (i * 4) 
        name_ptr_data = read_remote_memory(process_handle, name_ptr_pos, 4) 
        if not name_ptr_data: 
            print("[Debug] Failed to read name pointer at index={} (0x{:X}).Continuing...".format(i, name_ptr_pos)) 
            continue

        name_rva = int.from_bytes(name_ptr_data, byteorder='little', signed=False) 
        if name_rva == 0: 
            continue

        export_name = read_remote_string(process_handle, module_base + name_rva, 128)
        if not export_name: 
            continue


        if export_name.lower() == func_name.lower(): 
            print("[Debug] Match found! {} (index={}) => retrieving function address...".format(export_name, i))

        ord_pos = ord_array + (i * 2) 
        ord_data = read_remote_memory(process_handle, ord_pos, 2) 
        if not ord_data: 
            print("[Debug] Failed to read function ordinal at 0x{:X}".format(ord_pos))
            return 0

        ordinal = int.from_bytes(ord_data, byteorder='little', signed=False) 
        func_pos = func_addr_array + (ordinal * 4) 
        func_data = read_remote_memory(process_handle, func_pos, 4) 
        if not func_data: 
            print("[Debug] Failed to read function address at 0x{:X}".format(func_pos)) 
            return 0


        func_rva = int.from_bytes(func_data, byteorder='little', signed=False) 
        final_address = module_base + func_rva 
        print("[Debug] Final address for {} is 0x{:X}".format(export_name, final_address)) 
        return final_address 

    print("[Debug] '{}' not found among exports.".format(func_name)) 
    return 0