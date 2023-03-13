// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"
#include <tbb/spin_mutex.h>

#ifdef _WIN32
# define EXPORT_API __declspec(dllexport)
#else
# define EXPORT_API
#endif

namespace util = openVCB::util;

namespace {
/****************************************************************************************/

using openVCB::Ink, openVCB::Logic;

constexpr double TARGET_DT = 1.0 / 60.0;
constexpr double MIN_DT    = 1.0 / 30.0;

openVCB::Project *proj      = nullptr;
std::thread      *simThread = nullptr;
tbb::spin_mutex   simLock;

float  targetTPS  = 0.0f;
double maxTPS     = 0.0;
bool   run        = true;
bool   breakpoint = false;


[[__gnu__::__hot__]]
void
simFunc()
{
      using clock = std::chrono::high_resolution_clock;
      using std::chrono::duration, std::chrono::milliseconds;

      double tpsEst       = 2.0 / TARGET_DT;
      double desiredTicks = 0.0;
      auto   lastTime     = clock::now();

      while (run) {
            auto curTime = clock::now();
            auto diff    = duration_cast<duration<double>>(curTime - lastTime).count();
            lastTime     = curTime;
            desiredTicks = std::min(desiredTicks + diff * double(targetTPS),
                                    tpsEst * MIN_DT);

            // Find max tick amount we can do
            if (desiredTicks >= 1.0) {
                  double     maxTickAmount = std::max(tpsEst * TARGET_DT, 1.0);
                  auto const tickAmount    = int32_t(std::min(desiredTicks, maxTickAmount));

                  // Aquire lock, simulate, and time
                  simLock.lock();
                  auto t1  = clock::now();
                  auto res = proj->tick(tickAmount, INT64_C(100'000'000));
                  auto t2  = clock::now();
                  simLock.unlock();

                  // Use timings to estimate max possible tps
                  desiredTicks = desiredTicks - res.numTicksProcessed;
                  maxTPS       = res.numTicksProcessed / duration_cast<duration<double>>(t2 - t1).count();
                  if (isfinite(maxTPS))
                        tpsEst = glm::clamp(glm::mix(maxTPS, tpsEst, 0.95), 1.0, 1e8);

                  if (res.breakpoint) {
                        targetTPS    = 0.0f;
                        desiredTicks = 0.0;
                        breakpoint   = true;
                  }
            }

            // Check how much time I got left and sleep until the next check
            curTime = clock::now();
            diff    = duration_cast<duration<double>>(curTime - lastTime).count();
            if (auto const ms = 1000 * (TARGET_DT - diff); ms > 1.05)
                  std::this_thread::sleep_for(milliseconds{uint64_t(ms)});
      }
}


} // namespace
/****************************************************************************************/


extern "C" {

/*
 * Functions to control openVCB simulations
 */

EXPORT_API int64_t
getLineNumber(int const addr)
{
      auto const itr = proj->lineNumbers.find(addr);
      if (itr != proj->lineNumbers.end())
            return itr->second;
      return 0;
}

EXPORT_API size_t
getSymbol(char const *buf, int const size)
{
      auto const itr = proj->assemblySymbols.find({buf, static_cast<size_t>(size)});
      if (itr != proj->assemblySymbols.end())
            return itr->second;
      return 0;
}

EXPORT_API size_t
getNumTicks()
{
      return proj->tickNum;
}

EXPORT_API float
getMaxTPS()
{
      return static_cast<float>(maxTPS);
}

EXPORT_API uintptr_t
getVMemAddress()
{
#ifdef OVCB_BYTE_ORIENTED_VMEM 
      return proj->lastVMemAddr / 4;
#else
      return proj->lastVMemAddr;
#endif
}

EXPORT_API void
setTickRate(float const tps)
{
      targetTPS = std::max(0.0f, tps);
}

EXPORT_API void
tick(int const tick)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->tick(tick);
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
toggleLatch(int const x, int const y)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->toggleLatch(glm::ivec2(x, y));
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
toggleLatchIndex(int const idx)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->toggleLatch(idx);
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
addBreakpoint(int const gid)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->addBreakpoint(gid);
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
removeBreakpoint(int const gid)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->removeBreakpoint(gid);
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API int
pollBreakpoint()
{
      bool const res = breakpoint;
      breakpoint     = false;
      return res;
}

EXPORT_API void
setClockPeriod(uint64_t const period)
{
      proj->clockPeriod = static_cast<uint32_t>(period);
}


/*--------------------------------------------------------------------------------------*/
/*
 * Functions to initialize openVCB
 */

EXPORT_API void
newProject()
{
      proj = new openVCB::Project;
}

EXPORT_API int
initProject()
{
      proj->preprocess();

      // Start sim thread paused
      run       = true;
      simThread = new std::thread(simFunc);

      return proj->numGroups;
}

EXPORT_API void
initVMem(char const *assembly, int const aSize, char *err, int const errSize)
{
      proj->assembly = std::string(assembly, aSize);
      //proj->my_assembleVmem(err, errSize);
      proj->assembleVmem(err, errSize);
}

EXPORT_API void
deleteProject()
{
      run = false;
      if (simThread) {
            simThread->join();
            delete simThread;
            simThread = nullptr;
      }

      if (proj) {
            // These should be managed.
            proj->states = nullptr;
            proj->vmem   = nullptr;
            proj->image  = nullptr;

            delete proj;
            proj = nullptr;
      }
}

/*--------------------------------------------------------------------------------------*/
/*
 * Functions to replace openVCB buffers with managed ones
 */

EXPORT_API void
addInstrumentBuffer(openVCB::InkState *buf, int const bufSize, int const idx)
{
      float const tps = targetTPS;
      targetTPS       = 0;
      simLock.lock();
      proj->instrumentBuffers.push_back({buf, bufSize, idx});
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
setStateMemory(int *data, int const size)
{
      memcpy(data, proj->states, sizeof(*proj->states) * size);
      delete[] proj->states;
      proj->states_is_native = false;
      proj->states           = reinterpret_cast<openVCB::InkState *>(data);
}

EXPORT_API void
setVMemMemory(int *data, int const size)
{
      proj->vmem     = data;
      proj->vmemSize = size;
}

EXPORT_API void
setIndicesMemory(int *data, int const size)
{
      memcpy(data, proj->indexImage, sizeof(*proj->indexImage) * size);
}

EXPORT_API void
setImageMemory(int *data, int const width, int const height)
{
      proj->width  = width;
      proj->height = height;
      proj->image  = reinterpret_cast<openVCB::InkPixel *>(data);
}

EXPORT_API void
setDecoMemory(_Inout_ int *__restrict const       indices,
              _In_ UU int const                   indLen,
              _In_    int const *__restrict const col,
              _In_ UU int const                   colLen)
{
      std::queue<glm::ivec3> queue;
      std::vector            visited(size_t(proj->width * proj->height), false);

      for (int32_t y = 0; y < proj->height; ++y) {
            for (int32_t x = 0; x < proj->width; ++x) {
                  int32_t const idx = x + y * proj->width;
                  indices[idx]      = -1;

                  if (uint32_t(col[idx]) != UINT32_MAX)
                        continue;

                  auto const ink = setOff(proj->image[idx].ink);
                  if (util::eq_any(ink, Ink::LatchOff, Ink::LedOff)) {
                        queue.push({x, y, proj->indexImage[idx]});
                        visited[idx] = true;
                  }
            }
      }

      while (!queue.empty()) {
            auto pos = queue.front();
            queue.pop();

            indices[pos.x + pos.y * proj->width] = pos.z;

            for (auto const &neighbor : openVCB::fourNeighbors) {
                  glm::ivec2 const np = static_cast<glm::ivec2>(pos) + neighbor;
                  if (np.x < 0 || np.x >= proj->width || np.y < 0 || np.y >= proj->height)
                        continue;

                  int const nidx = np.x + np.y * proj->width;
                  if (visited[nidx])
                        continue;

                  visited[nidx] = true;

                  if (static_cast<uint32_t>(col[nidx]) != UINT32_MAX)
                        continue;
                  queue.push({np.x, np.y, pos.z});
            }
      }
}

/*--------------------------------------------------------------------------------------*/
/*
 * Functions to configure openVCB
 */

EXPORT_API void
getGroupStats(int *numGroups, int *numConnections)
{
      *numGroups      = proj->numGroups;
      *numConnections = proj->writeMap.nnz;
}

EXPORT_API void
setInterface(openVCB::LatchInterface const addr, openVCB::LatchInterface const data)
{
      proj->vmAddr = addr;
      proj->vmData = data;
}


/*--------------------------------------------------------------------------------------*/


static openVCB::string_array *global_errors_reference = nullptr;
static bool                   inhibitStupidity        = false;


EXPORT_API char **
openVCB_compile_and_run_project(size_t *numErrors, int *stateSize)
{
      proj->preprocess();

      *numErrors = proj->error_messages->qty;
      if (proj->error_messages->qty > 0) {
            *stateSize              = -1;
            global_errors_reference = proj->error_messages;
            proj->error_messages    = nullptr;

            inhibitStupidity = true;
            delete proj;
            proj = nullptr;
            inhibitStupidity = false;

            return global_errors_reference->list;
      }

      delete proj->error_messages;
      proj->error_messages = nullptr;
      *stateSize = proj->numGroups;

      // Start sim thread paused
      run        = true;
      simThread  = new std::thread(simFunc);
      return nullptr;
}

EXPORT_API void 
openVCB_free_error_strings() noexcept
{
      if (!inhibitStupidity) {
            delete global_errors_reference;
            global_errors_reference = nullptr;
      }
}


/*--------------------------------------------------------------------------------------*/
} // extern "C"
