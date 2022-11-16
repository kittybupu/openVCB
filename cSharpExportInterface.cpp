#include <stdio.h>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <queue>

#include <tbb/spin_mutex.h>

#include "openVCB/openVCB.h"

#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

using namespace openVCB;
using namespace std;
using namespace std::chrono;

const float TARGET_DT = 1.f / 60;
const float MIN_DT = 1.f / 30;

Project* proj = nullptr;
thread* simThread = nullptr;
tbb::spin_mutex simLock;

float targetTPS = 0;
double maxTPS = 0;
bool run = true;
bool breakpoint = false;

void simFunc() {
	double tpsEst = 2 / TARGET_DT;
	double desiredTicks = 0;
	auto lastTime = high_resolution_clock::now();

	while (run) {
		auto curTime = high_resolution_clock::now();
		desiredTicks = min(desiredTicks + duration_cast<duration<double>>(curTime - lastTime).count() * targetTPS, tpsEst * MIN_DT);
		lastTime = curTime;

		// Find max tick amount we can do
		if (desiredTicks >= 1.) {
			double maxTickAmount = max(tpsEst * TARGET_DT, 1.);
			int tickAmount = (int)min(desiredTicks, maxTickAmount);

			// Aquire lock, simulate, and time
			simLock.lock();
			auto s = high_resolution_clock::now();
			auto res = proj->tick(tickAmount, 100000000ll);
			auto e = high_resolution_clock::now();
			simLock.unlock();

			// Use timings to estimate max possible tps
			desiredTicks = desiredTicks - res.numTicksProcessed;
			maxTPS = res.numTicksProcessed / duration_cast<duration<double>>(e - s).count();
			if (isfinite(maxTPS))
				tpsEst = glm::clamp(glm::mix(maxTPS, tpsEst, 0.95), 1., 1e8);

			if (res.breakpoint) {
				targetTPS = 0;
				desiredTicks = 0;
				breakpoint = true;
			}
		}

		// Check how much time I got left and sleep until the next check
		curTime = high_resolution_clock::now();
		double ms = 1000 * (TARGET_DT - duration_cast<duration<double>>(curTime - lastTime).count());
		if (ms > 1.05)
			std::this_thread::sleep_for(milliseconds((int)ms));
	}
}

extern "C" {
	/*
	* Functions to control openVCB simulations
	*/

	EXPORT_API int getLineNumber(int addr) {
		auto itr = proj->lineNumbers.find(addr);
		if (itr != proj->lineNumbers.end())
			return itr->second;
		return 0;
	}

	EXPORT_API size_t getSymbol(char* buff, int size) {
		auto itr = proj->assemblySymbols.find(string(buff));
		if (itr != proj->assemblySymbols.end())
			return itr->second;
		return 0;
	}

	EXPORT_API size_t getNumTicks() {
		return proj->tickNum;
	}

	EXPORT_API float getMaxTPS() {
		return (float)maxTPS;
	}

	EXPORT_API int getVMemAddress() {
		return proj->lastVMemAddr;
	}

	EXPORT_API void setTickRate(float tps) {
		targetTPS = max(0.f, tps);
	}

	EXPORT_API int pollBreakpoint() {
		bool res = breakpoint;
		breakpoint = false;
		return res;
	}

	EXPORT_API void tick(int tick) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->tick(tick);
		simLock.unlock();
		targetTPS = tps;
	}

	EXPORT_API void toggleLatch(int x, int y) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->toggleLatch(glm::ivec2(x, y));
		simLock.unlock();
		targetTPS = tps;
	}

	EXPORT_API void toggleLatchIndex(int idx) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->toggleLatch(idx);
		simLock.unlock();
		targetTPS = tps;
	}

	EXPORT_API void addBreakpoint(int gid) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->addBreakpoint(gid);
		simLock.unlock();
		targetTPS = tps;
	}

	EXPORT_API void removeBreakpoint(int gid) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->removeBreakpoint(gid);
		simLock.unlock();
		targetTPS = tps;
	}

	/*
	* Functions to initialize openVCB
	*/

	EXPORT_API void newProject() {
		fprintf(stderr, "openVCB: https://github.com/kittybupu/openVCB\n");
		fprintf(stderr, "openVCB: Jerry#1058\n");
		proj = new Project;
	}

	EXPORT_API int initProject() {
		proj->preprocess(false);

		// Start sim thread paused
		run = true;
		simThread = new thread(simFunc);

		return proj->numGroups;
	}

	EXPORT_API void initVMem(char* assembly, int aSize, char* err, int errSize) {
		proj->assembly = string(assembly);
		proj->assembleVmem(err);
	}

	EXPORT_API void deleteProject() {
		run = false;
		if (simThread) {
			simThread->join();
			delete simThread;
			simThread = nullptr;
		}

		if (proj) {
			// These should be managed.
			proj->states = nullptr;
			proj->vmem = nullptr;
			proj->image = nullptr;

			delete proj;
			proj = nullptr;
		}
	}

	/*
	* Functions to replace openVCB buffers with managed ones
	*/

	EXPORT_API void setStateMemory(int* data, int size) {
		memcpy(data, proj->states, sizeof(int) * size);
		delete[] proj->states;
		proj->states = (InkState*)data;
	}

	EXPORT_API void addInstrumentBuffer(InkState* buff, int buffSize, int idx) {
		float tps = targetTPS;
		targetTPS = 0;
		simLock.lock();
		proj->instrumentBuffers.push_back({ buff, buffSize, idx });
		simLock.unlock();
		targetTPS = tps;
	}

	EXPORT_API void setVMemMemory(int* data, int size) {
		proj->vmem = data;
		proj->vmemSize = size;
	}

	EXPORT_API void setIndicesMemory(int* data, int size) {
		memcpy(data, proj->indexImage, sizeof(int) * size);
	}

	EXPORT_API void setImageMemory(int* data, int width, int height) {
		proj->width = width;
		proj->height = height;
		proj->image = (InkPixel*)data;
	}

	EXPORT_API void setDecoMemory(int* indices, int indLen, int* col, int colLen) {
		using namespace glm;
		std::vector<bool> visited(proj->width * proj->height, false);
		std::queue<ivec3> queue;

		for (size_t y = 0; y < proj->height; y++)
			for (size_t x = 0; x < proj->width; x++) {
				int idx = x + y * proj->width;
				indices[idx] = -1;

				if (col[idx] != 0xffffffff) continue;
				auto ink = openVCB::setOff(proj->image[idx].getInk());
				if (ink == Ink::LatchOff || ink == Ink::LedOff) {
					queue.push(ivec3(x, y, proj->indexImage[idx]));
					visited[idx] = true;
				}
			}

		const glm::ivec2 fourNeighbors[4]{
			ivec2(-1, 0),
			ivec2(0, 1),
			ivec2(1, 0),
			ivec2(0, -1)
		};

		while (queue.size()) {
			auto pos = queue.front();
			queue.pop();

			indices[pos.x + pos.y * proj->width] = pos.z;

			for (int k = 0; k < 4; k++) {
				ivec2 np = (ivec2)pos + fourNeighbors[k];
				if (np.x < 0 || np.x >= proj->width ||
					np.y < 0 || np.y >= proj->height) continue;

				int nidx = np.x + np.y * proj->width;
				if (visited[nidx]) continue;;
				visited[nidx] = true;

				if (col[nidx] != 0xffffffff) continue;
				queue.push(ivec3(np.x, np.y, pos.z));
			}
		}
	}

	/*
	* Functions to configure openVCB
	*/

	EXPORT_API void getGroupStats(int* numGroups, int* numConnections) {
		*numGroups = proj->numGroups;
		*numConnections = proj->writeMap.nnz;
	}

	EXPORT_API void setInterface(LatchInterface addr, LatchInterface data) {
		proj->vmAddr = addr;
		proj->vmData = data;
	}
}

int main() {
	printf("openVCB: https://github.com/kittybupu/openVCB\n");
}