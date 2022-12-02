// Code for simulations

#include "openVCB.h"

namespace openVCB {
	using namespace std;
	using namespace glm;

	SimulationResult Project::tick(int numTicks, long long maxEvents) {
		SimulationResult res{};
		for (; res.numTicksProcessed < numTicks; res.numTicksProcessed++) {
			if (res.numEventsProcessed > maxEvents) return res;

			for (auto itr = breakpoints.begin(); itr != breakpoints.end(); itr++) {
				auto state = states[itr->first];
				if (state.logic != (unsigned char)itr->second) {
					itr->second = (Logic)state.logic;
					res.breakpoint = true;
				}
			}
			if (res.breakpoint) return res;

			for (auto& inst : instrumentBuffers)
				inst.buffer[tickNum % inst.bufferSize] = states[inst.idx];

			tickNum++;

			// VMem integration
			if (vmem) {
				// Get current address
				uint32_t addr = 0;
				for (int k = 0; k < vmAddr.numBits; k++)
					addr |= (uint32_t)getOn((Logic)states[vmAddr.gids[k]].logic) << k;

				if (addr != lastVMemAddr) {
					// Load address
					lastVMemAddr = addr;
					int data = vmem[addr];

					// Turn on those latches
					for (int k = 0; k < vmData.numBits; k++) {
						bool isOn = getOn((Logic)states[vmData.gids[k]].logic);
						if (((data >> k) & 1) != isOn) {
							states[vmData.gids[k]].activeInputs = 1;
							if (states[vmData.gids[k]].visited) continue;
							states[vmData.gids[k]].visited = 1;
							updateQ[0][qSize++] = vmData.gids[k];
						}
					}

					// Force ignore further address updates
					for (int k = 0; k < vmAddr.numBits; k++)
						states[vmAddr.gids[k]].activeInputs = 0;
				}
				else {
					// Write address
					int data = 0;
					for (int k = 0; k < vmData.numBits; k++)
						data |= (int)getOn((Logic)states[vmData.gids[k]].logic) << k;
					vmem[addr] = data;
				}
			}

			// Update the clock ink
			if (++clockCounter >= clockPeriod)
				clockCounter = 0;
			if (clockCounter < 2)
				for (auto gid : clockGIDs)
					if (!states[gid].visited)
						updateQ[0][qSize++] = gid;

			for (int traceUpdate = 0; traceUpdate < 2; traceUpdate++) { // We update twice per tick
				// Remember stuff
				const int numEvents = qSize;
				res.numEventsProcessed += numEvents;
				qSize = 0;

				// Copy over the current number of active inputs
				for (size_t i = 0; i < numEvents; i++) {
					const int gid = updateQ[0][i];
					const unsigned char ink = states[gid].logic;

					// Reset visited flag
					states[gid].visited = 0;

					// Copy over last active inputs
					lastActiveInputs[i] = states[gid].activeInputs;
					if (ink == (unsigned char)Logic::Latch ||
						ink == (unsigned char)Logic::LatchOff)
						states[gid].activeInputs = 0;
				}

#ifdef OVCB_MT
#pragma omp parallel for schedule(static, 4 * 1024) num_threads(2) if(numEvents > 8 * 1024)
#endif
				// Main update loop
				for (int i = 0; i < numEvents; i++) {
					int gid = updateQ[0][i];
					Logic curInk = (Logic)states[gid].logic;

					const bool lastActive = getOn(curInk);
					const int lastInputs = lastActiveInputs[i];
					bool nextActive = false;

					const Logic offInk = setOff(curInk);
					switch (offInk) {
					case Logic::NonZeroOff:
						nextActive = lastInputs != 0;
						break;

					case Logic::ZeroOff:
						nextActive = lastInputs == 0;
						break;

					case Logic::XorOff:
						nextActive = lastInputs % 2;
						break;

					case Logic::XnorOff:
						nextActive = !(lastInputs % 2);
						break;

					case Logic::LatchOff:
						nextActive = lastActive ^ (lastInputs % 2);
						break;

					case Logic::ClockOff:
						nextActive = clockCounter == 0;
						break;
					}

					// Short circuit if the state didnt change
					if (lastActive == nextActive)
						continue;

					// Update the state
					states[gid].logic = (unsigned char)setOn(curInk, nextActive);

					// Loop over neighbors
					const int delta = nextActive ? 1 : -1;
					int r = writeMap.ptr[gid];
					int end = writeMap.ptr[gid + 1];
					for (; r < end; r++) {
						const int nxtId = writeMap.rows[r];
						const Logic nxtInk = setOff((Logic)states[nxtId].logic);

						// Ignore falling edge for latches
						if (!nextActive && nxtInk == Logic::LatchOff)
							continue;

						// Update actives
#ifdef OVCB_MT
						const int lastNxtInput = states[nxtId].activeInputs.fetch_add(delta, std::memory_order_relaxed);
#else
						const int lastNxtInput = states[nxtId].activeInputs;
						states[nxtId].activeInputs = lastNxtInput + delta;
#endif

						// Inks have convenient "critical points"
						// We can skip any updates that do not hover around 0
						// with a few exceptions.
						if (lastNxtInput == 0 || lastNxtInput + delta == 0 ||
							nxtInk == Logic::XorOff || nxtInk == Logic::XnorOff)
							tryEmit(nxtId);
					}
			}

				// Swap buffer
				std::swap(updateQ[0], updateQ[1]);
		}
	}
		return res;
}

	void Project::addBreakpoint(int gid) {
		breakpoints[gid] = (Logic)states[gid].logic;
	}

	void Project::removeBreakpoint(int gid) {
		breakpoints.erase(gid);
	}
}