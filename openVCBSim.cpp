/*
 * Code for simulations
 */

// ReSharper disable CppTooWideScope
// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"

extern "C" void openVCB_evil_assembly_bit_manipulation_routine_setVMem(uint8_t *vmem, size_t addr, uint32_t data, int numBits);

namespace openVCB {


[[__gnu__::__hot__]]
SimulationResult
Project::tick(int const numTicks, int64_t const maxEvents)
{
      SimulationResult res{};

      for (; res.numTicksProcessed < numTicks; ++res.numTicksProcessed)
      {
            if (res.numEventsProcessed > maxEvents)
                  return res;

            for (auto &[index, bp_logic] : breakpoints) {
                  auto const &state = states[index];
                  if (state.logic != bp_logic) {
                        bp_logic       = state.logic;
                        res.breakpoint = true;
                  }
            }
            if (res.breakpoint)
                  return res;

            for (auto const &[buffer, bufferSize, idx] : instrumentBuffers) {
#ifdef OVCB_MT
                  memmove(&buffer[tickNum % bufferSize], &states[idx], sizeof(InkState));
#else
                  buffer[tickNum % bufferSize] = states[idx];
#endif
            }
            ++tickNum;

            // VMem implementation.
            if (vmem) {
                  // Get current address
                  uint32_t addr = 0;
                  for (int k = 0; k < vmAddr.numBits; ++k)
                        addr |= static_cast<unsigned>(getOn(states[vmAddr.gids[k]].logic)) << k;

                  if (addr != lastVMemAddr) {
                        // Load address
                        lastVMemAddr  = addr;
#ifdef OVCB_BYTE_ORIENTED_VMEM
                        uint32_t data = (vmem.b[addr]) | (vmem.b[addr+1]<<8) | (vmem.b[addr+2]<<16) | (vmem.b[addr+3]<<24);
#else
                        uint32_t data = vmem.i[addr];
#endif
                        // Turn on those latches
                        for (int k = 0; k < vmData.numBits; ++k) {
                              auto      &state = states[vmData.gids[k]];
                              bool const isOn  = getOn(state.logic);

                              if (((data >> k) & 1) != isOn) {
                                    state.activeInputs = 1;
                                    if (state.visited)
                                          continue;
                                    state.visited       = true;
                                    updateQ[0][qSize++] = vmData.gids[k];
                              }
                        }

                        // Forcibly ignore further address updates
                        for (int k = 0; k < vmAddr.numBits; ++k)
                              states[vmAddr.gids[k]].activeInputs = 0;
                  } else {
                        // Write address
                        uint32_t data = 0;
                        for (int k = 0; k < vmData.numBits; ++k) {
                              auto const &state = states[vmData.gids[k]];
                              data |= static_cast<unsigned>(getOn(state.logic)) << k;
                        }

#ifdef OVCB_BYTE_ORIENTED_VMEM
                        uint32_t *const tmp_ptr = vmem.word_at_byte(addr);
                        // If numBits is 32 then this shift does nothing, meaning data
                        // will be ORed with all of *tmp_ptr. So avoid that.
                        *tmp_ptr = vmData.numBits == 32
                                       ? data
                                       : data | ((UINT32_MAX << vmData.numBits) & *tmp_ptr);

                        //openVCB_evil_assembly_bit_manipulation_routine_setVMem(vmem.b, addr, data, vmData.numBits);
#else
                        vmem.i[addr] = data;
#endif
                  }
            }

            // Update the clock ink
            if (++clockCounter >= clockPeriod)
                  clockCounter = 0;
            if (clockCounter < 2)
                  for (auto const gid : clockGIDs)
                        if (!states[gid].visited)
                              updateQ[0][qSize++] = gid;

            for (int traceUpdate = 0; traceUpdate < 2; ++traceUpdate) {
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
                        states[gid].visited = false;

                        // Copy over last active inputs
                        lastActiveInputs[i] = states[gid].activeInputs;
                        if (bool(ink & Logic::LatchOff))
                              states[gid].activeInputs = 0;
                  }

#ifdef OVCB_MT
# pragma omp parallel for schedule(static, 4 * 1024) num_threads(2) if (numEvents > 8 * 1024)
#endif
                  // Main update loop
                  for (int i = 0; i < numEvents; ++i) {
                        int const   gid        = updateQ[0][i];
                        Logic const curInk     = states[gid].logic;
                        bool const  lastActive = getOn(curInk);
                        int const   lastInputs = lastActiveInputs[i];
                        bool        nextActive = false;

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
                              nextActive = static_cast<int>(lastActive) ^ (lastInputs % 2);
                              break;
                        case Logic::ClockOff:
                              nextActive = clockCounter == 0;
                              break;
                        default:;
                        }

                        // Short circuit if the state didnt change
                        if (lastActive == nextActive)
                              continue;

                        // Update the state
                        states[gid].logic = setOn(curInk, nextActive);

                        // Loop over neighbors
                        int const     delta = nextActive ? 1 : -1;
                        int32_t const end   = writeMap.ptr[gid + 1];

                        for (int32_t r = writeMap.ptr[gid]; r < end; ++r) {
                              auto const  nxtId  = static_cast<unsigned>(writeMap.rows[r]);
                              Logic const nxtInk = setOff(states[nxtId].logic);

                              // Ignore falling edge for latches
                              if (!nextActive && nxtInk == Logic::LatchOff)
                                    continue;

                              // Update actives
#ifdef OVCB_MT
                              int const lastNxtInput = states[nxtId].activeInputs.fetch_add(delta, std::memory_order::relaxed);
#else
                              int const lastNxtInput     = states[nxtId].activeInputs;
                              states[nxtId].activeInputs = static_cast<int16_t>(lastNxtInput + delta);
#endif
                              static constexpr Logic XOR_XNOR = Logic::XorOff | Logic::XnorOff;

                              // Inks have convenient "critical points"
                              // We can skip any updates that do not hover around 0 with a few exceptions.
                              if (lastNxtInput == 0 || lastNxtInput + delta == 0 || bool(nxtInk & XOR_XNOR))
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
Project::addBreakpoint(int const gid)
{
      breakpoints[gid] = states[gid].logic;
}

void
Project::removeBreakpoint(int const gid)
{
      breakpoints.erase(gid);
}


} // namespace openVCB
