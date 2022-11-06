
#include "openVCB.hh"
#include <chrono>
#include <memory>
#include <vector>

#if __cplusplus < 202002L || _MSVC_LANG < 202002L
# error "Wrong version clang, you moron."
#endif

namespace lazy {

using clock = std::chrono::high_resolution_clock;

struct tagged_time_point
{
      using time_point = clock::time_point;

      char const *const name;
      time_point const  time;

      tagged_time_point(char const *aname, time_point const atime)
          : name(aname), time(atime)
      {}

      constexpr auto operator-(tagged_time_point const &other) const noexcept
      {
            return time - other.time;
      }

      template <typename Ty>
      constexpr auto operator-(Ty const &other) const noexcept
      {
            return time - other;
      }
};

template <typename Ty>
    requires std::derived_from<Ty, std::chrono::duration<typename Ty::rep, typename Ty::period>>
constexpr double dur_to_dbl(Ty const &dur) noexcept
{
      return std::chrono::duration_cast<std::chrono::duration<double>>(dur).count();
}

template <typename Elem, size_t Num>
constexpr auto fwrite_l(Elem const (&buf)[Num], FILE *dest)
{
      return ::fwrite(buf, sizeof(Elem), Num - 1, dest);
}

} // namespace lazy


using namespace std::literals;
static constexpr bool     useGorder  = false;
static constexpr unsigned numTicks   = 100'000;
static constexpr double   numTicks_d = numTicks;


int
main()
{
      using namespace lazy;

      FILE *realStdout = stdout;
      auto  proj       = std::make_unique<openVCB::Project>();
      auto  times      = std::vector<tagged_time_point>();
      times.reserve(100);

      // Read .vcb file
      times.emplace_back("File read", clock::now());
      proj->readFromVCB("sampleProject.vcb");

      times.emplace_back("Project preprocess", clock::now());
      proj->preprocess(useGorder);

      times.emplace_back("VMem assembly", clock::now());
      proj->assembleVmem();

      ::printf("Loaded %d groups and %d connections.\n"
               "Simulating %u ticks into the future...\n",
               proj->numGroups, proj->writeMap.nnz, numTicks);

      // Advance 1 million ticks
      times.emplace_back("Advance 1 million ticks", clock::now());
      proj->tick(numTicks);

      times.emplace_back("End", clock::now());
      fwrite_l("\nDone.\n\n", realStdout);
      size_t const size = times.size();

      for (size_t i = 0; i < size - 1; ++i) {
            double const elapsed = dur_to_dbl(times[i + 1] - times[i]);
            ::printf("%s took %.3f seconds.\n", times[i].name, elapsed);
      }

      double const simTime = dur_to_dbl(times[size - 1] - times[size - 2]);
      ::printf("Average TPS: %.1f\n\n", numTicks_d / simTime);

      proj->dumpVMemToText("vmemDump.txt");
      fwrite_l("Final VMem contents dumped to vmemDump.txt!\n", realStdout);
}