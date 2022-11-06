#include "openVCB.hh"
#include "utils.hh"

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#if defined _WIN32
#  define EXPORT_API __declspec(dllexport)
#else
#  define EXPORT_API
#endif

constexpr float TARGET_DT = 1.f / 60;
constexpr float MIN_DT    = 1.f / 30;

openVCB::Project *proj      = nullptr;
std::thread      *simThread = nullptr;
std::mutex        simLock;

float  targetTPS = 0;
double maxTPS    = 0;
bool   run       = true;


void
simFunc()
{
      double tpsEst       = 2 / TARGET_DT;
      double desiredTicks = 0;
      auto   lastTime     = std::chrono::high_resolution_clock::now();

      while (run) {
            auto curTime = std::chrono::high_resolution_clock::now();
            desiredTicks = std::min(desiredTicks + duration_cast<std::chrono::duration<double>>(curTime - lastTime).count() * targetTPS, tpsEst * MIN_DT);
            lastTime = curTime;

            // Find max tick amount we can do
            if (desiredTicks >= 1.) {
                  double maxTickAmount = std::max(tpsEst * TARGET_DT, 1.);
                  int    tickAmount    = (int)std::min(desiredTicks, maxTickAmount);

                  // Aquire lock, simulate, and time
                  simLock.lock();
                  auto s     = std::chrono::high_resolution_clock::now();
                  tickAmount = proj->tick(tickAmount, 100000000ll).numTicksProcessed;
                  auto e     = std::chrono::high_resolution_clock::now();
                  simLock.unlock();

                  // Use timings to estimate max possible tps
                  desiredTicks = desiredTicks - tickAmount;
                  maxTPS = tickAmount / duration_cast<std::chrono::duration<double>>(e - s).count();
                  if (isfinite(maxTPS))
                        tpsEst = glm::clamp(glm::mix(maxTPS, tpsEst, 0.95), 1., 1e8);
            }

            // Check how much time I got left and sleep until the next check
            curTime   = std::chrono::high_resolution_clock::now();
            double ms = 1000 * (TARGET_DT - duration_cast<std::chrono::duration<double>>(curTime - lastTime).count());

            if (ms > 1.05)
                  std::this_thread::sleep_for(std::chrono::milliseconds((int)ms));
      }
}

extern "C" {
/*
 * Functions to control openVCB simulations
 */

EXPORT_API size_t
getSymbol(char *buff, int size)
{
      auto itr = proj->assemblySymbols.find(std::string(buff));
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
      return (float)maxTPS;
}

EXPORT_API int
getVMemAddress()
{
      return proj->lastVMemAddr;
}

EXPORT_API void
setTickRate(float tps)
{
      targetTPS = std::max(0.f, tps);
}

EXPORT_API void
tick(int tick)
{
      float tps = targetTPS;
      targetTPS = 0;
      simLock.lock();
      proj->tick(tick);
      simLock.unlock();
      targetTPS = tps;
}

EXPORT_API void
toggleLatch(int x, int y)
{
      float tps = targetTPS;
      targetTPS = 0;
      simLock.lock();
      proj->toggleLatch(glm::ivec2(x, y));
      simLock.unlock();
      targetTPS = tps;
}

/*
 * Functions to initialize openVCB
 */

EXPORT_API void
newProject()
{
      std::wcerr << L"openVCB: https://github.com/kittybupu/openVCB\nopenVCB: Jerry#1058\n";
      proj = new openVCB::Project;
}

EXPORT_API int
initProject()
{
      proj->preprocess(false);

      // Start sim thread paused
      run       = true;
      simThread = new std::thread(simFunc);

      return proj->numGroups;
}

EXPORT_API void
initVMem(char *assembly, int const aSize, char *err, int const errSize)
{
      proj->assembly = std::string(assembly, aSize);
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

/*
 * Functions to replace openVCB buffers with managed ones
 */

EXPORT_API void
setStateMemory(int *data, int size)
{
      memcpy(data, proj->states, sizeof(int) * size);
      delete[] proj->states;
      proj->states = (openVCB::InkState *)data;
}

EXPORT_API void
addInstrumentBuffer(openVCB::InstrumentBuffer buffer)
{
      proj->instrumentBuffers.push_back(buffer);
}

EXPORT_API void
setVMemMemory(int *data, int size)
{
      proj->vmem     = data;
      proj->vmemSize = size;
}

EXPORT_API void
setIndicesMemory(int *data, int size)
{
      memcpy(data, proj->indexImage, sizeof(int) * size);
}

EXPORT_API void
setImageMemory(int *data, int width, int height)
{
      proj->width  = width;
      proj->height = height;
      proj->image  = (openVCB::InkPixel *)data;
}

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
setInterface(openVCB::LatchInterface addr, openVCB::LatchInterface data)
{
      proj->vmAddr = addr;
      proj->vmData = data;
}


} // extern "C"
/****************************************************************************************/


#ifdef MAIN_IN_A_DLL_QUESTIONMARK
int
main()
{
      printf("openVCB: https://github.com/kittybupu/openVCB\n");
}
#endif