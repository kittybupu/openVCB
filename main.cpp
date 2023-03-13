#include "openVCB.h"
#include <chrono>
#include <memory>
#include <vector>

#ifndef STRINGIZE
#  define STRINGIZE_HELPER(x) #x
#  define STRINGIZE(x)        STRINGIZE_HELPER(x)
#endif
#define UINT64_CAST_HELPER(x) UINT64_C(x)
#define UINT64_CAST(x)        UINT64_CAST_HELPER(x)

#if __cplusplus < 202002L || _MSVC_LANG < 202002L
# error "Wrong version clang, you moron."
#endif

namespace lazy
{
template <typename T> concept HasDuration = requires { typename T::duration; };
template <typename T> concept HasRep = requires { typename T::rep; };

template <typename Tp>
      requires HasDuration<Tp> && HasRep<Tp>
class tagged_time_point {
      using this_type = tagged_time_point<Tp>;
      using time_point = Tp;
      using duration = typename Tp::duration;
      using rep = typename Tp::rep;

      static constexpr bool arith_ = std::is_arithmetic_v<rep>;

      std::string_view name_{};
      time_point const time_{};

public:
      tagged_time_point() = default;

      tagged_time_point(std::string_view const name, time_point time)
            : name_(name),
              time_(std::move(time))
      {
      }

      template <typename T>
      constexpr auto operator-(
            tagged_time_point<T> const &other) const noexcept(arith_ && other.arith_)
      {
            return time_ - other.time_;
      }

      ND constexpr std::string_view const &name() const & noexcept { return name_; }
      ND constexpr char const *name_c() const noexcept { return name_.data(); }

      ND auto const &tp() const & noexcept { return time_; }
      ND auto &      tp() & noexcept { return time_; }
};

template <typename Ty>
      requires std::derived_from<Ty, std::chrono::duration<
                                       typename Ty::rep, typename Ty::period>>
constexpr double
dur_to_dbl(Ty const &dur) noexcept
{
      return std::chrono::duration_cast<std::chrono::duration<double>>(dur).count();
}

template <typename Elem, size_t Num>
constexpr auto
fwrite_l(Elem const (&buf)[Num], FILE *dest)
{
      return ::fwrite(buf, sizeof(Elem), Num - 1, dest);
}
} // namespace lazy


#define NUMTICKS   10'000'000
#define NUMTICKS_S STRINGIZE(NUMTICKS)

using namespace std::literals;
static constexpr uint64_t numTicks   = UINT64_CAST(NUMTICKS);
static constexpr double   numTicks_d = numTicks;

int
main()
{
      using clock = std::chrono::high_resolution_clock;
      using lazy::tagged_time_point, lazy::fwrite_l, lazy::dur_to_dbl;

      FILE *realStdout = stdout;
      auto  proj       = std::make_unique<openVCB::Project>();
      auto  times      = std::vector<tagged_time_point<clock::time_point>>();
      times.reserve(100);

      // Read .vcb file
      times.emplace_back("File read"sv, clock::now());
      proj->readFromVCB("sampleProject.vcb");

      times.emplace_back("Project preprocess"sv, clock::now());
      proj->preprocess();

      times.emplace_back("VMem assembly"sv, clock::now());
      proj->assembleVmem();

      ::printf("Loaded %d groups and %d connections.\n"
               "Simulating " NUMTICKS_S " ticks into the future...\n",
               proj->numGroups, proj->writeMap.nnz);

      // Advance 1 million ticks
      times.emplace_back("Advance " NUMTICKS_S " ticks", clock::now());
      proj->tick(numTicks);

      times.emplace_back("End", clock::now());
      fwrite_l("\nDone.\n\n", realStdout);
      size_t const size = times.size();

      for (size_t i = 0; i < size - 1; ++i) {
            double const elapsed = dur_to_dbl(times[i + 1] - times[i]);
            ::printf("%s took %.3f seconds.\n", times[i].name_c(), elapsed);
      }

      double const simTime = dur_to_dbl(times[size - 1] - times[size - 2]);
      ::printf("Average TPS: %.1f\n\n", numTicks_d / simTime);

      proj->dumpVMemToText("vmemDump.txt");
      fwrite_l("Final VMem contents dumped to vmemDump.txt!\n", realStdout);

      return 0;
}
