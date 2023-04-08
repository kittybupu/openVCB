#include "openVCB.h"
 
#ifndef NDEBUG
# ifndef _DEBUG
#  define _DEBUG
# endif
#endif


/*--------------------------------------------------------------------------------------*/
namespace openVCB::util {

static FILE *log = nullptr;

void logf(UU PRINTF_FORMAT_STRING format, ...)
{
#ifdef _DEBUG
      if (!log)
            return;
      va_list ap;
      va_start(ap, format);
      std::ignore = vfprintf(log, format, ap);
      va_end(ap);
      std::ignore = fputc('\n', log);
      std::ignore = fflush(log);
#endif
}

void logs(UU char const *msg, UU size_t const len)
{
#ifdef _DEBUG
      if (!log)
            return;
      std::ignore = fwrite(msg, 1, len, log);
      std::ignore = fputc('\n', log);
#endif
}

void logs(UU char const *msg)
{
#ifdef _DEBUG
      if (!log)
            return;
      std::ignore = fputs(msg, log);
      std::ignore = fputc('\n', log);
#endif
}

} // namespace openVCB::util
/*--------------------------------------------------------------------------------------*/


#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
# include <Windows.h>

BOOL WINAPI
DllMain(HINSTANCE const inst, DWORD const fdwReason, LPVOID)
{
      using namespace std::literals;

      switch (fdwReason) {
      case DLL_PROCESS_ATTACH: {
# ifdef _DEBUG
            std::ignore = ::_set_abort_behavior(0, _WRITE_ABORT_MSG);
            std::ignore = ::signal(SIGABRT, [](int){});

            std::filesystem::path base;
            {
                  wchar_t fname[1024];
                  if (::GetModuleFileNameW(inst, fname, std::size(fname)) == 0) {
                        ::MessageBoxW(nullptr, L"Error determining openVCB.dll file path.", L"ERROR", MB_OK);
                        ::exit(1);
                  }
                  base = absolute(std::filesystem::path(fname).parent_path());
            }

            auto const path    = base / L"_openVCB.log"sv;
            openVCB::util::log = _wfopen(path.c_str(), L"w");
            if (!openVCB::util::log) {
                  ::MessageBoxW(nullptr, L"Error opening openVCB.dll log file.", L"ERROR", MB_OK);
                  ::exit(1);
            }
# endif
            break;
      }

      case DLL_PROCESS_DETACH:
# ifdef _DEBUG
            if (openVCB::util::log)
                  std::ignore = fclose(openVCB::util::log);
# endif
            break;

      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
            break;

      default:
            std::terminate();
      }

      return TRUE;
}
#endif