// Code for image proprocessing and graph generation

// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"

namespace openVCB {
/*======================================================================================*/


class Preprocessor
{
      using Group = std::tuple<int32_t, Logic, Ink>;

    public:
      explicit Preprocessor(Project &proj) noexcept;

      ~Preprocessor()                                   = default;
      Preprocessor(Preprocessor const &)                = delete;
      Preprocessor(Preprocessor &&) noexcept            = delete;
      Preprocessor &operator=(Preprocessor const &)     = delete;
      Preprocessor &operator=(Preprocessor &&) noexcept = delete;

      /*--------------------------------------------------------------------------------*/

    private:
      void init() const;
      void find_entities();
      void search(int x, int y);
      void create_events();

      void explore_bus(glm::ivec2 pos, unsigned mask);
      void handle_read_inks(glm::ivec2 vec);
      void handle_write_inks(glm::ivec2 vec);

      /*--------------------------------------------------------------------------------*/

      Project      &p;
      ssize_t const canvas_size;

      std::vector<glm::ivec2> stack;
      std::vector<glm::ivec2> bundleStack;
      std::vector<glm::ivec2> readInks;
      std::vector<glm::ivec2> writeInks;
      std::vector<int32_t>    visited;
      std::vector<Group>      indexDict;

      std::unordered_multimap<int32_t, int32_t> bundleCons;
      std::unordered_set<int64_t>               bundleConsSet;

      // Hash sets to keep track of unique connections
      std::unordered_set<int64_t>              conSet;
      std::vector<std::pair<int32_t, int32_t>> conList;

};


Preprocessor::Preprocessor(Project &proj) noexcept
    : p(proj),
      canvas_size(static_cast<ssize_t>(p.width) * static_cast<ssize_t>(p.height)),
      visited(canvas_size, 0)
{
      init();
      find_entities();

      for (auto const &vec : readInks)
            handle_read_inks(vec);
      conSet.clear();
      for (auto const &vec : writeInks)
            handle_write_inks(vec);

      create_events();
}


void Preprocessor::init() const
{
      // Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
      for (int i = 0; i < canvas_size; i++) {
            switch (p.image[i].ink) {
            default:
                  break;
            case Ink::Trace: case Ink::Read:
            case Ink::Write: case Ink::Buffer:
            case Ink::Or:    case Ink::And:
            case Ink::Xor:   case Ink::Not:
            case Ink::Nor:   case Ink::Nand:
            case Ink::Xnor:  case Ink::Clock:
            case Ink::Led:   case Ink::Bus:
                  p.image[i].ink = setOff(p.image[i].ink);
            }
      }

      // Split up the ordering by ink vs. comp.
      // Hopefully groups things better in memory.
      p.writeMap.n = 0;
      p.indexImage = new int32_t[canvas_size];
      memset(p.indexImage, -1, canvas_size * sizeof *p.indexImage);
}


void Preprocessor::find_entities()
{
      for (int y = 0; y < p.height; y++)
            for (int x = 0; x < p.width; x++)
                  search(x, y);

      p.numGroups = p.writeMap.n;

      // Sort groups by ink vs. component then by morton code.
      std::ranges::sort(indexDict, [](Group const &a, Group const &b) -> bool {
            if (std::get<2>(a) == std::get<2>(b))
                  return std::get<0>(a) < std::get<0>(b);
            return static_cast<int>(std::get<2>(a)) <
                   static_cast<int>(std::get<2>(b));
      });

      // List of connections.
      // Build state vector.
      p.states    = new InkState[p.writeMap.n];
      p.stateInks = new Ink[p.writeMap.n];

      // Borrow writeMap for a reverse mapping
      p.writeMap.ptr = new int[p.writeMap.n + 1];

      for (int i = 0; i < p.writeMap.n; ++i)
      {
            auto const g   = indexDict[i];
            InkState  &s   = p.states[i];
            p.stateInks[i] = std::get<2>(g);
            s.logic        = std::get<1>(g);
            s.visited      = false;
            s.activeInputs = 0;

            p.writeMap.ptr[std::get<0>(g)] = i;
      }

      // Remap indices
      for (ssize_t i = 0; i < canvas_size; ++i) {
            auto &idx = p.indexImage[i];
            if (idx >= 0)
                  idx = p.writeMap.ptr[idx];
      }
}


void Preprocessor::search(int const x, int const y)
{
      if (!(visited[x + y * p.width] & 1))
      {
            // Check what ink this group is of
            Ink ink = p.image[x + y * p.width].ink;
            if (ink == Ink::Cross || ink == Ink::None || ink == Ink::Annotation || ink == Ink::Filler)
                  return;
            if (ink == Ink::ReadOff) {
                  readInks.emplace_back(x, y);
                  ink = Ink::TraceOff;
            } else if (ink == Ink::WriteOff) {
                  writeInks.emplace_back(x, y);
                  ink = Ink::TraceOff;
            }

            // Allocate new group id
            int const gid = p.writeMap.n;
            ++p.writeMap.n;

            // DFS
            stack.emplace_back(x, y);
            visited[x + y * p.width] |= 1;

            while (!stack.empty())
            {
                  auto const comp   = stack.back();
                  auto const idx    = comp.x + comp.y * p.width;
                  p.indexImage[idx] = gid;
                  stack.pop_back();

                  for (auto neighbor : fourNeighbors) {
                        glm::ivec2 ncomp = comp + neighbor;
                        if (ncomp.x < 0 || ncomp.x >= p.width)
                              continue;
                        if (ncomp.y < 0 || ncomp.y >= p.height)
                              continue;

                        int       nidx   = ncomp.x + ncomp.y * p.width;
                        int const nvis   = visited[nidx];
                        Ink       newInk = p.image[nidx].ink;

                        // Handle wire bundles.
                        // TODO This should support Read and Write inks!!
                        if (newInk == Ink::BusOff && util::eq_any(ink, Ink::TraceOff, Ink::ReadOff, Ink::WriteOff))
                        {
                              // What kind of ink are we again?
                              uint mask;
                              if (p.image[idx].ink == Ink::ReadOff)
                                    mask = 2U << (Ink::numTypes + 1U);
                              else if (p.image[idx].ink == Ink::WriteOff)
                                    mask = 2U << (Ink::numTypes + 2);
                              else
                                    mask = 2U << p.image[idx].meta;

                              if (nvis & mask)
                                    continue;

                              // Hold my beer, we're jumping in.
                              explore_bus(ncomp, mask);

                              if (nvis & 1) {
                                    // Try to insert new connection
                                    int otherIdx = p.indexImage[nidx];
                                    if (bundleConsSet.insert((int64_t(otherIdx) << 32) | gid).second)
                                          bundleCons.insert({gid, otherIdx});
                              }

                              continue;
                        }

                        // Check if already visited
                        if (nvis & 1) {
                              if (ink == Ink::BusOff && util::eq_any(newInk, Ink::TraceOff, Ink::ReadOff, Ink::WriteOff)) {
                                    // Try to insert new connection
                                    int otherIdx = p.indexImage[nidx];
                                    if (bundleConsSet.insert((int64_t(gid) << 32) | otherIdx).second)
                                          bundleCons.insert({otherIdx, gid});
                              }
                              continue;
                        }

                        // Check ink type and handle crosses
                        if (newInk == Ink::Cross) {
                              ncomp += neighbor;
                              if (ncomp.x < 0 || ncomp.x >= p.width || ncomp.y < 0 ||
                                  ncomp.y >= p.height)
                                    continue;

                              nidx = ncomp.x + ncomp.y * p.width;
                              if (visited[nidx] & 1)
                                    continue;
                              newInk = p.image[ncomp.x + ncomp.y * p.width].ink;
                        }

                        // Push back if Allowable
                        if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
                              readInks.push_back(ncomp);
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        } else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
                              writeInks.push_back(ncomp);
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        } else if (newInk == ink) {
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        }
                  }
            }

            // Add on the new group
            indexDict.emplace_back(gid, inkLogicType(ink), ink);
      }
}


void Preprocessor::explore_bus(glm::ivec2 const pos, unsigned const mask)
{
      bundleStack.push_back(pos);
      visited[pos.x + pos.y * p.width] |= mask;

      while (!bundleStack.empty())
      {
            glm::ivec2 const comp = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (auto neighbor : fourNeighbors)
            {
                  glm::ivec2 ncomp = comp + neighbor;
                  if (ncomp.x < 0 || ncomp.x >= p.width)
                        continue;
                  if (ncomp.y < 0 || ncomp.y >= p.height)
                        continue;

                  int       nidx = ncomp.x + ncomp.y * p.width;
                  int const nvis = visited[nidx];
                  // Check if already visited
                  if (nvis & mask)
                        continue;

                  auto [newInk, newMeta] = p.image[nidx];

                  // Handle different inks
                  if (newInk == Ink::ReadOff) {
                        if (nvis & 1)
                              continue;
                        readInks.push_back(ncomp);
                        if ((mask >> 16) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        }
                        continue;
                  }
                  if (newInk == Ink::WriteOff) {
                        if (nvis & 1)
                              continue;
                        writeInks.push_back(ncomp);
                        if ((mask >> 17) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        }
                        continue;
                  }
                  if (newInk == Ink::TraceOff) {
                        if (nvis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if ((mask >> newMeta) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(ncomp);
                        }
                        continue;
                  }
                  if (newInk == Ink::Cross) {
                        ncomp += neighbor;
                        if (ncomp.x < 0 || ncomp.x > p.width || ncomp.y < 0 || ncomp.y > p.height)
                              continue;

                        nidx = ncomp.x + ncomp.y * p.width;
                        if (visited[nidx] & mask)
                              continue;
                        newInk = p.image[ncomp.x + ncomp.y * p.width].ink;
                  }
                  if (newInk == Ink::BusOff) {
                        visited[nidx] |= mask;
                        bundleStack.push_back(ncomp);
                  }
            }
      }
}


void Preprocessor::handle_read_inks(glm::ivec2 const vec)
{
      int const srcGID = p.indexImage[vec.x + vec.y * p.width];

      for (auto neighbor : fourNeighbors) {
            glm::ivec2 nvec = vec + neighbor;
            if (nvec.x < 0 || nvec.x >= p.width || nvec.y < 0 || nvec.y >= p.height)
                  continue;

            // Ignore any bundles or clocks
            auto ink = p.image[nvec.x + nvec.y * p.width].ink;
            if (ink == Ink::BusOff || ink == Ink::ClockOff)
                  continue;

            int const dstGID = p.indexImage[nvec.x + nvec.y * p.width];
            if (srcGID != dstGID && dstGID != -1 && conSet.insert((int64_t(srcGID) << 32) | dstGID).second)
                  conList.emplace_back(srcGID, dstGID);
      }
}


void Preprocessor::handle_write_inks(glm::ivec2 const vec)
{
      int const dstGID = p.indexImage[vec.x + vec.y * p.width];

      // Check if we got any wire bundles as baggage
      auto oldTraceIdx = std::get<0>(indexDict[dstGID]);
      auto range       = bundleCons.equal_range(oldTraceIdx);

      for (auto neighbor : fourNeighbors) {
            glm::ivec2 const nvec = vec + neighbor;
            if (nvec.x < 0 || nvec.x >= p.width )
                  continue;
            if (nvec.y < 0 || nvec.y >= p.height)
                  continue;

            // Ignore any bundles
            if (p.image[nvec.x + nvec.y * p.width].ink == Ink::BusOff)
                  continue;

            int const srcGID = p.indexImage[nvec.x + nvec.y * p.width];

            if (srcGID != dstGID && srcGID != -1 && conSet.insert((int64_t(srcGID) << 32) | dstGID).second)
            {
                  conList.emplace_back(srcGID, dstGID);

                  // Tack on those for the bundle too
                  for (auto itr = range.first; itr != range.second; ++itr) {
                        int bundleID = p.writeMap.ptr[itr->second];
                        if (conSet.insert((int64_t(srcGID) << 32) | bundleID).second)
                              conList.emplace_back(srcGID, bundleID);
                  }
            }
      }
}


void Preprocessor::create_events()
{
      // Stores rows per colume.
      std::vector accu(p.writeMap.n, 0);
      for (auto const &e : conList)
            accu[e.first]++;

      // Construct adjacentcy matrix
      p.writeMap.nnz  = conList.size();
      p.writeMap.ptr[p.writeMap.n] = p.writeMap.nnz;
      p.writeMap.rows = new int[p.writeMap.nnz];

      // Prefix sum
      for (int i = 0, c = 0; i < p.writeMap.n; ++i) {
            p.writeMap.ptr[i] = c;
            c      += accu[i];
            accu[i] = 0;
      }

      // Populate
      for (auto const &[first, second] : conList) {
            // Set the active inputs of AND to be -numInputs
            Ink dstInk = p.stateInks[second];
            if (dstInk == Ink::AndOff || dstInk == Ink::NandOff)
                  --p.states[second].activeInputs;

            auto const idx = p.writeMap.ptr[first] + accu[first]++;
            p.writeMap.rows[idx] = second;
      }

      // Sort rows
      for (int i = 0; i < p.writeMap.n; i++) {
            auto const start = p.writeMap.ptr[i];
            auto const end   = p.writeMap.ptr[i + 1];
            std::sort(&p.writeMap.rows[start], &p.writeMap.rows[end]);
      }

      p.updateQ[0]       = new int[p.writeMap.n];
      p.updateQ[1]       = new int[p.writeMap.n];
      p.lastActiveInputs = new int16_t[p.writeMap.n];
      p.qSize            = 0;

      // Insert starting events into the queue
      for (int i = 0; i < p.writeMap.n; i++) {
            switch (p.stateInks[i]) {
            case Ink::ClockOff:
                  p.clockGIDs.push_back(i);
            case Ink::Latch:
                  p.states[i].activeInputs = 1;
                  [[fallthrough]];
            case Ink::NotOff:
            case Ink::NorOff:
            case Ink::NandOff:
            case Ink::XnorOff:
                  p.updateQ[0][p.qSize++] = i;
            default:;
            }
      }
}



/*--------------------------------------------------------------------------------------*/


[[__gnu__::__hot__]]
void Project::preprocess()
{
      Preprocessor(*this);
}


/*======================================================================================*/
} // namespace openVCB
