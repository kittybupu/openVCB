// Code for simulations

// ReSharper disable CppTooWideScope
#include "openVCB.hh"


namespace openVCB {


SimulationResult
Project::tick(int const numTicks, int64_t const maxEvents)
{
      SimulationResult res{};

      for (; res.numTicksProcessed < numTicks; ++res.numTicksProcessed) {
            if (res.numEventsProcessed > maxEvents)
                  return res;

            // for (auto &&itr = breakpoints.begin(); itr != breakpoints.end(); ++itr) {
            for (auto &itr : breakpoints) {
                  auto const &state = states[itr.first];
                  if (state.logic != itr.second) {
                        itr.second     = state.logic;
                        res.breakpoint = true;
                  }
            }
            if (res.breakpoint)
                  return res;

            for (auto const &inst : instrumentBuffers)
                  inst.buffer[tickNum % inst.bufferSize] = states[inst.idx];

            ++tickNum;

            // VMem integration
            if (vmem) {
                  // Get current address
                  int addr = 0;
                  for (int k = 0; k < vmAddr.numBits; ++k)
                        addr |= (int)getOn(states[vmAddr.gids[k]].logic) << k;

                  if (addr != lastVMemAddr) {
                        // Load address
                        lastVMemAddr = addr;
                        int data     = vmem[addr];

                        // Turn on those latches
                        for (int k = 0; k < vmData.numBits; k++) {
                              bool isOn = getOn(states[vmData.gids[k]].logic);
                              if (((data >> k) & 1) != isOn) {
                                    states[vmData.gids[k]].activeInputs = 1;
                                    if (states[vmData.gids[k]].visited)
                                          continue;
                                    states[vmData.gids[k]].visited = 1;
                                    updateQ[0][qSize++]            = vmData.gids[k];
                              }
                        }

                        // Force ignore further address updates
                        for (int k = 0; k < vmAddr.numBits; ++k)
                              states[vmAddr.gids[k]].activeInputs = 0;
                  } else {
                        // Write address
                        int data = 0;
                        for (int k = 0; k < vmData.numBits; ++k)
                              data |= (int)getOn(states[vmData.gids[k]].logic) << k;
                        vmem[addr] = data;
                  }
            }

            for (int traceUpdate = 0; traceUpdate < 2; ++traceUpdate)
            {
                  // We update twice per tick
                  // Remember stuff
                  int const numEvents = qSize;
                  qSize = 0;
                  res.numEventsProcessed += numEvents;

                  // Copy over the current number of active inputs
                  for (int i = 0; i < numEvents; ++i) {
                        int const   gid = updateQ[0][i];
                        Logic const ink = states[gid].logic;

                        // Reset visited flag
                        states[gid].visited = 0;

                        // Copy over last active inputs
                        lastActiveInputs[i] = states[gid].activeInputs;
                        if (ink == Logic::Latch || ink == Logic::LatchOff)
                              states[gid].activeInputs = 0;
                  }

#ifdef OVCB_MT
#pragma omp parallel for schedule(static, 4 * 1024) \
    num_threads(2) if (numEvents > 8 * 1024)
#endif
                  // Main update loop
                  for (int i = 0; i < numEvents; ++i) {
                        int   gid    = updateQ[0][i];
                        Logic curInk = states[gid].logic;

                        bool const lastActive = getOn(curInk);
                        int const  lastInputs = lastActiveInputs[i];
                        bool       nextActive = false;

                        Logic const offInk = setOff(curInk);
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
                              nextActive = (int)lastActive ^ (lastInputs % 2);
                              break;

                        case Logic::ClockOff:
                              nextActive = (!traceUpdate) ^ lastActive;
                              // As a special case, we are going to self emit
                              tryEmit(gid);
                              break;
                        }

                        // Short circuit if the state didnt change
                        if (lastActive == nextActive)
                              continue;

                        // Update the state
                        states[gid].logic = setOn(curInk, nextActive);

                        // Loop over neighbors
                        int const delta = nextActive ? 1 : -1;
                        int32_t   r     = writeMap.ptr[gid];
                        int32_t   end   = writeMap.ptr[gid + 1];

                        for (; r < end; r++) {
                              int const   nxtId  = writeMap.rows[r];
                              Logic const nxtInk = setOff(states[nxtId].logic);

                              // Ignore falling edge for latches
                              if (!nextActive && nxtInk == Logic::LatchOff)
                                    continue;

                                    // Update actives
#ifdef OVCB_MT
                              int const lastNxtInput = states[nxtId].activeInputs.fetch_add(delta, std::memory_order::relaxed);
#if 0
#pragma omp critical
                              {
                                    int const lastNxtInput = states[nxtId].activeInputs;
                                    states[nxtId].activeInputs = lastNxtInput + delta;

                                    if (lastNxtInput == 0 || lastNxtInput + delta == 0 || nxtInk == Logic::XorOff || nxtInk == Logic::XnorOff)
                                          tryEmit(nxtId);
                              }
#endif
#else
                              int const lastNxtInput     = states[nxtId].activeInputs;
                              states[nxtId].activeInputs = lastNxtInput + delta;
#endif

                              // Inks have convenient "critical points"
                              // We can skip any updates that do not hover around 0
                              // with a few exceptions.
                              if (lastNxtInput == 0 || lastNxtInput + delta == 0 || nxtInk == Logic::XorOff || nxtInk == Logic::XnorOff)
                                    tryEmit(nxtId);
                        }
                  }

                  // Swap buffer
                  std::swap(updateQ[0], updateQ[1]);
            }
      }
      return res;
}

void
Project::addBreakpoint(int gid)
{
      breakpoints[gid] = states[gid].logic;
}

void
Project::removeBreakpoint(int gid)
{
      breakpoints.erase(gid);
}


} // namespace openVCB
