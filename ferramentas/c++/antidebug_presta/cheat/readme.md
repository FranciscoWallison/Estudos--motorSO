g++ -o injector.exe injector.cpp -static-libgcc -static-libstdc++ -lkernel32


g++ -shared -static-libgcc -static-libstdc++ -o hook_antidebug_duplo_x64.dll     hook_antidebug_duplo.cpp     MinHook/src/buffer.c MinHook/src/trampoline.c MinHook/src/hook.c MinHook/src/hde/hde64.c     -I MinHook/include -D _CRT_SECURE_NO_WARNINGS     -luser32 -lkernel32


# v4

g++ -std=gnu++17 -shared -static-libgcc -static-libstdc++ -D__USE_MINGW_ANSI_STDIO=1 \
  -o hook_antidebug_duplo_4vs_x64.dll hook_antidebug_duplo_4vs.cpp \
  MinHook/src/buffer.c MinHook/src/trampoline.c MinHook/src/hook.c MinHook/src/hde/hde64.c \
  -I MinHook/include -D _CRT_SECURE_NO_WARNINGS \
  -luser32 -lkernel32

