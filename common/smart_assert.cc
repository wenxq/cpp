
#include "smart_assert.h"
#include "integer_cast.h"
#include "slice.h"

#ifndef _MSC_VER
#include <execinfo.h>
#include <cxxabi.h>

void fill_stack_trace(std::ostream& os)
{
    const int len = 200;
    void* buffer[len];
    int nptrs = ::backtrace(buffer, len);
    if (nptrs == 0)
    {
        os << "<No stack trace>" << std::endl;
        return ;
    }

    char** strings = ::backtrace_symbols(buffer, nptrs);
    if (!strings)
    {
        os << "<Unknown error: backtrace_symbols returned NULL>" << std::endl;
        return ;
    }

    std::string tmp;
    for (int i = 0; i < nptrs; ++i)
    {
        Slice mangled = Slice(strings[i], integer_cast<int>(strlen(strings[i])));
        int beg = mangled.find_first('(');
        int end = mangled.find_first('+', beg);
        if (beg == Slice::NPOS || end == Slice::NPOS)
        {
            os << mangled << '\n';
            continue;
        }
        ++beg;

        mangled.substr(beg, end - beg).copy_to(&tmp);

        int status;
        char* s = abi::__cxa_demangle(tmp.c_str(), NULL, 0, &status);
        if (status != 0)
        {
            os << mangled << '\n';
            continue;
        }
        else
        {
            Slice demangle(s, integer_cast<int>(strlen(s)));
            if (demangle.find_first("fill_stack_trace") != Slice::NPOS)
            {
                continue;
            }
            os << mangled.substr(0, beg) << demangle << mangled.substr(end) << '\n';
            ::free(s);
        }
    }
    os << std::flush;
    ::free(strings);
}

#else

#include <windows.h>
#include <dbghelp.h>
#include <cstdlib>
#pragma comment (lib, "dbghelp.lib")
#define MAX_NAME_LENGTH 256

void fill_stack_trace(std::ostream& os)
{
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)calloc(1, sizeof(SYMBOL_INFO) + (MAX_NAME_LENGTH - 1) * sizeof(TCHAR));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO); 
    symbol->MaxNameLen   = MAX_NAME_LENGTH;

    IMAGEHLP_LINE64 imagehelp;  
    memset(&imagehelp, 0, sizeof(IMAGEHLP_LINE64));
    imagehelp.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 frame;

    memset(&frame, 0, sizeof(STACKFRAME64));
    frame.AddrPC.Mode    = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;

    DWORD machine_type = IMAGE_FILE_MACHINE_I386; 

#ifdef _M_IX86
    machine_type = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset    = context.Eip;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrFrame.Offset = context.Ebp;
#elif _M_X64
    machine_type = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrFrame.Offset = context.Rsp;
#elif _M_IA64
    machine_type = IMAGE_FILE_MACHINE_IA64;
    frame.AddrPC.Offset    = context.StIIP;
    frame.AddrStack.Offset = context.IntSp;
    frame.AddrFrame.Offset = context.IntSp;
    frame.AddrBStore.Offset = context.RsBSP;
#else
    #error "Platform not supported!"
#endif

    HANDLE process = GetCurrentProcess();
    HANDLE thread  = GetCurrentThread();

    if (!SymInitialize(process, NULL, TRUE))
    {
        os << "<Initialize dbghelp library failed, errcode=" << GetLastError() << ">" << std::endl;
        return ;
    }
 
    while (StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &frame,
            &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
    {
        if (frame.AddrFrame.Offset == 0)
        { 
            break;
        }

        const char* func_name = "Unknown";
        if (SymFromAddr(process, frame.AddrPC.Offset, NULL, symbol))
        {
            func_name = symbol->Name;
            Slice demangle(func_name, integer_cast<int>(strlen(func_name)));
            if (demangle.find_first("fill_stack_trace") != Slice::NPOS)
            {
                continue;
            }
        }

        DWORD displacement;
        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &imagehelp))
        {
            os << func_name << "@" << imagehelp.FileName << ":" << imagehelp.LineNumber << "\n";
        }
        else if (GetLastError() == 0x1E7)
        { 
            os << "<No debug symbol loaded for this function.>\n";
			break;
        }
    }
    os << std::flush;

    SymCleanup(process); 
    free(symbol);
}

#endif // _MSC_VER
