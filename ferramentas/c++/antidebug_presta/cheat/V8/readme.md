```bash
$ g++ -std=gnu++17 -shared -static-libgcc -static-libstdc++   -fno-omit-frame-pointer -O2 -g   -o hook_antidebug_duplo_8vs_x64.dll   hook_antidebug_duplo_8vs.cpp   MinHook/src/buffer.c MinHook/src/trampoline.c MinHook/src/hook.c MinHook/src/hde/hde64.c   -I MinHook/include -D_CRT_SECURE_NO_WARNINGS   -ldbghelp -lpsapi -lntdll -luser32 -lkernel32 -lbcrypt -DAB_BYPASS_THREADHIDE=1
```