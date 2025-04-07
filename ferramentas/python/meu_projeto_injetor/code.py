import ctypes
import win32con

# Load vcruntime140.dll for memcpy 
vcruntime = ctypes.WinDLL("vcruntime140.dll") 
c_memcpy = vcruntime.memcpy 
c_memcpy.restype = ctypes.c_void_p
c_memcpy.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]


# Create functions for reading and writing memory 
def readMemory (address, size): 
    buffer = ctypes.create_string_buffer(size) 
    c_memcpy(buffer, ctypes.c_void_p(address), size) 
    return buffer.raw

def writeMemory (address, data, size): 
    c_memcpy(ctypes.c_void_p(address), data, size)

health_address = 0x026CA0E0

# Read the current health 
current_health_data = readMemory (health_address, 4) 
current_health = int.from_bytes(current_health_data, byteorder='little')

# Show current health in a message box 
MessageBoxA = ctypes.windll.user32.MessageBoxA 
health_message = f"Your health is: {current_health}".encode('utf-8') 
MessageBoxA(0, ctypes.c_char_p(health_message), ctypes.c_char_p(b"Health"), win32con.MB_OK)