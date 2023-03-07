#include "openVCB.h"

namespace openVCB::util {

static FILE *log = nullptr;

int
logf(PRINTF_FORMAT_STRING format, ...)
{
      va_list ap;
      va_start(ap, format);
      int const ret = ::vfprintf(log, format, ap);
      va_end(ap);
      return ret;
}

} // namespace openVCB::util

#ifdef _WIN32
# include <Windows.h>

BOOL WINAPI
DllMain(HINSTANCE const inst, DWORD const fdwReason, LPVOID)
{
      using namespace std::literals;

      // Perform actions based on the reason for calling.
      switch (fdwReason) {
      case DLL_PROCESS_ATTACH: {
            std::filesystem::path base;
            {
                  wchar_t fname[1024];
                  if (::GetModuleFileNameW(inst, fname, std::size(fname)) == 0)
                      break;
                  base = absolute(std::filesystem::path(fname).parent_path());
            }
            auto const path    = base / L"_openVCB.log"sv;
            openVCB::util::log = _wfopen(path.c_str(), L"wt");
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