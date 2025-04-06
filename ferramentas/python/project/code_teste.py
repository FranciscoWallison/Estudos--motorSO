import ctypes
import win32con


# Retrieve the MessageBoxA function from the Windows API's User32 library. 
MessageBoxA = ctypes.windll.user32.MessageBoxA

# Our window handle is NULL. 
# Python uses UTF-8 encoding, our character pointer type is c_char_p. Value should be bytes. 
# Our message box type is the 'MB_OK' constant. 
hWnd = 0 
lpText = ctypes.c_char_p(b"Hello from internal python :)") 
lpCaption = ctypes.c_char_p(b"Success")
uType = win32con.MB_OK

#Call the function with our arguments. 
MessageBoxA(hWnd, lpText, lpCaption, uType)