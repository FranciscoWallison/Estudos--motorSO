#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>

std::vector<DWORD> listThreadsOf(DWORD pid);
bool suspendThreadById(DWORD tid, HANDLE& outHandle, DWORD& outErr);
bool resumeAndClose(HANDLE& hThread);  // resume se aberto e fecha
