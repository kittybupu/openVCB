// Code for image proprocessing and graph generation

// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"
#include <tbb/spin_mutex.h>

namespace openVCB {
/*======================================================================================*/


class Preprocessor
{
      static_assert(sizeof(int) == sizeof(int32_t));

      using ivec = glm::ivec2;

      struct Group {
            int   gid;
            Logic logic;
            Ink   ink;
      };

    public:
      static void preprocess(Project &proj)
      {
            Preprocessor instance(proj);
            instance.run();
      }

      /*--------------------------------------------------------------------------------*/

    protected:
      explicit Preprocessor(Project &proj) noexcept
          : p(proj),
            canvas_size(static_cast<size_t>(p.width) * static_cast<size_t>(p.height)),
            visited(canvas_size)
      {}

      void run();

    private:
      void search(ivec vec);
      void explore_bus(std::vector<ivec> &stack, ivec pos, InkPixel pix, uint64_t mask, InkPixel busPix);
      bool handle_tunnel(unsigned nindex, bool ignoreMask, int idx, int &newIdx, Ink &newInk, ivec &newComp);
      void handle_read_ink(ivec vec);
      void handle_write_ink(ivec vec);

      std::pair<Ink, int> identify_ink(ivec vec, Ink ink);

      ND bool validate_vector(ivec nvec) const;
      ND int  calc_index(ivec vec) const;
      ND int  calc_index(int x, int y) const;
      ND ivec get_pos_from_index(int idx) const;

      void push_tunnel_exit_not_found_error(ivec neighbor, ivec tunComp) const;
      void push_invalid_tunnel_entrance_error(ivec origComp, ivec tmpComp) const;

      /*--------------------------------------------------------------------------------*/

      Project           &p;
      size_t const       canvas_size;
      std::vector<ivec>  readInks;
      std::vector<ivec>  writeInks;
      std::vector<Group> indexDict;
      std::vector<int>   meshIDs;
      std::array<int, 4> wirelessIDs = {-1, -1, -1, -1};

      // Hash sets to keep track of unique connections
      std::unordered_set<int64_t>       conSet;
      std::vector<std::pair<int, int>>  conList;
      std::unordered_multimap<int, int> busCons;
      std::unordered_set<int64_t>       busConsSet;

      class visited_IDs {
            std::array<std::unique_ptr<uint64_t[]>, 2> ptrs_;
            std::array<std::span<uint64_t>, 2>         spans_;

          public:
            explicit visited_IDs(size_t const canvas_size)
                : ptrs_({std::make_unique<uint64_t[]>(canvas_size),
                         std::make_unique<uint64_t[]>(canvas_size)}),
                  spans_({std::span<uint64_t>(ptrs_[0].get(), canvas_size),
                          std::span<uint64_t>(ptrs_[1].get(), canvas_size)})
            {}

            ND auto &normal() & { return spans_[0]; }
            ND auto &mesh()   & { return spans_[1]; }
      } visited;

      /*--------------------------------------------------------------------------------*/

      ND static constexpr uint64_t get_mask(InkPixel const &pix)
      {
            return get_mask(pix.ink, pix.meta);
      }

      ND static constexpr uint64_t get_mask(Ink const ink, unsigned const meta = 0)
      {
            switch (ink) {
            case Ink::TraceOff:      return UINT64_C(2) << meta;
            case Ink::TunnelOff:     return UINT64_C(2) << (16 + (meta % 4));
            case Ink::BusOff:        return UINT64_C(2) << (20 + meta);
            case Ink::ReadOff:       return UINT64_C(2) << 26;
            case Ink::WriteOff:      return UINT64_C(2) << 27;
            case Ink::BufferOff:     return UINT64_C(2) << 28;
            case Ink::OrOff:         return UINT64_C(2) << 29;
            case Ink::NandOff:       return UINT64_C(2) << 30;
            case Ink::NotOff:        return UINT64_C(2) << 31;
            case Ink::NorOff:        return UINT64_C(2) << 32;
            case Ink::AndOff:        return UINT64_C(2) << 33;
            case Ink::XorOff:        return UINT64_C(2) << 34;
            case Ink::XnorOff:       return UINT64_C(2) << 35;
            case Ink::ClockOff:      return UINT64_C(2) << 36;
            case Ink::LatchOff:      return UINT64_C(2) << 37;
            case Ink::Latch:         return UINT64_C(2) << 38;
            case Ink::LedOff:        return UINT64_C(2) << 39;
            case Ink::TimerOff:      return UINT64_C(2) << 40;
            case Ink::RandomOff:     return UINT64_C(2) << 41;
            case Ink::BreakpointOff: return UINT64_C(2) << 42;
            case Ink::Wireless1Off:  return UINT64_C(2) << 43;
            case Ink::Wireless2Off:  return UINT64_C(2) << 44;
            case Ink::Wireless3Off:  return UINT64_C(2) << 45;
            case Ink::Wireless4Off:  return UINT64_C(2) << 46;
            default:                 return 0;
            }
      }
};


/*--------------------------------------------------------------------------------------*/


void
Preprocessor::run()
{
      delete p.error_messages;
      p.error_messages = new StringArray(32);

       // Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
      for (ssize_t i = 0; i < static_cast<ssize_t>(canvas_size); i++) {
            switch (p.image[i].ink) {
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
      p.indexImage = new int[canvas_size];
      std::fill_n(p.indexImage, canvas_size, -1);

      // We need to find the location of each mesh entity before proceeding.
      for (unsigned i = 0; i < canvas_size; ++i)
            if (p.image[i].ink == Ink::Mesh)
                  meshIDs.push_back(i);
      if (!meshIDs.empty())
            std::ranges::sort(meshIDs);

      for (int y = 0; y < p.height; ++y)
            for (int x = 0; x < p.width; ++x)
                  search({x, y});

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
      p.states           = new InkState[p.writeMap.n];
      p.stateInks        = new Ink[p.writeMap.n];
      p.states[0]        = {0, false, Logic::None};

      // Borrow writeMap for a reverse mapping
      p.writeMap.ptr = new int[p.writeMap.n + 1];
      // The following line represents the culmination of 3 hours of debugging.
      memset(p.writeMap.ptr, 0, (p.writeMap.n + 1) * sizeof(int));

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
      for (unsigned i = 0; i < canvas_size; ++i) {
            int &idx = p.indexImage[i];
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
      for (auto const &key : conList | std::views::keys)
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
Preprocessor::search(ivec const vec)
{
      int const top_idx = calc_index(vec);
      if (visited.normal()[top_idx] & 1)
            return;

      // Check what ink this group is of
      InkPixel const topPix = p.image[top_idx];
      auto const [ink, gid] = identify_ink(vec, topPix.ink);
      if (gid == -1)
            return;

      // DFS
      std::vector stack{vec};
      visited.normal()[top_idx] |= 1;

      while (!stack.empty())
      {
            auto const comp    = stack.back();
            auto const idx     = calc_index(comp);
            bool const is_mesh = p.image[idx].ink == Ink::Mesh;
            stack.pop_back();

            if (!is_mesh)
                  p.indexImage[idx] = gid;

            for (unsigned nindex = 0; nindex < 4; ++nindex)
            {
                  auto const &neighbor = fourNeighbors[nindex];
                  ivec newComp   = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int newIdx = calc_index(newComp);
                  Ink newInk = p.image[newIdx].ink;

                  if (newInk == Ink::None)
                        continue;

                  // Handle wire buses.
                  if (ink != Ink::BusOff && newInk == Ink::BusOff) {
                        // What kind of ink are we again?
                        auto const pix  = p.image[idx];
                        auto const mask = get_mask(pix);

                        if (mask != 0) {
                              if (!(visited.normal()[newIdx] & 1)) {
                                    // BUG What the heck is this doing?
                                    std::vector<ivec> backup;
                                    std::swap(stack, backup);
                                    search(newComp);
                                    stack = std::move(backup);
                              }
                              if (visited.normal()[newIdx] & mask)
                                    continue;

                              auto const newPix   = p.image[newIdx];
                              auto const otherIdx = p.indexImage[newIdx];
                              // Hold my beer, we're jumping in.
                              explore_bus(stack, newComp, pix, mask, newPix);

                              if (visited.normal()[newIdx] & 1) {
                                    auto const shifted  = static_cast<int64_t>(gid) << 32;
                                    if (busConsSet.insert(shifted | gid).second)
                                          busCons.emplace(gid, otherIdx);
                              }
                              continue;
                        }
                  }

                  // Check ink type and handle crosses
                  if (newInk == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;
                        newIdx = calc_index(newComp);
                        if (visited.normal()[newIdx] & 1)
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
                        // Here we use the normal `visited` instance. The mesh instance is only needed for buses.
                        auto const prevComp = newComp - neighbor;
                        if (!validate_vector(prevComp))
                              continue;
                        int  const prevIdx = calc_index(prevComp);
                        auto const prevPix = p.image[prevIdx];
                        auto const mask    = get_mask(prevPix);

                        if (visited.normal()[newIdx] & mask)
                              continue;
                        visited.normal()[newIdx] |= mask;

                        for (auto const mesh : meshIDs) {
                              for (auto const n : fourNeighbors) {
                                    auto const tmpVec = get_pos_from_index(mesh) + n;
                                    if (!validate_vector(tmpVec))
                                          continue;
                                    if (p.image[calc_index(tmpVec)] == prevPix)
                                          stack.push_back(tmpVec);
                              }
                        }
                        continue;
                  }
                  else if (visited.normal()[newIdx] & 1) {
                        if (ink == Ink::BusOff && newInk != Ink::BusOff && get_mask(p.image[idx]) != 0) {
                              // Try to insert new connection
                              auto const otherIdx = p.indexImage[newIdx];
                              auto const shifted  = static_cast<int64_t>(gid) << 32;
                              if (busConsSet.insert(shifted | otherIdx).second)
                                    busCons.emplace(otherIdx, gid);
                        }
                        continue;
                  }

                  // Push back if Allowable
                  if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
                        readInks.push_back(newComp);
                        visited.normal()[newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
                        writeInks.push_back(newComp);
                        visited.normal()[newIdx] |= 1;
                        stack.push_back(newComp);
                  } else if (newInk == ink) {
                        visited.normal()[newIdx] |= 1;
                        stack.push_back(newComp);
                  }
            }
      }

      // Add on the new group
      indexDict.push_back({gid, (inkLogicType(ink)), topPix.ink});
}

std::pair<Ink, int>
Preprocessor::identify_ink(ivec const vec, Ink ink)
{
      int gid;

      switch (ink) {
      case Ink::None:
      case Ink::Cross:
      case Ink::TunnelOff:
      case Ink::Mesh:
      case Ink::Annotation:
      case Ink::Filler:
            return {Ink::None, -1};

      case Ink::ReadOff:
            readInks.push_back(vec);
            ink = Ink::TraceOff;
            gid = p.writeMap.n++;
            break;

      case Ink::WriteOff:
            writeInks.push_back(vec);
            ink = Ink::TraceOff;
            gid = p.writeMap.n++;
            break;

      case Ink::Wireless1Off:
      case Ink::Wireless2Off:
      case Ink::Wireless3Off:
      case Ink::Wireless4Off: {
            auto const i = Ink::Wireless4Off - ink;
            if (wirelessIDs[i] < 0)
                  wirelessIDs[i] = p.writeMap.n++;
            gid = wirelessIDs[i];
            break;
      }

      default:
            gid = p.writeMap.n++;
            break;
      }

      return {ink, gid};
}

/*--------------------------------------------------------------------------------------*/

void
Preprocessor::explore_bus(std::vector<ivec> &stack,
                          ivec const         pos,
                          InkPixel const     pix,
                          uint64_t const     mask,
                          InkPixel const     busPix)
{
      std::vector busStack{pos};
      int const   idx = calc_index(pos);
      visited.normal()[idx] |= mask;

      while (!busStack.empty())
      {
            ivec const comp = busStack.back();
            busStack.pop_back();

            // Check four directions
            for (unsigned nindex = 0; nindex < 4; ++nindex) {
                  auto const neighbor = fourNeighbors[nindex];
                  auto       newComp  = comp + neighbor;
                  if (!validate_vector(newComp))
                        continue;

                  int   newIdx = calc_index(newComp);
                  auto *newVis = &visited.normal()[newIdx];
                  auto  newPix = p.image[newIdx];

                  // Let's not waste time.
                  if (newPix.ink == Ink::None)
                        continue;

                  if (newPix.ink == Ink::ReadOff) {
                        if (*newVis & 1)
                              continue;
                        readInks.push_back(newComp);
                        if (pix.ink == Ink::ReadOff) {
                              *newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink == Ink::WriteOff) {
                        if (*newVis & 1)
                              continue;
                        writeInks.push_back(newComp);
                        if (pix.ink == Ink::WriteOff) {
                              *newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink == Ink::TraceOff) {
                        if (*newVis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if (pix.meta == newPix.meta) {
                              *newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }
                  if (newPix.ink != Ink::BusOff && pix == newPix) {
                        if (*newVis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if (!(*newVis & mask)) {
                              *newVis |= 1;
                              stack.push_back(newComp);
                        }
                        continue;
                  }

                  if (newPix.ink == Ink::Cross) {
                        newComp += neighbor;
                        if (!validate_vector(newComp))
                              continue;
                        newIdx = calc_index(newComp);
                        newVis = &visited.normal()[newIdx];
                        if (*newVis & mask)
                              continue;
                        newPix = p.image[newIdx];
                        if (newPix.ink == Ink::TunnelOff)
                              continue;
                  }
                  else if (newPix.ink == Ink::TunnelOff) {
                        if (*newVis & mask)
                              continue;
                        *newVis |= mask;
                        if (!handle_tunnel(nindex, true, idx, newIdx, newPix.ink, newComp))
                              continue;
                        newVis = &visited.normal()[newIdx];
                  }
                  else if (newPix.ink == Ink::Mesh) {
                        if (visited.mesh()[newIdx] & mask)
                              continue;
                        visited.mesh()[newIdx] |= mask;

                        for (auto const mesh : meshIDs) {
                              for (auto const n : fourNeighbors) {
                                    auto const tmpVec = get_pos_from_index(mesh) + n;
                                    if (!validate_vector(tmpVec))
                                          continue;
                                    auto const tmpPix = p.image[calc_index(tmpVec)];
                                    if (tmpPix == busPix)
                                          busStack.push_back(tmpVec);
                              }
                        }

                        continue;
                  }

                  if (newPix.ink == Ink::BusOff) {
                        if (*newVis & mask)
                              continue;
                        *newVis |= mask;
                        busStack.push_back(newComp);
                  }
            }
      }
}

bool
Preprocessor::handle_tunnel(unsigned const nindex,
                            bool const     ignoreMask,
                            int const      idx,
                            int           &newIdx,
                            Ink           &newInk,
                            ivec          &newComp)
{
      auto const &neighbor = fourNeighbors[nindex];
      auto const  origComp = newComp;
      auto const  origPix  = p.image[calc_index(origComp - neighbor)];
      auto const  origMask = get_mask(Ink::TunnelOff, nindex);

      if (!ignoreMask) {
            if (visited.normal()[idx] & origMask)
                  return false;
            visited.normal()[idx] |= origMask;
      }

      for (ivec tunComp = newComp + neighbor; ; tunComp += neighbor)
      {
            if (!validate_vector(tunComp)) {
                  if (!ignoreMask)
                        push_tunnel_exit_not_found_error(neighbor, newComp);
                  return false;
            }
            auto tunIdx = calc_index(tunComp);
            if (p.image[tunIdx].ink != Ink::TunnelOff)
                  continue;
            auto &tunVis = visited.normal()[tunIdx];

      retry:
            tunComp += neighbor;
            if (!validate_vector(tunComp)) {
                  if (!ignoreMask)
                        push_tunnel_exit_not_found_error(neighbor, newComp);
                  return false;
            }

            tunIdx      = calc_index(tunComp);
            auto tunPix = p.image[tunIdx];

            if (tunPix.ink == Ink::TunnelOff)
                  goto retry;

            if (tunPix == origPix) {
                  if (!ignoreMask) {
                        // get_mask() for tunnels ensures that its argument is between 0 and 3.
                        auto const mask = get_mask(Ink::TunnelOff, nindex + 2);
                        if (tunVis & mask)
                              return false;
                        tunVis |= mask;
                        visited.normal()[tunIdx] |= mask;
                  }
                  newComp = tunComp;
                  newInk  = tunPix.ink;
                  newIdx  = tunIdx;
                  return true;
            }

            ivec const errComp = tunComp - neighbor - neighbor;
            tunIdx = calc_index(errComp);
            tunPix = p.image[tunIdx];

            if (tunPix == p.image[idx]) {
                  if (!ignoreMask)
                        push_invalid_tunnel_entrance_error(origComp, errComp);
                  return false;
            }
      }
}

/*--------------------------------------------------------------------------------------*/

void
Preprocessor::handle_read_ink(ivec const vec)
{
      int const idx    = calc_index(vec);
      int const srcGID = p.indexImage[idx];

      for (auto neighbor : fourNeighbors) {
            ivec const nvec = vec + neighbor;
            if (!validate_vector(nvec))
                  continue;

            auto const newIdx = calc_index(nvec);
            auto const ink  = p.image[newIdx].ink;

            // Ignore any buses or clocks
            if (util::eq_any(ink, Ink::BusOff, Ink::ClockOff, Ink::TimerOff))
                  continue;

            auto const dstGID  = p.indexImage[newIdx];
            if (srcGID != dstGID && dstGID != -1) {
                  auto const shifted = static_cast<int64_t>(srcGID) << 32;
                  if (conSet.insert(shifted | dstGID).second)
                        conList.emplace_back(srcGID, dstGID);
            }
      }
}

void
Preprocessor::handle_write_ink(ivec const vec)
{
      int const dstGID = p.indexImage[calc_index(vec)];
      // Check if we got any wire buses as baggage
      auto const range = busCons.equal_range(indexDict[dstGID].gid);

      for (auto neighbor : fourNeighbors) {
            ivec const nvec = vec + neighbor;
            if (!validate_vector(nvec))
                  continue;
            auto const newIdx = calc_index(nvec);

            // Ignore any buses
            if (p.image[newIdx].ink == Ink::BusOff)
                  continue;

            auto const srcGID  = p.indexImage[newIdx];
            if (srcGID != dstGID && srcGID != -1) {
                  auto const shifted = static_cast<int64_t>(srcGID) << 32;

                  if (conSet.insert(shifted | dstGID).second) {
                        conList.emplace_back(srcGID, dstGID);
                        // Tack on those for the bus too
                        for (auto itr = range.first; itr != range.second; ++itr) {
                              int busID = p.writeMap.ptr[itr->second];
                              if (conSet.insert(shifted | busID).second)
                                    conList.emplace_back(srcGID, busID);
                        }
                  }
            }
      }
}

/*--------------------------------------------------------------------------------------*/

bool Preprocessor::validate_vector(ivec const nvec) const
{
      return nvec.x >= 0 && nvec.x < p.width  &&
             nvec.y >= 0 && nvec.y < p.height;
}

int Preprocessor::calc_index(ivec const vec) const
{
      return vec.x + vec.y * p.width;
}

int Preprocessor::calc_index(int const x, int const y) const
{
      return x + y * p.width;
}

glm::ivec2 Preprocessor::get_pos_from_index(int const idx) const
{
      auto const [quot, rem] = ::div(idx, p.width);
      return {rem, quot};
}

void
Preprocessor::push_tunnel_exit_not_found_error(ivec const neighbor,
                                               ivec const tunComp) const
{
      char const *dir = neighbor.x == 1    ? "east"
                        : neighbor.x == -1 ? "west"
                        : neighbor.y == 1  ? "south"
                                           : "north";
      char *buf = p.error_messages->push_blank(128);
      // Overflow isn't possible for this size of buffer, so sprintf is fine.
      auto const size = ::sprintf(
          buf, R"(Error @ (%d, %d): No exit tunnel found in a search to the %s.)",
          tunComp.x, tunComp.y, dir);
      util::logs(buf, size);
}

void
Preprocessor::push_invalid_tunnel_entrance_error(ivec const origComp,
                                                 ivec const tmpComp) const
{
      char *buf = p.error_messages->push_blank(128);
      // Overflow isn't possible for this size of buffer, so sprintf is fine.
      auto const size = ::sprintf(
            buf, "Error @ (%d, %d) & (%d, %d): Two consecutive tunnel entrances for the same ink found.",
            origComp.x, origComp.y, tmpComp.x, tmpComp.y);
      util::logs(buf, size);
}


/*--------------------------------------------------------------------------------------*/


[[__gnu__::__hot__]] void
Project::preprocess()
{
      Preprocessor::preprocess(*this);

      static constexpr char endlnmsg[] = "Preprocessing completed.\n"
                                         "----------------------------------------"
                                         "----------------------------------------";
      util::logs(endlnmsg, std::size(endlnmsg) - 1);
}


/*======================================================================================*/
} // namespace openVCB
