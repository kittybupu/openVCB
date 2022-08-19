
#include "openVCB.h"



namespace openVCB {
	using namespace std;
	using namespace glm;

	void Project::tick(int numTicks) {
		for (size_t i = 0; i < numTicks; i++) {
			// VMem integration
			if (vmem) {
				// Get current address
				int addr = 0;
				for (int k = 0; k < vmAddr.numBits; k++)
					addr |= (int)getOn((Ink)states[vmAddr.gids[k]].ink) << k;

				if (addr != lastVMemAddr) {
					// Load address
					lastVMemAddr = addr;
					int data = vmem[addr];

					// Turn on those latches
					for (int k = 0; k < vmData.numBits; k++) {
						bool isOn = getOn((Ink)states[vmData.gids[k]].ink);
						if (((data >> k) & 1) != isOn) {
							states[vmData.gids[k]].activeInputs = 1;
							if (states[vmData.gids[k]].visited) continue;
							states[vmData.gids[k]].visited = 1;
							updateQ[0][qSize++] = vmData.gids[k];
						}
					}
				}
				else {
					// Write address
					int data = 0;
					for (int k = 0; k < vmData.numBits; k++)
						data |= (int)getOn((Ink)states[vmData.gids[k]].ink) << k;
					vmem[addr] = data;
				}
			}

			for (int traceUpdate = 0; traceUpdate < 2; traceUpdate++) { // We update twice per tick
				// Remember stuff
				const int numEvents = qSize;
				qSize = 0;

				// Copy over the current number of active inputs
				for (size_t i = 0; i < numEvents; i++) {
					const int gid = updateQ[0][i];
					const unsigned char ink = states[gid].ink;

					// Reset visited flag
					states[gid].visited = 0;

					// Copy over last active inputs
					lastActiveInputs[i] = states[gid].activeInputs;
					if (ink == (unsigned char)Ink::Latch ||
						ink == (unsigned char)Ink::LatchOff)
						states[gid].activeInputs = 0;
				}

#ifdef OVCB_MT
#pragma omp parallel for schedule(static, 4 * 1024) num_threads(2) if(numEvents > 8 * 1024)
#endif
				// Main update loop
				for (int i = 0; i < numEvents; i++) {
					int gid = updateQ[0][i];
					Ink curInk = (Ink)states[gid].ink;

					const bool lastActive = getOn(curInk);
					const int lastInputs = lastActiveInputs[i];
					bool nextActive = false;

					const Ink offInk = setOff(curInk);
					switch (offInk) {
					case Ink::TraceOff:
					case Ink::BufferOff:
					case Ink::OrOff:
					case Ink::NandOff:
						nextActive = lastInputs != 0;
						break;

					case Ink::NotOff:
					case Ink::NorOff:
					case Ink::AndOff:
						nextActive = lastInputs == 0;
						break;

					case Ink::XorOff:
						nextActive = lastInputs % 2;
						break;

					case Ink::XnorOff:
						nextActive = !(lastInputs % 2);
						break;

					case Ink::LatchOff:
						nextActive = lastActive ^ (lastInputs % 2);
						break;
					case Ink::ClockOff:
						nextActive = (!traceUpdate) ^ lastActive;
						// As a special case, we are going to self emit
						tryEmit(gid);
						break;
					case Ink::LedOff:
						nextActive = lastInputs > 0;
						break;
					}

					// Short circuit if the state didnt change
					if (lastActive == nextActive)
						continue;

					// Update the state
					states[gid].ink = (unsigned char)setOn(curInk, nextActive);

					// Loop over neighbors
					const int delta = nextActive ? 1 : -1;
					int r = writeMap.ptr[gid];
					int end = writeMap.ptr[gid + 1];
					for (; r < end; r++) {
						const int nxtId = writeMap.rows[r];
						const Ink nxtInk = setOff((Ink)states[nxtId].ink);

						// Ignore falling edge for latches
						if (!nextActive && nxtInk == Ink::LatchOff)
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
							nxtInk == Ink::XorOff || nxtInk == Ink::XnorOff)
							tryEmit(nxtId);
					}
				}

				// Swap buffer
				std::swap(updateQ[0], updateQ[1]);
			}
		}
	}
}