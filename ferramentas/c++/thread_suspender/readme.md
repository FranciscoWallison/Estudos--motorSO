cd d/projeto_Meus/GC/thread_suspender/

g++ -std=c++17 -O2 -municode \
  src/main.cpp src/privilege.cpp src/process_utils.cpp src/thread_utils.cpp src/ntdll_utils.cpp src/memory_utils.cpp\
  -I include -o watcher.exe -lntdll -ladvapi32


g++ -std=c++17 -O2 -m64 -municode src/main.cpp src/privilege.cpp src/process_utils.cpp src/thread_utils.cpp src/ntdll_utils.cpp -I include -o watcher_64-bit.exe -ladvapi32 && g++ -std=c++17 -O2 -m32 -municode src/main.cpp src/privilege.cpp src/process_utils.cpp src/thread_utils.cpp src/ntdll_utils.cpp -I include -o watcher_32-bit.exe -ladvapi32

g++ -std=c++17 -m64 -municode src/main.cpp src/privilege.cpp src/process_utils.cpp src/thread_utils.cpp src/ntdll_utils.cpp src/memory_utils.cpp -I include -o watcher_64-bit.exe -ladvapi32
