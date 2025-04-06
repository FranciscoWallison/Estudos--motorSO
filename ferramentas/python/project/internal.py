from ctypes import wintypes 
from lib import injector, utility, winapi


def runCode(pid, codeString): 
    base = utility.modBase (pid, 'python310.dll')

    handle = wintypes.HANDLE ( 
        winapi.OpenProcess (winapi.PROCESS_ALL_ACCESS, 0, wintypes. DWORD (pid)) 
    ) 

    Py_InitializeEx = wintypes.LPVOID(utility.getRemoteProcAddress(handle, base,"Py_InitializeEx")) 
    PyRun_SimpleString = wintypes.LPVOID(utility.getRemoteProcAddress(handle, base,"PyRun_SimpleString"))
    
    #Py_InitializeEx(0) 
    pyinit = wintypes.HANDLE ( 
        winapi.CreateRemoteThread(handle, None, 0, Py_InitializeEx, 0, 0, None) 
    )
    winapi.WaitForSingleObject(pyinit, wintypes.DWORD(10000))


    # Write code to target process. 
    codeString = codeString.encode('utf-8') + b'\x00' 
    codeAddress = wintypes.LPVOID ( 
        winapi.VirtualAllocEx(handle, winapi.NULL, (len(codeString) + 1), winapi. 
        MEM_RESERVE | winapi.MEM_COMMIT, winapi.PAGE_EXECUTE_READWRITE)
    )

    if (codeAddress.value != None): 
        codeWritten = wintypes.BOOL(winapi.WriteProcessMemory(handle, codeAddress, codeString, len(codeString), winapi.NULL) )


        if (codeWritten.value): 
            # PyRun_SimpleString(codeString) 
            winapi.CreateRemoteThread(handle, None, 0, PyRun_SimpleString, 
            codeAddress, 0, None) 
    winapi.CloseHandle(handle)


def loadCode(path): 
    return (open(path).read()) 

pid = utility.getPid('sauerbraten.exe')

if (pid != None): 
    injector.inject(b'python310.dll', pid) 
    codeString = loadCode('code.py')
    runCode(pid, codeString)