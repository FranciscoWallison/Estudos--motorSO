import ctypes
import win32con

api = ctypes.WinDLL('Kernel32', use_last_error=True)

OpenProcess = api.OpenProcess
CloseHandle = api.CloseHandle
VirtualAllocEx = api.VirtualAllocEx  # Corrigido
WriteProcessMemory = api.WriteProcessMemory
CreateRemoteThread = api.CreateRemoteThread  # Corrigido
LoadLibraryA = api.LoadLibraryA
CreateToolhelp32Snapshot = api.CreateToolhelp32Snapshot
Module32First = api.Module32First
Module32Next = api.Module32Next
WaitForSingleObject = api.WaitForSingleObject

# Constants & Structures 
PROCESS_ALL_ACCESS = win32con.PROCESS_ALL_ACCESS 
MEM_COMMIT = win32con.MEM_COMMIT 
MEM_RESERVE = win32con.MEM_RESERVE 
PAGE_EXECUTE_READWRITE = win32con.PAGE_EXECUTE_READWRITE 
NULL = win32con.NULL 
INVALID_HANDLE_VALUE = -1 
TH32CS_SNAPMODULE = 0x8 
TH32CS_SNAPMODULE32 = 0x10 


class MODULEENTRY32 (ctypes.Structure): 
    _fields_ = [ ('dwSize', ctypes.wintypes.DWORD), 
        ('th32ModuleID', ctypes.wintypes.DWORD), 
        ('th32ProcessID', ctypes.wintypes.DWORD), 
        ('GlblcntUsage', ctypes.wintypes.DWORD), 
        ('ProccntUsage', ctypes.wintypes.DWORD), 
        ('modBaseAddr', ctypes.POINTER(ctypes.wintypes.BYTE)), 
        ('modBaseSize', ctypes.wintypes.DWORD), 
        ('hModule', ctypes.wintypes.HMODULE), 
        ('szModule', ctypes.c_char * 256), 
        ('szExePath', ctypes.c_char * 260)]