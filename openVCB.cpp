// Code for mostly misc stuff.

#include "openVCB.h"

extern "C" void openVCB_FreeErrorArray() noexcept;


namespace openVCB {
/****************************************************************************************/


/* Paranoia. */
static_assert(sizeof(InkState) == 4 && offsetof(InkState, logic) == 3);
static_assert(sizeof(InkPixel) == 4 && offsetof(InkPixel, meta)  == 2);
static_assert(sizeof(SparseMat) == 24);
static_assert(sizeof(InstrumentBuffer) == 16);
static_assert(sizeof(SimulationResult) == 16);
static_assert(sizeof(LatchInterface) == 284);
static_assert(sizeof(VMemWrapper) == sizeof(void *));
static_assert(sizeof(int) == 4);


void
Project::toggleLatch(glm::ivec2 const pos)
{
      if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
            return;
      int const gid = indexImage[pos.x + pos.y * width];
      toggleLatch(gid);
}

void
Project::toggleLatch(int const gid)
{
      if (SetOff(stateInks[gid]) != Ink::LatchOff)
            return;
      states[gid].activeInputs = 1;
      if (states[gid].visited)
            return;
      states[gid].visited = true;
      updateQ[0][qSize++] = gid;
}

Project::Project(int64_t const seed, bool const vmemIsBytes)
    : vmemIsBytes(vmemIsBytes)
{
      if (seed > 0 && seed <= UINT32_MAX)
            random = RandomBitProvider{static_cast<uint32_t>(seed)};
}

Project::~Project()
{
      if (states_is_native)
            delete[] states;
      delete[] originalImage;
      delete[] indexImage;
      delete[] decoration[0];
      delete[] decoration[1];
      delete[] decoration[2];
      delete[] writeMap.ptr;
      delete[] writeMap.rows;
      delete[] stateInks;
      delete[] updateQ[0];
      delete[] updateQ[1];
      delete[] lastActiveInputs;

      ::openVCB_FreeErrorArray();
}

std::pair<Ink, int>
Project::sample(glm::ivec2 const pos) const
{
      if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
            return {Ink::None, -1};

      int idx  = pos.x + pos.y * width;
      Ink type = image[idx].ink;
      idx      = indexImage[idx];
      if (type == Ink::Cross)
            return {Ink::Cross, idx};

      if (idx == -1)
            return {Ink::None, -1};

      type = (type & 0x7F) | (states[idx].logic & 0x80);
      return {type, idx};
}

void
Project::addBreakpoint(int const gid)
{
      //breakpoints[gid] = states[gid].logic;
}

void
Project::removeBreakpoint(int const gid)
{
      //breakpoints.erase(gid);
}


/****************************************************************************************/
} // namespace openVCB