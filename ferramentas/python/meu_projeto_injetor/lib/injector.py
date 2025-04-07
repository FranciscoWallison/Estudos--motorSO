import ctypes
import ctypes.wintypes as wintypes
from libs import winapi


def printError(desc=""): 
    print(desc + str(ctypes.get_last_error()))

def inject(dllPath, pid): 
    # Open handle to target process. 
    handle = wintypes.HANDLE ( 
        winapi. OpenProcess (winapi.PROCESS_ALL_ACCESS, wintypes. BOOL (0), wintypes. DWORD 
        (pid)) 
    )

    # Allocate space for DLL path string. 
    dllPathAddress = wintypes.LPVOID( 
        winapi.VirtualAllocEx (handle, winapi.NULL, (len(dllPath) + 1), winapi.MEM_RESERVE | winapi.MEM_COMMIT, winapi.PAGE_EXECUTE_READWRITE) 
    )

    if (dllPathAddress.value != None): 
        #Write path to memory. 
        pathWritten = wintypes.BOOL(
            winapi.WriteProcessMemory(handle, dllPathAddress, dllPath, len(dllPath), winapi.NULL) 
        )
        if (pathWritten.value):
            #Create remote thread.
            remoteThread = wintypes.HANDLE(
                winapi.CreateRemoteThread(handle, winapi.NULL, winapi.NULL, winapi.LoadLibraryA, dllPathAddress, wintypes.DWORD(0), winapi.NULL) 
            ) 
            winapi.WaitForSingleObject(remoteThread, wintypes.DWORD(10000)) 
            if (remoteThread.value == None): 
                printError("Error RThread: ") 
        else:
            printError("Error Write: ")

    else:
        printError("Error Alloc: ")

    # Close handle.
    winapi.CloseHandle(handle)