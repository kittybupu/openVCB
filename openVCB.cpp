// Code for mostly misc stuff.

#include "openVCB.hh"


namespace openVCB {


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
      if (setOff(stateInks[gid]) != Ink::LatchOff)
            return;
      states[gid].activeInputs = 1;
      if (states[gid].visited)
            return;
      states[gid].visited = true;
      updateQ[0][qSize++] = gid;
}

Project::~Project()
{
      delete[] originalImage;
      delete[] vmem.def();
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

      type = (type & 0x7f) | static_cast<unsigned>(states[idx].logic & 0x80);
      return {type, idx};
}


} // namespace openVCB