// Code for mostly misc stuff.

#include "openVCB.hh"


namespace openVCB {


void
Project::toggleLatch(glm::ivec2 pos)
{
      if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
            return;
      int const gid = indexImage[pos.x + pos.y * width];
      toggleLatch(gid);
}

void
Project::toggleLatch(int gid)
{
      if (setOff(stateInks[gid]) != Ink::LatchOff)
            return;
      states[gid].activeInputs = 1;
      if (states[gid].visited)
            return;
      states[gid].visited = 1;
      updateQ[0][qSize++] = gid;
}

Project::~Project()
{
      delete[] originalImage;
      delete[] vmem;
      delete[] image;
      delete[] indexImage;
      delete[] decoration[0];
      delete[] decoration[1];
      delete[] decoration[2];
      delete[] writeMap.ptr;
      delete[] writeMap.rows;
      delete[] states;
      delete[] stateInks;
      delete[] updateQ[0];
      delete[] updateQ[1];
      delete[] lastActiveInputs;
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

      auto state = states[idx].logic;
      type       = (type & 0x7f) | static_cast<unsigned>(state & 0x80);
      return {type, idx};
}


} // namespace openVCB