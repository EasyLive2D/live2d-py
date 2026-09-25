#include "Debug.hpp"

#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#include <dbghelp.h>
#include <mutex>

#pragma comment(lib, "dbghelp.lib")
#endif

namespace Live2D {
namespace Debug {
    
#ifdef _WIN32

// dbghelp 不是线程安全的，全局加锁
static std::mutex g_dbghelp_mutex;

// 只初始化一次
static void ensure_sym_initialized()
{
    static bool inited = false;
    if (!inited) {
        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
        SymInitialize(process, NULL, TRUE);
        inited = true;
    }
}

void PrintStackWithLines(const char* tag)
{
    std::lock_guard<std::mutex> lock(g_dbghelp_mutex);

    ensure_sym_initialized();

    HANDLE process = GetCurrentProcess();

    void* frames[64];
    USHORT n = CaptureStackBackTrace(0, 64, frames, NULL);

    // 每帧的符号缓冲区
    char sym_buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)sym_buf;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    fprintf(stderr, "===== stack trace: %s =====\n", tag);
    for (USHORT i = 0; i < n; i++) {
        DWORD64 addr = (DWORD64)frames[i];
        DWORD64 displacement = 0;
        DWORD line_disp = 0;

        const char* sym_name = "???";
        if (SymFromAddr(process, addr, &displacement, symbol)) {
            sym_name = symbol->Name;
        }

        const char* file = "???";
        DWORD line_num = 0;
        if (SymGetLineFromAddr64(process, addr, &line_disp, &line)) {
            file = line.FileName;
            line_num = line.LineNumber;
        }

        fprintf(stderr,
                "  [%2d] %s  (%s:%lu)  +0x%llx\n",
                i,
                sym_name,
                file,
                line_num - 1,
                (unsigned long long)displacement);
    }
    fprintf(stderr, "==================================\n");
}
#else
void PrintStackWithLines() {}
#endif
}
}