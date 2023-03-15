// Code for image proprocessing and graph generation

// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"

#ifdef NDEBUG
# undef NDEBUG
# include <cassert>
#endif

namespace openVCB {
/*======================================================================================*/


class Preprocessor
{
      struct Group {
            int   gid;
            Logic logic;
            Ink   ink;
      };

    public:
      explicit Preprocessor(Project &proj) noexcept;

      ~Preprocessor()                                   = default;
      Preprocessor(Preprocessor const &)                = delete;
      Preprocessor(Preprocessor &&) noexcept            = delete;
      Preprocessor &operator=(Preprocessor const &)     = delete;
      Preprocessor &operator=(Preprocessor &&) noexcept = delete;

      void do_it();

      /*--------------------------------------------------------------------------------*/

    private:
      void search(int x, int y);

      void explore_bus(glm::ivec2 pos, unsigned mask);
      void handle_read_ink(glm::ivec2 vec);
      void handle_write_ink(glm::ivec2 vec);

      bool handle_tunnel(uint nindex, bool ignoreMask, int32_t idx, int &newIdx, Ink &newInk, glm::ivec2 &newComp);

      ND bool    validate_vector(glm::ivec2 nvec) const;
      ND int32_t calc_index(glm::ivec2 vec) const;
      ND int32_t calc_index(int x, int y) const;

      void add_notfound_errmsg(glm::ivec2 neighbor, glm::ivec2 tunComp) const;
      void add_invalid_tunnel_entrance_errmsg(glm::ivec2 origComp, glm::ivec2 tmpComp) const;

      /*--------------------------------------------------------------------------------*/

      Project      &p;
      ssize_t const canvas_size;

      std::array<int, 4> wirelessIDs = {-1, -1, -1, -1};

      std::vector<glm::ivec2> stack;
      std::vector<glm::ivec2> bundleStack;
      std::vector<glm::ivec2> readInks;
      std::vector<glm::ivec2> writeInks;
      std::vector<uint32_t>   visited;
      std::vector<Group>      indexDict;

      std::unordered_multimap<int32_t, int32_t> bundleCons;
      std::unordered_set<int64_t>               bundleConsSet;

      // Hash sets to keep track of unique connections
      std::unordered_set<int64_t>              conSet;
      std::vector<std::pair<int32_t, int32_t>> conList;

      enum InkMask : uint32_t {
            MASK_READ     = 2U << 16,
            MASK_WRITE    = 2U << 17,
            MASK_TUNNEL_1 = 2U << 18,
            MASK_TUNNEL_2 = 2U << 19,
            MASK_TUNNEL_3 = 2U << 20,
            MASK_TUNNEL_4 = 2U << 21,
      };

      ND static unsigned get_tunnel_mask(uint const i) { return 2U << (18U + (i % 4U)); }
};


Preprocessor::Preprocessor(Project &proj) noexcept
    : p(proj),
      canvas_size(static_cast<ssize_t>(p.width) * static_cast<ssize_t>(p.height)),
      visited(canvas_size, 0)
{}


/*--------------------------------------------------------------------------------------*/


void
Preprocessor::do_it()
{
      delete p.error_messages;
      p.error_messages = new StringArray(32);

       // Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
      for (int i = 0; i < canvas_size; i++) {
            switch (p.image[i].ink) {  // NOLINT(clang-diagnostic-switch-enum)
            default:
                  break;
            case Ink::Trace:     case Ink::Read:
            case Ink::Write:     case Ink::Buffer:
            case Ink::Or:        case Ink::And:
            case Ink::Xor:       case Ink::Not:
            case Ink::Nor:       case Ink::Nand:
            case Ink::Xnor:      case Ink::Clock:
            case Ink::Led:       case Ink::Bus:
            case Ink::Tunnel:    case Ink::Timer:
            case Ink::Random:    case Ink::Breakpoint:
            case Ink::Wireless1: case Ink::Wireless2:
            case Ink::Wireless3: case Ink::Wireless4:
                  p.image[i].ink = SetOff(p.image[i].ink);
            }
      }
 
      // Split up the ordering by ink vs. comp.
      // Hopefully groups things better in memory.
      p.writeMap.n = 0;
      p.indexImage = new int32_t[canvas_size]{-1};
      std::fill_n(p.indexImage, canvas_size, -1);

      for (int y = 0; y < p.height; y++)
            for (int x = 0; x < p.width; x++)
                  search(x, y);

      p.numGroups = p.writeMap.n;

      // Sort groups by ink vs. component then by morton code.
      std::ranges::sort(indexDict, [](Group const &a, Group const &b) -> bool
      {
            if (a.ink == b.ink)
                  return a.gid < b.gid;
            return a.ink < b.ink;
      });
      glm::ivec2 foo;
      foo[0];

      // List of connections.
      // Build state vector.
      p.states_is_native = true;
      p.states    = new InkState[p.writeMap.n];
      p.stateInks = new Ink[p.writeMap.n];
      p.states[0] = {0, false, Logic::None};

      // Borrow writeMap for a reverse mapping
      p.writeMap.ptr = new int[p.writeMap.n + 1];

      for (int i = 0; i < p.writeMap.n; ++i) {
            auto const g   = indexDict[i];
            InkState  &s   = p.states[i];
            p.stateInks[i] = g.ink;
            s.logic        = g.logic;
            s.visited      = false;
            s.activeInputs = 0;

            p.writeMap.ptr[g.gid] = i;
      }

      // Remap indices
      for (ssize_t i = 0; i < canvas_size; ++i) {
            auto &idx = p.indexImage[i];
            if (idx >= 0)
                  idx = p.writeMap.ptr[idx];
      }

      for (auto const &vec : readInks)
            handle_read_ink(vec);
      conSet.clear();
      for (auto const &vec : writeInks)
            handle_write_ink(vec);

      // Stores rows per colume.
      std::vector accu(p.writeMap.n, 0);
      for (auto const &e : conList)
            accu[e.first] += 1;

      // Construct adjacentcy matrix
      p.writeMap.nnz  = static_cast<int32_t>(conList.size());
      p.writeMap.rows = new int[p.writeMap.nnz];
      p.writeMap.ptr[p.writeMap.n] = p.writeMap.nnz;

      // Prefix sum
      for (int i = 0, c = 0; i < p.writeMap.n; ++i) {
            p.writeMap.ptr[i] = c;
            c      += accu[i];
            accu[i] = 0;
      }

      // Populate
      for (auto const &[first, second] : conList) {
            // Set the active inputs of AND to be -numInputs
            if (util::eq_any(p.stateInks[second], Ink::AndOff, Ink::NandOff))
                  --(p.states[second].activeInputs);

            auto const idx = p.writeMap.ptr[first] + accu[first]++;
            p.writeMap.rows[idx] = second;
      }

      // Sort rows
      for (int i = 0; i < p.writeMap.n; ++i) {
            auto const start = p.writeMap.ptr[i];
            auto const end   = p.writeMap.ptr[i + 1];
            std::sort(&p.writeMap.rows[start], &p.writeMap.rows[end]);
      }

      p.updateQ[0]       = new int[p.writeMap.n];
      p.updateQ[1]       = new int[p.writeMap.n];
      p.lastActiveInputs = new int16_t[p.writeMap.n];
      p.qSize            = 0;

      // Insert starting events into the queue
      for (int i = 0; i < p.writeMap.n; ++i) {
            Ink const ink = p.stateInks[i];
            if (ink == Ink::ClockOff)
                  p.tickClock.GIDs.push_back(i);
            else if (ink == Ink::TimerOff)
                  p.realtimeClock.GIDs.push_back(i);
            else if (util::eq_any(ink, Ink::NandOff, Ink::NotOff, Ink::NorOff, Ink::XnorOff, Ink::Latch))
                  p.updateQ[0][p.qSize++] = i;
            if (ink == Ink::Latch)
                  p.states[i].activeInputs = 1;
      }
}


/*--------------------------------------------------------------------------------------*/


void
Preprocessor::search(int const x, int const y)
{
      int const top_idx = calc_index(x, y);

      if (visited[top_idx] & 1)
            return;

      // Check what ink this group is of
      InkPixel const pix = p.image[top_idx];
      Ink            ink = pix.ink;
      int            gid;

      switch (ink) {  // NOLINT(clang-diagnostic-switch-enum)
      case Ink::None:
      case Ink::Cross:
      case Ink::TunnelOff:
      case Ink::Annotation:
      case Ink::Filler:
            return;

      case Ink::ReadOff:
            readInks.emplace_back(x, y);
            ink = Ink::TraceOff;
            gid = p.writeMap.n++;
            break;

      case Ink::WriteOff:
            writeInks.emplace_back(x, y);
            ink = Ink::TraceOff;
            gid = p.writeMap.n++;
            break;;

      case Ink::Wireless1Off:
      case Ink::Wireless2Off:
      case Ink::Wireless3Off:
      case Ink::Wireless4Off: {
            auto const i = Ink::Wireless4Off - ink;
            if (wirelessIDs[i] < 0) {
                  gid = p.writeMap.n++;
                  wirelessIDs[i] = gid;
            } else {
                  gid = wirelessIDs[i];
            }
            break;
      }

      case Ink::BreakpointOff:
            gid = p.writeMap.n++;
            p.addBreakpoint(gid);
            break;

      default:
            gid = p.writeMap.n++;
            break;
      }

      // DFS
      stack.emplace_back(x, y);
      visited[top_idx] |= 1;

      while (!stack.empty())
      {
            auto const comp   = stack.back();
            auto const idx    = calc_index(comp);
            p.indexImage[idx] = gid;
            stack.pop_back();

            for (uint nindex = 0; nindex < 4; ++nindex) {
                  auto const &neighbor = fourNeighbors[nindex];
                  glm::ivec2 newComp  = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int  newIdx = calc_index(newComp);
                  uint newVis = visited[newIdx];
                  Ink  newInk = p.image[newIdx].ink;

                  // Handle wire bundles.
                  if (ink == Ink::TraceOff && newInk == Ink::BusOff) {
                        // What kind of ink are we again?
                        auto const [sink, smeta] = p.image[idx];
                        uint mask;

                        switch (sink) {  // NOLINT(clang-diagnostic-switch-enum)
                        case Ink::ReadOff:  mask = MASK_READ;   break;
                        case Ink::WriteOff: mask = MASK_WRITE;  break;
                        default:            mask = 2U << smeta; break;
                        }
                        if (newVis & mask)
                              continue;

                        // Hold my beer, we're jumping in.
                        explore_bus(newComp, mask);

                        if (newVis & 1) {
                              auto const otherIdx = p.indexImage[newIdx];
                              auto const shifted  = static_cast<int64_t>(gid) << 32;
                              if (bundleConsSet.insert(shifted | gid).second)
                                    bundleCons.emplace(gid, otherIdx);
                        }
                        continue;
                  }

                  if (newVis & 1) {
                        if (ink == Ink::BusOff && util::eq_any(newInk, Ink::TraceOff, Ink::ReadOff, Ink::WriteOff)) {
                              // Try to insert new connection
                              auto const otherIdx = p.indexImage[newIdx];
                              auto const shifted  = static_cast<int64_t>(gid) << 32;
                              if (bundleConsSet.insert(shifted | otherIdx).second)
                                    bundleCons.emplace(otherIdx, gid);
                        }
                        continue;
                  }

                  // Check ink type and handle crosses
                  if (newInk == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;

                        newIdx = calc_index(newComp);
                        if (visited[newIdx] & 1)
                              continue;
                        newInk = p.image[newIdx].ink;
                        if (newInk == Ink::TunnelOff)
                              continue;
                  }
                  else if (newInk == Ink::TunnelOff) {
                        if (!handle_tunnel(nindex, false, idx, newIdx, newInk, newComp))
                              continue;
                  }

                  // Push back if Allowable
                  if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
                        readInks.push_back(newComp);
                        visited[newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
                        writeInks.push_back(newComp);
                        visited[newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == ink) {
                        visited[newIdx] |= 1;
                        stack.push_back(newComp);
                  }
            }
      }

      // Add on the new group
      indexDict.push_back({gid, inkLogicType(ink), pix.ink});
}

void
Preprocessor::explore_bus(glm::ivec2 const pos, unsigned const mask)
{
      auto const idx = calc_index(pos);
      bundleStack.push_back(pos);
      visited[idx] |= mask;

      while (!bundleStack.empty())
      {
            glm::ivec2 const comp = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (uint nindex = 0; nindex < 4; ++nindex) {
                  glm::ivec2 const &neighbor = fourNeighbors[nindex];
                  glm::ivec2 newComp = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int  newIdx = calc_index(newComp);
                  uint newVis = visited[newIdx];
                  // Check if already visited

                  auto [newInk, newMeta] = p.image[newIdx];

                  if ((newVis & mask) == 2U)
                        continue;
                  // Handle different inks
                  if (newInk == Ink::ReadOff) {
                        if (newVis & 1)
                              continue;
                        readInks.push_back(newComp);
                        if ((mask >> 16) == 2) {
                              visited[newIdx] |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newInk == Ink::WriteOff) {
                        if (newVis & 1)
                              continue;
                        writeInks.push_back(newComp);
                        if ((mask >> 17) == 2) {
                              visited[newIdx] |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newInk == Ink::TraceOff) {
                        if (newVis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if ((mask >> newMeta) == 2) {
                              visited[newIdx] |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newInk == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;

                        newIdx = calc_index(newComp);
                        if (visited[newIdx] & mask)
                              continue;
                        newInk = p.image[newIdx].ink;
                  }
                  else if (newInk == Ink::TunnelOff) {
                        if (visited[newIdx] & mask)
                              continue;
                        visited[newIdx] |= mask;
                        if (!handle_tunnel(nindex, true, idx, newIdx, newInk, newComp))
                              continue;
                  }

                  if (newInk == Ink::BusOff && !(newVis & mask)) {
                        visited[newIdx] |= mask;
                        bundleStack.push_back(newComp);
                  }
            }
      }
}

void
Preprocessor::handle_read_ink(glm::ivec2 const vec)
{
      int const srcGID = p.indexImage[calc_index(vec)];

      for (auto neighbor : fourNeighbors) {
            glm::ivec2 const nvec = vec + neighbor;
            if (!validate_vector(nvec))
                  continue;

            auto const newIdx = calc_index(nvec);
            auto const ink  = p.image[newIdx].ink;
            // Ignore any bundles or clocks
            if (ink == Ink::BusOff || ink == Ink::ClockOff || ink == Ink::TimerOff)
                  continue;

            auto const dstGID  = p.indexImage[newIdx];
            auto const shifted = static_cast<int64_t>(srcGID) << 32;

            if (srcGID != dstGID && dstGID != -1 && conSet.insert(shifted | dstGID).second)
                  conList.emplace_back(srcGID, dstGID);
      }
}

void
Preprocessor::handle_write_ink(glm::ivec2 const vec)
{
      int const dstGID = p.indexImage[calc_index(vec)];
      // Check if we got any wire bundles as baggage
      auto const range = bundleCons.equal_range(indexDict[dstGID].gid);

      for (auto neighbor : fourNeighbors) {
            glm::ivec2 const nvec = vec + neighbor;
            if (!validate_vector(nvec))
                  continue;

            auto const newIdx = calc_index(nvec);
            // Ignore any bundles
            if (p.image[newIdx].ink == Ink::BusOff)
                  continue;

            auto const srcGID  = p.indexImage[newIdx];
            auto const shifted = static_cast<int64_t>(srcGID) << 32;

            if (srcGID != dstGID && srcGID != -1 && conSet.insert(shifted | dstGID).second)
            {
                  conList.emplace_back(srcGID, dstGID);

                  // Tack on those for the bundle too
                  for (auto itr = range.first; itr != range.second; ++itr) {
                        int  bundleID = p.writeMap.ptr[itr->second];
                        if (conSet.insert(shifted | bundleID).second)
                              conList.emplace_back(srcGID, bundleID);
                  }
            }
      }
}


bool
Preprocessor::handle_tunnel(uint const    nindex,
                            bool const    ignoreMask,
                            int32_t const idx,
                            int          &newIdx,
                            Ink          &newInk,
                            glm::ivec2   &newComp)
{
      auto const &neighbor = fourNeighbors[nindex];
      auto const origComp  = newComp;

      if (!ignoreMask) {
            auto const mask = get_tunnel_mask(nindex);
            if (visited[idx] & mask)
                  return false;
            visited[idx] |= mask;
      }

      for (auto tunComp = newComp + neighbor; ; tunComp += neighbor)
      {
            if (!validate_vector(tunComp)) {
                  // BUG ERROR ERROR ERROR ERROR
                  add_notfound_errmsg(neighbor, newComp);
                  return false;
            }
            auto tunIdx = calc_index(tunComp);
            if (p.image[tunIdx].ink != Ink::TunnelOff)
                  continue;
            auto &tunVis = visited[tunIdx];

      retry:
            tunComp += neighbor;
            if (!validate_vector(tunComp)) {
                  // BUG ERROR ERROR ERROR ERROR
                  add_notfound_errmsg(neighbor, newComp);
                  return false;
            }

            tunIdx      = calc_index(tunComp);
            auto tunPix = p.image[tunIdx];

            if (tunPix.ink == Ink::TunnelOff)
                  goto retry;

            if (tunPix == p.image[idx]) {
                  if (!ignoreMask) {
                        auto const mask = get_tunnel_mask(nindex + 2);
                        if (tunVis & mask)
                              return false;
                        tunVis |= mask;
                        visited[tunIdx] |= mask;
                  }
                  newComp = tunComp;
                  newInk  = tunPix.ink;
                  newIdx  = tunIdx;
                  return true;
            }

            auto tmpComp = tunComp - neighbor;
            tmpComp -= neighbor;
            tunIdx = calc_index(tunComp);
            tunPix = p.image[tunIdx];
            if (tunPix == p.image[idx]) {
                  // BUG ERROR ERROR ERROR ERROR
                  add_invalid_tunnel_entrance_errmsg(origComp, tmpComp);
                  return false;
            }
      }
}


bool Preprocessor::validate_vector(glm::ivec2 const nvec) const
{
      return nvec.x >= 0 && nvec.x < p.width  &&
             nvec.y >= 0 && nvec.y < p.height;
}

int32_t Preprocessor::calc_index(glm::ivec2 const vec) const
{
      return vec.x + vec.y * p.width;
}

int32_t Preprocessor::calc_index(int const x, int const y) const
{
      return x + y * p.width;
}


void
Preprocessor::add_notfound_errmsg(glm::ivec2 const neighbor,
                                  glm::ivec2 const tunComp) const
{
      char const *dir = neighbor.x == 1    ? "east"
                        : neighbor.x == -1 ? "west"
                        : neighbor.y == 1  ? "south"
                                           : "north";
      char      *buf  = p.error_messages->push_blank(256);
      auto const size = snprintf(
          buf, 256,
          R"(Error @ (%d, %d): No exit tunnel found in a search to the %s -- (%d, %d).)",
          tunComp.x, tunComp.y, dir, neighbor.x, neighbor.y);
      util::logs(buf, size);
}


void
Preprocessor::add_invalid_tunnel_entrance_errmsg(glm::ivec2 const origComp,
                                                 glm::ivec2 const tmpComp) const
{
      char      *buf = p.error_messages->push_blank(256);
      auto const size = snprintf(
            buf, 256,
            "Error (%d, %d) -> (%d, %d): While searching for an exit, another "
            "tunnel entrance for the same ink was encountered.",
            origComp.x, origComp.y, tmpComp.x, tmpComp.y);
      util::logs(buf, size);
}


/*--------------------------------------------------------------------------------------*/


[[__gnu__::__hot__]] void
Project::preprocess()
{
      Preprocessor pp(*this);
      pp.do_it();
      util::logs("--------------------------------------------------------------------------------", 80);
}


/*======================================================================================*/
} // namespace openVCB
