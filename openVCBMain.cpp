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
      (void)vfprintf(log, format, ap);
      va_end(ap);
      (void)fputc('\n', log);
      (void)fflush(log);
#endif
}

void logs(UU char const *msg, UU size_t const len)
{
#ifdef _DEBUG
      if (!log)
            return;
      (void)fwrite(msg, 1, len, log);
      (void)fputc('\n', log);
#endif
}

void logs(UU char const *msg)
{
#ifdef _DEBUG
      if (!log)
            return;
      (void)fputs(msg, log);
      (void)fputc('\n', log);
#endif
}

} // namespace openVCB::util
/*--------------------------------------------------------------------------------------*/


#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
# endif
# include <Windows.h>

BOOL WINAPI
DllMain(HINSTANCE const inst, DWORD const fdwReason, LPVOID)
{
      using namespace std::literals;

      switch (fdwReason) {
      case DLL_PROCESS_ATTACH: {
# ifdef _DEBUG
            (void)::_set_abort_behavior(0, _WRITE_ABORT_MSG);
            (void)::signal(SIGABRT, [](int){});

            wchar_t fname[2048];
            if (::GetModuleFileNameW(inst, fname, std::size(fname)) == 0) {
                  ::MessageBoxW(nullptr, L"Error determining openVCB.dll file path.", L"ERROR", MB_OK);
                  ::exit(1);
            }
            auto path = absolute(std::filesystem::path(fname)).parent_path();
            path     /= L"OpenVCB.log"sv;

            if (_wfopen_s(&openVCB::util::log, path.c_str(), L"w") != 0) {
                  ::MessageBoxW(nullptr, L"Error opening openVCB.dll log file.", L"ERROR", MB_OK);
                  ::exit(1);
            }
# endif
            break;
      }

      case DLL_PROCESS_DETACH:
# ifdef _DEBUG
            if (openVCB::util::log)
                  (void)fclose(openVCB::util::log);
            break;
# endif

      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      default:
            break;
      }

      return TRUE;
}
#endif