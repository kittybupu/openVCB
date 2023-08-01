/*
 * Code for simulations
 */

// ReSharper disable CppTooWideScope
// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppUseStructuredBinding
#include "openVCB.h"

#if (defined __INTEL_COMPILER && false)                                   || \
    (defined __INTEL_LLVM_COMPILER && __INTEL_LLVM_COMPILER >= 20220000L) || \
    (defined __clang__ && __clang__ >= 12)                                || \
    (defined __GNUC__  && __GNUC__  >= 10)
# if !(defined __INTELLISENSE__ || defined __RESHARPER__)
#  define USE_GNU_INLINE_ASM 1
# endif
#endif


extern "C" {
ND uint32_t openVCB_evil_assembly_bit_manipulation_routine_getVMem(uint8_t *vmem, size_t addr);
   void     openVCB_evil_assembly_bit_manipulation_routine_setVMem(uint8_t *vmem, size_t addr, uint32_t data, int numBits);
}


namespace openVCB {


[[__gnu__::__hot__]]
SimulationResult
Project::tick(int32_t const numTicks, int64_t const maxEvents)
{
      SimulationResult res{};

      for (; res.numTicksProcessed < numTicks; ++res.numTicksProcessed)
      {
            if (res.numEventsProcessed > maxEvents)
                  return res;

            for (auto const &[buffer, bufferSize, idx] : instrumentBuffers)
                  buffer[tickNum % bufferSize] = states[idx];
            ++tickNum;
      
            // VMem implementation.
            if (vmem) {
#ifdef OVCB_BYTE_ORIENTED_VMEM
                  if (vmemIsBytes)
                        handleByteVMemTick();
                  else
                        handleWordVMemTick();
#else
                  handleWordVMemTick();
#endif
            }

            if (tickClock.tick())
                  for (auto const gid : tickClock.GIDs)
                        if (!states[gid].visited)
                              updateQ[0][qSize++] = gid;

            if (!realtimeClock.GIDs.empty() && realtimeClock.tick()) [[unlikely]]
                  for (auto const gid : realtimeClock.GIDs)
                        if (!states[gid].visited)
                              updateQ[0][qSize++] = gid;

            for (int traceUpdate = 0; traceUpdate < 2; ++traceUpdate) {
                  // We update twice per tick
                  // Remember stuff
                  uint const numEvents = qSize;
                  qSize                = 0;
                  res.numEventsProcessed += numEvents;

                  // Copy over the current number of active inputs
                  for (uint i = 0; i < numEvents; ++i) {
                        int   const gid = updateQ[0][i];
                        Logic const ink = states[gid].logic;

                        // Reset visited flag
                        states[gid].visited = false;

                        // Copy over last active inputs
                        lastActiveInputs[i] = states[gid].activeInputs;
                        if (SetOff(ink) == Logic::LatchOff)
                              states[gid].activeInputs = 0;
                  }

                  // Main update loop
                  for (uint i = 0; i < numEvents; ++i) {
                        int  const gid        = updateQ[0][i];
                        auto const curInk     = states[gid];
                        bool const lastActive = IsOn(curInk.logic);
                        bool const nextActive = resolve_state(res, curInk, lastActive, lastActiveInputs[i]);

                        // Short circuit if the state didnt change
                        if (lastActive == nextActive)
                              continue;

                        // Update the state
                        states[gid].logic = SetOn(curInk.logic, nextActive);

                        // Loop over neighbors
                        int     const delta = nextActive ? 1 : -1;
                        int32_t const end   = writeMap.ptr[gid + 1];

                        for (int n = writeMap.ptr[gid]; n < end; ++n) {
                              auto  const nxtId  = writeMap.rows[n];
                              Logic const nxtInk = SetOff(states[nxtId].logic);

                              // Ignore falling edge for latches
                              if (!nextActive && nxtInk == Logic::LatchOff)
                                    continue;

                              // Update actives
                              int const lastNxtInput     = states[nxtId].activeInputs;
                              states[nxtId].activeInputs = int16_t(lastNxtInput + delta);

                              // Inks have convenient "critical points"
                              // We can skip any updates that do not hover around 0 with a few exceptions.
                              if (lastNxtInput == 0 || lastNxtInput + delta == 0 || nxtInk == Logic::XorOff || nxtInk == Logic::XnorOff)
                                    tryEmit(nxtId);
                        }
                  }

                  if (res.breakpoint) // Stop early
                        return res;
                  // Swap buffer
                  std::swap(updateQ[0], updateQ[1]);
            }
      }

      return res;
}


[[__gnu__::__hot__]] OVCB_INLINE bool
Project::resolve_state(SimulationResult &res,
                       InkState const    curInk,
                       bool const        lastActive,
                       int const         lastInputs)
{
      // NOLINTNEXTLINE(clang-diagnostic-switch-enum)
      switch (SetOff(curInk.logic)) {
      case Logic::NonZeroOff: return lastInputs != 0;
      case Logic::ZeroOff:    return lastInputs == 0;
      case Logic::XorOff:     return lastInputs & 1;
      case Logic::XnorOff:    return !(lastInputs & 1);
      case Logic::LatchOff:   return lastActive ^ (lastInputs & 1);
      case Logic::RandomOff:  return lastInputs > 0 && (lastActive || GetRandomBit());
      case Logic::ClockOff:   return tickClock.is_zero()     ? !lastActive : lastActive;
      case Logic::TimerOff:   return realtimeClock.is_zero() ? !lastActive : lastActive;
      case Logic::BreakpointOff: {
            bool const ret = lastInputs > 0;
            if (ret)
                  res.breakpoint = true;
            return ret;
      }
      default:
            return false;
      }
}


[[__gnu__::__hot__]]
OVCB_CONSTEXPR bool
Project::tryEmit(int32_t const gid)
{
      // Check if this event is already in queue.
      if (states[gid].visited)
            return false;
      states[gid].visited = true;
      updateQ[1][qSize++] = gid;
      return true;
}


[[__gnu__::__hot__]]
OVCB_CONSTEXPR void
Project::handleWordVMemTick()
{
      // Get current address
      uint32_t addr = 0;
      for (int k = 0; k < vmAddr.numBits; ++k)
            addr |= static_cast<uint32_t>(IsOn(states[vmAddr.gids[k]].logic)) << k;

      if (addr != lastVMemAddr) {
            // Load address
            uint32_t data = 0;
            lastVMemAddr  = addr;

            data = vmem.i[addr];

            // Turn on those latches
            for (int k = 0; k < vmData.numBits; ++k) {
                  auto      &state = states[vmData.gids[k]];
                  bool const isOn  = IsOn(state.logic);

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
            for (int k = 0; k < vmData.numBits; ++k)
                  data |= static_cast<uint32_t>(IsOn(states[vmData.gids[k]].logic)) << k;

            vmem.i[addr] = data;
      }
}


#ifdef OVCB_BYTE_ORIENTED_VMEM
OVCB_CONSTEXPR void
Project::handleByteVMemTick()
{
      // Get current address
      uint32_t addr = 0;
      for (int k = 0; k < vmAddr.numBits; ++k)
            addr |= static_cast<uint32_t>(IsOn(states[vmAddr.gids[k]].logic)) << k;

      if (addr != lastVMemAddr) {
            // Load address
            uint32_t data = 0;
            lastVMemAddr  = addr;

# if 1
#  ifdef USE_GNU_INLINE_ASM
            // We can use Intel syntax. Hooray.
            __asm__ __volatile__ (
                  "mov	%[data], [%[vmem] + %q[addr]]"
                  : [data] "=r" (data)
                  : [vmem] "r" (vmem.b), [addr] "r" (addr)
                  :
            );
#  else
            data = ::openVCB_evil_assembly_bit_manipulation_routine_getVMem(vmem.b, addr);
#  endif
# else
            memcpy(&data, vmem.b + addr, sizeof data);
#endif

            // Turn on those latches
            for (int k = 0; k < vmData.numBits; ++k) {
                  auto      &state = states[vmData.gids[k]];
                  bool const isOn  = IsOn(state.logic);

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
            for (int k = 0; k < vmData.numBits; ++k)
                  data |= static_cast<uint32_t>(IsOn(states[vmData.gids[k]].logic)) << k;

# if 1
#  ifdef USE_GNU_INLINE_ASM
            __asm__ __volatile__ (
                  "shrx	eax, [%[vmem] + %q[addr]], %[numBits]" "\n\t"
                  "shlx	rax, rax, %q[numBits]"                 "\n\t"
                  "or	eax, %[data]"                          "\n\t"
                  "mov	[%[vmem] + %q[addr]], eax"             "\n\t"
                  :
                  : [vmem] "r" (vmem.b), [addr] "r" (addr),
                    [data] "r" (data),   [numBits] "r" (vmData.numBits)
                  : "cc", "rax"
            );
#  else
            openVCB_evil_assembly_bit_manipulation_routine_setVMem(vmem.b, addr, data, vmData.numBits);
#  endif
# else
            data = data >> vmData.numBits;
            data = static_cast<uint32_t>(static_cast<uint64_t>(data) << vmData.numBits);
            memcpy(vmem.b + addr, &data, sizeof data);
# endif
      }
}
#endif


} // namespace openVCB
