#include "openVCB.h"


/*--------------------------------------------------------------------------------------*/
namespace openVCB::util {

static FILE *log = nullptr;

void logf(PRINTF_FORMAT_STRING format, ...)
{
      if (!log)
            return;
      va_list ap;
      va_start(ap, format);
      std::ignore = ::vfprintf(log, format, ap);
      va_end(ap);
      std::ignore = fputc('\n', log);
      std::ignore = fflush(log);
}

void logs(char const *msg, size_t const len)
{
      fwrite(msg, 1, len, log);
      fputc('\n', log);
}

void logs(char const *msg)
{
      fputs(msg, log);
      fputc('\n', log);
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
            std::ignore = ::_set_abort_behavior(0, _WRITE_ABORT_MSG);
            std::ignore = ::signal(SIGABRT, SIG_IGN);

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

            break;
      }

      case DLL_PROCESS_DETACH:
            if (openVCB::util::log)
                  std::ignore = fclose(openVCB::util::log);
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