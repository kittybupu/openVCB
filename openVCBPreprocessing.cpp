// Code for image proprocessing and graph generation

// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"
#include <tbb/spin_mutex.h>

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
      void tryInsertConnection(int gid, int newIdx);

      /*--------------------------------------------------------------------------------*/

    private:
      void search(int x, int y);

      void explore_bus(glm::ivec2 pos, InkPixel const &pix, uint64_t mask);
      void handle_read_ink(glm::ivec2 vec);
      void handle_write_ink(glm::ivec2 vec);

      bool handle_tunnel(unsigned nindex, bool ignoreMask, int idx, int &newIdx,
                         Ink &newInk, glm::ivec2 &newComp);

      ND bool    validate_vector(glm::ivec2 nvec) const;
      ND int32_t calc_index(glm::ivec2 vec) const;
      ND int32_t calc_index(int x, int y) const;

      ND glm::ivec2 get_pos_from_index(int idx) const;

      void push_tunnel_exit_not_found_error(glm::ivec2 neighbor, glm::ivec2 tunComp) const;
      void push_invalid_tunnel_entrance_error(glm::ivec2 origComp, glm::ivec2 tmpComp) const;

      /*--------------------------------------------------------------------------------*/

      Project      &p;
      ssize_t const canvas_size;

      std::array<int, 4> wirelessIDs = {-1, -1, -1, -1};

      std::vector<glm::ivec2> stack;
      std::vector<glm::ivec2> bundleStack;
      std::vector<glm::ivec2> readInks;
      std::vector<glm::ivec2> writeInks;
      std::vector<Group>      indexDict;

      std::unordered_multimap<int, int> bundleCons;
      std::unordered_set<int64_t>       bundleConsSet;

      // Hash sets to keep track of unique connections
      std::unordered_set<int64_t>      conSet;
      std::vector<std::pair<int, int>> conList;

      std::vector<int> meshList;

      std::array<std::span<uint64_t>, 2> visited;
      std::unique_ptr<uint64_t[]>        visited_ptrs_[2];

      enum InkMask : uint64_t {
            MASK_READ  = 2U << 16,
            MASK_WRITE = 2U << 17,

            SHIFT_OFFSET_TUNNEL = 18,
            SHIFT_OFFSET_BUS    = 40,
      };

      ND static constexpr uint64_t get_tunnel_mask(unsigned const i)
      {
            return UINT64_C(2) << (SHIFT_OFFSET_TUNNEL + (i % 4U));
      }

      ND static constexpr uint64_t get_mask(InkPixel const &pix)
      {
            switch (pix.ink) { // NOLINT(clang-diagnostic-switch-enum)
            case Ink::ReadOff:       return MASK_READ;
            case Ink::WriteOff:      return MASK_WRITE;
            case Ink::TraceOff:      return UINT64_C(2) << pix.meta;
            case Ink::BufferOff:     return UINT64_C(2) << 22;
            case Ink::OrOff:         return UINT64_C(2) << 23;
            case Ink::NandOff:       return UINT64_C(2) << 24;
            case Ink::NotOff:        return UINT64_C(2) << 25;
            case Ink::NorOff:        return UINT64_C(2) << 26;
            case Ink::AndOff:        return UINT64_C(2) << 27;
            case Ink::XorOff:        return UINT64_C(2) << 28;
            case Ink::XnorOff:       return UINT64_C(2) << 29;
            case Ink::ClockOff:      return UINT64_C(2) << 30;
            case Ink::LatchOff:      return UINT64_C(2) << 31;
            case Ink::LedOff:        return UINT64_C(2) << 32;
            case Ink::TimerOff:      return UINT64_C(2) << 33;
            case Ink::RandomOff:     return UINT64_C(2) << 34;
            case Ink::BreakpointOff: return UINT64_C(2) << 35;
            case Ink::Wireless1Off:  return UINT64_C(2) << 36;
            case Ink::Wireless2Off:  return UINT64_C(2) << 37;
            case Ink::Wireless3Off:  return UINT64_C(2) << 38;
            case Ink::Wireless4Off:  return UINT64_C(2) << 39;
            case Ink::BusOff:        return UINT64_C(2) << SHIFT_OFFSET_BUS;
            default:                 return 0;
            }

      }
};


Preprocessor::Preprocessor(Project &proj) noexcept
    : p(proj),
      canvas_size(static_cast<ssize_t>(p.width) * static_cast<ssize_t>(p.height))
{
      visited_ptrs_[0] = std::make_unique<uint64_t[]>(canvas_size);
      visited_ptrs_[1] = std::make_unique<uint64_t[]>(canvas_size);
      visited[0]       = std::span(visited_ptrs_[0].get(), canvas_size);
      visited[1]       = std::span(visited_ptrs_[1].get(), canvas_size);
}


/*--------------------------------------------------------------------------------------*/


void
Preprocessor::do_it()
{
      delete p.error_messages;
      p.error_messages = new StringArray(32);

       // Turn off any inks that start as off
//#pragma omp parallel for schedule(static, 8192)
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
            case Ink::Timer:     case Ink::Random:
            case Ink::Tunnel:    case Ink::Breakpoint:
            case Ink::Wireless1: case Ink::Wireless2:
            case Ink::Wireless3: case Ink::Wireless4:
            case Ink::InvalidMesh:
                  p.image[i].ink = SetOff(p.image[i].ink);
            }
      }
 
      // Split up the ordering by ink vs. comp.
      // Hopefully groups things better in memory.
      p.writeMap.n = 0;
      p.indexImage = new int32_t[canvas_size];
      std::fill_n(p.indexImage, canvas_size, -1);

      for (int i = 0; i < canvas_size; ++i)
            if (p.image[i].ink == Ink::Mesh)
                  meshList.push_back(i);
      if (!meshList.empty())
            std::ranges::sort(meshList);

      for (int y = 0; y < p.height; y++)
            for (int x = 0; x < p.width; x++)
                  search(x, y);

      p.numGroups = p.writeMap.n;

      // Sort groups by ink vs. component then by morton code.
      std::ranges::sort(indexDict, [](Group const &a, Group const &b)
      {
            if (a.ink == b.ink)
                  return a.gid < b.gid;
            return a.ink < b.ink;
      });

      // List of connections.
      // Build state vector.
      p.states_is_native = true;
      p.states    = new InkState[p.writeMap.n];
      p.stateInks = new Ink[p.writeMap.n];
      p.states[0] = {0, false, Logic::None};

      // Borrow writeMap for a reverse mapping
      p.writeMap.ptr = new int[p.writeMap.n + 1];

      for (int i = 0; i < p.writeMap.n; ++i) {
            Group const &g = indexDict[i];
            InkState    &s = p.states[i];
            s.logic        = g.logic;
            s.visited      = false;
            s.activeInputs = 0;

            p.stateInks[i]        = g.ink;
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
      for (auto const key : conList | std::views::keys)
            accu[key] += 1;

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
                  --p.states[second].activeInputs;

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
      if (visited[0][top_idx] & 1)
            return;

      // Check what ink this group is of
      InkPixel const pix = p.image[top_idx];
      Ink            ink = pix.ink;
      int            gid;

      switch (ink) {  // NOLINT(clang-diagnostic-switch-enum)
      case Ink::None:
      case Ink::Cross:
      case Ink::TunnelOff:
      case Ink::Mesh:
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
            break;

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

      default:
            gid = p.writeMap.n++;
            break;
      }

      // DFS
      stack.emplace_back(x, y);
      visited[0][top_idx] |= 1;

      while (!stack.empty())
      {
            auto const comp = stack.back();
            auto const idx  = calc_index(comp);
            stack.pop_back();
            if (p.image[idx].ink != Ink::Mesh)
                  p.indexImage[idx] = gid;

            for (unsigned nindex = 0; nindex < 4; ++nindex)
            {
                  auto const &neighbor = fourNeighbors[nindex];
                  glm::ivec2 newComp   = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int  newIdx = calc_index(newComp);
                  Ink  newInk = p.image[newIdx].ink;

                  // Handle wire bundles.
                  if (ink != Ink::BusOff && newInk == Ink::BusOff) {
                        // What kind of ink are we again?
                        auto const bus_pix = p.image[idx];
                        auto const mask    = get_mask(bus_pix);

                        if (mask != 0) {
                              if (visited[0][newIdx] & mask)
                                    continue;

                              // Hold my beer, we're jumping in.
                              explore_bus(newComp, bus_pix, mask);

                              if (visited[0][newIdx] & 1) {
                                    auto const otherIdx = p.indexImage[newIdx];
                                    auto const shifted  = static_cast<int64_t>(gid) << 32;
                                    if (bundleConsSet.insert(shifted | gid).second)
                                          bundleCons.emplace(gid, otherIdx);
                              }
                              continue;
                        }
                  }

                  if (visited[0][newIdx] & 1) {
                        if (ink == Ink::BusOff && newInk != Ink::BusOff && get_mask(p.image[idx])) {
                              // Try to insert new connection
                              tryInsertConnection(gid, newIdx);
                        }
                        continue;
                  }

                  // Check ink type and handle crosses
                  if (newInk == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;

                        newIdx = calc_index(newComp);
                        if (visited[0][newIdx] & 1)
                              continue;
                        newInk = p.image[newIdx].ink;
                        if (newInk == Ink::TunnelOff)
                              continue;
                  }
                  else if (newInk == Ink::TunnelOff) {
                        if (!handle_tunnel(nindex, false, idx, newIdx, newInk, newComp))
                              continue;
                  }
                  else if (newInk == Ink::Mesh) {
                        auto const mask = get_mask(pix);
                        if (visited[0][newIdx] & mask)
                              continue;
                        visited[0][newIdx] |= mask;
                        for (auto const mesh : meshList)
                              stack.push_back(get_pos_from_index(mesh));
                        continue;
                  }

                  // Push back if Allowable
                  if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
                        readInks.push_back(newComp);
                        visited[0][newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
                        writeInks.push_back(newComp);
                        visited[0][newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == ink) {
                        visited[0][newIdx] |= 1;
                        stack.push_back(newComp);
                  }
            }
      }

      // Add on the new group
      indexDict.push_back({gid, (inkLogicType(ink)), pix.ink});
}

void
Preprocessor::explore_bus(glm::ivec2 const pos, InkPixel const &pix, uint64_t const mask)
{
      auto const idx = calc_index(pos);
      bundleStack.push_back(pos);
      visited[0][idx] |= mask;

      while (!bundleStack.empty())
      {
            glm::ivec2 const comp = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (unsigned nindex = 0; nindex < 4; ++nindex) {
                  auto const neighbor = fourNeighbors[nindex];
                  auto       newComp  = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int   newIdx = calc_index(newComp);
                  auto &newVis = visited[0][newIdx];
                  auto  newPix = p.image[newIdx];

                  // Let's not waste time.
                  if (newPix.ink == Ink::None)
                        continue;

                  // Handle different inks
                  if (newPix.ink == Ink::ReadOff) {
                        if (newVis & 1)
                              continue;
                        readInks.push_back(newComp);
                        if (pix.ink == Ink::ReadOff) {
                              newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink == Ink::WriteOff) {
                        if (newVis & 1)
                              continue;
                        writeInks.push_back(newComp);
                        if (pix.ink == Ink::WriteOff) {
                              newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink == Ink::TraceOff) {
                        if (newVis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if (pix.meta == newPix.meta) {
                              newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink != Ink::BusOff && pix == newPix) {
                        if (newVis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if (!(newVis & mask)) {
                              newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }

                  if (newPix.ink == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;

                        newIdx = calc_index(newComp);
                        if (newVis & mask)
                              continue;
                        newPix.ink = p.image[newIdx].ink;
                  }
                  else if (newPix.ink == Ink::TunnelOff) {
                        if (newVis & mask)
                              continue;
                        newVis |= mask;
                        if (!handle_tunnel(nindex, true, idx, newIdx, newPix.ink, newComp))
                              continue;
                  }
                  else if (newPix.ink == Ink::Mesh) {
                        if (visited[1][newIdx] & mask)
                              continue;
                        visited[1][newIdx] |= mask;
                        for (auto const mesh : meshList)
                              bundleStack.push_back(get_pos_from_index(mesh));
                        continue;
                  }

                  if (newPix.ink == Ink::BusOff) {
                        newVis = visited[0][newIdx];
                        if (newVis & mask)
                              continue;
                        newVis |= mask;
                        bundleStack.push_back(newComp);
                  }
            }
      }
}

bool
Preprocessor::handle_tunnel(unsigned const nindex,
                            bool const     ignoreMask,
                            int32_t const  idx,
                            int           &newIdx,
                            Ink           &newInk,
                            glm::ivec2    &newComp)
{
      auto const &neighbor = fourNeighbors[nindex];
      auto const  origComp = newComp;
      auto const  origPix  = p.image[calc_index(origComp - neighbor)];

      if (!ignoreMask) {
            auto const mask = get_tunnel_mask(nindex);
            if (visited[0][idx] & mask)
                  return false;
            visited[0][idx] |= mask;
      }

      for (auto tunComp = newComp + neighbor; ; tunComp += neighbor)
      {
            if (!validate_vector(tunComp)) {
                  push_tunnel_exit_not_found_error(neighbor, newComp);
                  return false;
            }
            auto tunIdx = calc_index(tunComp);
            if (p.image[tunIdx].ink != Ink::TunnelOff)
                  continue;
            auto &tunVis = visited[0][tunIdx];

      retry:
            tunComp += neighbor;
            if (!validate_vector(tunComp)) {
                  push_tunnel_exit_not_found_error(neighbor, newComp);
                  return false;
            }

            tunIdx      = calc_index(tunComp);
            auto tunPix = p.image[tunIdx];

            if (tunPix.ink == Ink::TunnelOff)
                  goto retry;

            if (tunPix == origPix) {
                  if (!ignoreMask) {
                        auto const mask = get_tunnel_mask(nindex + 2);
                        if (tunVis & mask)
                              return false;
                        tunVis |= mask;
                        visited[0][tunIdx] |= mask;
                  }
                  newComp = tunComp;
                  newInk  = tunPix.ink;
                  newIdx  = tunIdx;
                  return true;
            }

            auto errComp = tunComp - neighbor - neighbor;
            errComp -= neighbor;
            tunIdx = calc_index(errComp);
            tunPix = p.image[tunIdx];
            if (tunPix == p.image[idx]) {
                  push_invalid_tunnel_entrance_error(origComp, errComp);
                  return false;
            }
      }
}

/*--------------------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------------------*/


void Preprocessor::tryInsertConnection(int const gid, int const newIdx)
{
      auto const otherIdx = p.indexImage[newIdx];
      auto const shifted  = static_cast<int64_t>(gid) << 32;
      if (bundleConsSet.insert(shifted | otherIdx).second)
            bundleCons.emplace(otherIdx, gid);
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

glm::ivec2 Preprocessor::get_pos_from_index(int const idx) const
{
      auto const [quot, rem] = ::div(idx, p.width);
      return {rem, quot};
}


void
Preprocessor::push_tunnel_exit_not_found_error(glm::ivec2 const neighbor,
                                               glm::ivec2 const tunComp) const
{
      char const *dir = neighbor.x == 1    ? "east"
                        : neighbor.x == -1 ? "west"
                        : neighbor.y == 1  ? "south"
                                           : "north";
      char      *buf  = p.error_messages->push_blank(128);
      auto const size = ::sprintf(
          buf,
          R"(Error @ (%d, %d): No exit tunnel found in a search to the %s.)",
          tunComp.x, tunComp.y, dir);
      util::logs(buf, size);
}


void
Preprocessor::push_invalid_tunnel_entrance_error(glm::ivec2 const origComp,
                                                 glm::ivec2 const tmpComp) const
{
      char      *buf  = p.error_messages->push_blank(128);
      auto const size = ::sprintf(
            buf,
            "Error @ (%d, %d) & (%d, %d): Two consecutive tunnel entrances for the same ink found.",
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
