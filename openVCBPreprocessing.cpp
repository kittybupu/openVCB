// Code for image proprocessing and graph generation

// ReSharper disable CppTooWideScopeInitStatement
#include "openVCB.h"

#include <unordered_set>

#ifdef OVCB_USE_GORDER
# include "gorder/Graph.h"
# include "gorder/Util.h"
#endif


namespace openVCB {

struct Bundle
{
    private:
      using int_vec   = std::vector<int>;
      using ivec2_vec = std::vector<glm::ivec2>;

    public:
      explicit constexpr
            Bundle(int const       &width,
                   int const       &height,
                   InkPixel const  *image,
                   int_vec         &visited,
                   ivec2_vec       &stack,
                   ivec2_vec       &bundleStack,
                   ivec2_vec       &readInks,
                   ivec2_vec       &writeInks)
          : width(width),
            height(height),
            image(image),
            visited(visited),
            stack(stack),
            bundleStack(bundleStack),
            readInks(readInks),
            writeInks(writeInks)
      {}

      ~Bundle()                             = default;
      Bundle(Bundle const &)                = default;
      Bundle(Bundle &&) noexcept            = default;
      Bundle &operator=(Bundle const &)     = delete;
      Bundle &operator=(Bundle &&) noexcept = delete;

      void explore(glm::ivec2 pos, int mask) const;

    private:
      int const      &width;
      int const      &height;
      InkPixel const *image;
      int_vec        &visited;
      ivec2_vec      &stack;
      ivec2_vec      &bundleStack;
      ivec2_vec      &readInks;
      ivec2_vec      &writeInks;
};


void Bundle::explore(glm::ivec2 const pos, int const mask) const
{
      bundleStack.push_back(pos);
      visited[pos.x + pos.y * width] |= mask;

      while (!bundleStack.empty()) {
            auto const p = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (auto const & neighbor : fourNeighbors)
            {
                  glm::ivec2 np = p + neighbor;
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  int       nidx = np.x + np.y * width;
                  int const nvis = visited[nidx];

                  // Check if already visited
                  if (nvis & mask)
                        continue;

                  InkPixel const & newPix = image[nidx];
                  Ink              newInk = newPix.ink;

                  // Handle different inks
                  if (newInk == Ink::ReadOff) {
                        if (nvis & 1)
                              continue;
                        readInks.push_back(np);
                        if ((mask >> 16) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(np);
                        }
                        continue;
                  }

                  if (newInk == Ink::WriteOff) {
                        if (nvis & 1)
                              continue;
                        writeInks.push_back(np);
                        if ((mask >> 17) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(np);
                        }
                        continue;
                  }

                  if (newInk == Ink::TraceOff) {
                        if (nvis & 1)
                              continue;
                        // We will only connect to traces of the matching color
                        if ((mask >> newPix.meta) == 2) {
                              visited[nidx] |= 1;
                              stack.push_back(np);
                        }
                        continue;
                  }

                  if (newInk == Ink::Cross) {
                        np += neighbor;
                        if (np.x < 0 || np.x > width || np.y < 0 || np.y > height)
                              continue;

                        nidx = np.x + np.y * width;
                        if (visited[nidx] & mask)
                              continue;
                        newInk = image[np.x + np.y * width].ink;
                  }

                  if (newInk == Ink::BundleOff) {
                        visited[nidx] |= mask;
                        bundleStack.push_back(np);
                  }
            }
      }
}

/*--------------------------------------------------------------------------------------*/

void
Project::preprocess()
{
      // Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
      for (int i = 0; i < width * height; ++i) {
            switch (auto ink = image[i].ink) {
            case Ink::Trace:
            case Ink::Read:
            case Ink::Write:
            case Ink::Buffer:
            case Ink::Or:
            case Ink::And:
            case Ink::Xor:
            case Ink::Not:
            case Ink::Nor:
            case Ink::Nand:
            case Ink::Xnor:
            case Ink::Clock:
            case Ink::Led:
            case Ink::Bundle:
                  ink          = setOff(ink);
                  image[i].ink = ink;
                  break;

            default:
                  /* Nothing */;
            }
      }

      std::vector             visited(width * height, 0);
      std::vector<glm::ivec2> stack;
      std::vector<glm::ivec2> bundleStack;
      std::vector<glm::ivec2> readInks;
      std::vector<glm::ivec2> writeInks;

      auto const bundle_handler = Bundle{width, height,      image,    visited,
                                         stack, bundleStack, readInks, writeInks};

      // Split up the ordering by ink vs. comp.
      // Hopefully groups things better in memory
      writeMap.n = 0;

      indexImage = new int[width * height];
      for (int i = 0, lim = width * height; i < lim; ++i)
            indexImage[i] = -1;

      using Group = std::tuple<int, Logic, Ink>;

      // This translates from morton ordering to sequential ordering
      std::vector<Group>                indexDict;
      std::unordered_multimap<int, int> bundleCons;
      std::unordered_set<int64_t>       bundleConsSet;

      // Connected Components Search
      for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                  if (visited[x + y * width] & 1)
                        continue;

                  // Check what ink this group is of
                  Ink ink = image[x + y * width].ink;
                  if (util::eq_any(ink, Ink::None, Ink::Cross, Ink::Filler, Ink::Annotation))
                        continue;
                  if (ink == Ink::ReadOff) {
                        readInks.emplace_back(x, y);
                        ink = Ink::TraceOff;
                  } else if (ink == Ink::WriteOff) {
                        writeInks.emplace_back(x, y);
                        ink = Ink::TraceOff;
                  }

                  // Allocate new group id
                  int const gid = writeMap.n;
                  ++writeMap.n;

                  // DFS
                  stack.emplace_back(x, y);
                  visited[x + y * width] |= 1;

                  while (!stack.empty())
                  {
                        glm::ivec2 const p = stack.back();
                        stack.pop_back();

                        int const idx   = p.x + p.y * width;
                        indexImage[idx] = gid;

                        for (auto const &fourNeighbor : fourNeighbors)
                        {
                              glm::ivec2 np = p + fourNeighbor;

                              if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                                    continue;

                              int       nidx = np.x + np.y * width;
                              int const nvis = visited[nidx];

                              // Handle wire bundles
                              Ink newInk = image[nidx].ink;
                              if (ink == Ink::TraceOff && newInk == Ink::BundleOff) {
                                    // Whoo wire bundles
                                    // What kind of ink are we again?
                                    InkPixel npix = image[idx];
                                    int      mask;

                                    mask = 2 << (npix.ink == Ink::ReadOff    ? 16
                                                 : npix.ink == Ink::WriteOff ? 17
                                                                             : npix.meta);

                                    if (nvis & mask)
                                          continue;

                                    // Hold my beer, we're jumping in.
                                    bundle_handler.explore(np, mask);

                                    if (nvis & 1) {
                                          // Try to insert new connection
                                          int otherIdx = indexImage[nidx];
                                          if (bundleConsSet.insert((static_cast<int64_t>(otherIdx) << 32) | gid).second)
                                                bundleCons.insert({gid, otherIdx});
                                    }

                                    continue;
                              }

                              // Check if already visited
                              if (nvis & 1) {
                                    if (ink == Ink::BundleOff && util::eq_any(newInk, Ink::TraceOff, Ink::ReadOff, Ink::WriteOff))
                                    {
                                          // Try to insert new connection
                                          int const otherIdx = indexImage[nidx];
                                          if (bundleConsSet.insert((static_cast<uint64_t>(gid) << 32) | otherIdx).second)
                                                bundleCons.insert({otherIdx, gid});
                                    }
                                    continue;
                              }

                              // Check ink type and handle crosses
                              if (newInk == Ink::Cross) {
                                    np += fourNeighbor;
                                    if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                                          continue;

                                    nidx = np.x + np.y * width;
                                    if (visited[nidx] & 1)
                                          continue;
                                    newInk = image[np.x + np.y * width].ink;
                              }

                              // Push back if Allowable
                              if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
                                    readInks.emplace_back(np);
                                    visited[nidx] |= 1;
                                    stack.emplace_back(np);
                              } else if (newInk == Ink::WriteOff &&
                                         ink == Ink::TraceOff) {
                                    writeInks.emplace_back(np);
                                    visited[nidx] |= 1;
                                    stack.emplace_back(np);
                              } else if (newInk == ink) {
                                    visited[nidx] |= 1;
                                    stack.emplace_back(np);
                              }
                        }
                  }

                  // Add on the new group
                  indexDict.emplace_back(gid, inkLogicType(ink), ink);
            }
      }
      numGroups = writeMap.n;

      // Sort groups by ink vs. component then by morton code.
      std::ranges::sort(indexDict, [](Group const &a, Group const &b) -> bool
      {
            if (std::get<2>(a) == std::get<2>(b))
                  return std::get<0>(a) < std::get<0>(b);
            return static_cast<int>(std::get<2>(a)) < static_cast<int>(std::get<2>(b));
      });

      // List of connections
      // Build state vector
      states    = new InkState[writeMap.n];
      stateInks = new Ink[writeMap.n];

      // Borrow writeMap for a reverse mapping
      writeMap.ptr = new int[writeMap.n + 1];
      for (int i = 0; i < writeMap.n; ++i) {
            auto const &g = indexDict[i];

            stateInks[i]           = std::get<2>(g);
            states[i].logic        = std::get<1>(g);
            states[i].visited      = false;
            states[i].activeInputs = 0;

            writeMap.ptr[std::get<0>(g)] = i;
      }

      // Remap indices
      for (size_t i = 0, lim = width * height; i < lim; ++i) {
            int idx = indexImage[i];
            if (idx >= 0)
                  indexImage[i] = writeMap.ptr[idx];
      }

      // printf("Found %i groups.\n", numGroups);
      // printf("Found %i read inks and %i write inks.\n", readInks.size(),
      // writeInks.size());

      // Hash sets to keep track of unique connections
      std::unordered_set<int64_t>         conSet;
      std::vector<std::pair<int, int>> conList;

      // Add connections from inks to components
      for (glm::ivec2 p : readInks) {
            int const srcGID = indexImage[p.x + p.y * width];

            for (auto const &neighbor : fourNeighbors) {
                  glm::ivec2 np = p + neighbor;
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  // Ignore any bundles or clocks
                  if (util::eq_any(image[np.x + np.y * width].ink, Ink::BundleOff, Ink::ClockOff))
                        continue;

                  int const dstGID = indexImage[np.x + np.y * width];
                  if (srcGID != dstGID && dstGID != -1 &&
                      conSet.insert(((int64_t)srcGID << 32) | dstGID).second)
                        conList.emplace_back(srcGID, dstGID);
            }
      }

      size_t numRead2Comp = conSet.size();
      conSet.clear();

      // Add connections from components to inks
      for (glm::ivec2 p : writeInks) {
            int const dstGID = indexImage[p.x + p.y * width];

            // Check if we got any wire bundles as baggage
            int  oldTraceIdx = std::get<0>(indexDict[dstGID]);
            auto range       = bundleCons.equal_range(oldTraceIdx);

            for (auto const &neighbor : fourNeighbors) {
                  glm::ivec2 np = p + neighbor;
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  // Ignore any bundles
                  if (image[np.x + np.y * width].ink == Ink::BundleOff)
                        continue;

                  int const srcGID = indexImage[np.x + np.y * width];
                  if (srcGID != dstGID && srcGID != -1 &&
                      conSet.insert(((int64_t)srcGID << 32) | dstGID).second) {
                        conList.emplace_back(srcGID, dstGID);

                        // Tack on those for the bundle too
                        for (auto itr = range.first; itr != range.second; ++itr) {
                              int bundleID = writeMap.ptr[itr->second];
                              if (conSet.insert(((int64_t)srcGID << 32) | bundleID)
                                      .second)
                                    conList.emplace_back(srcGID, bundleID);
                        }
                  }
            }
      }
      size_t numComp2Write = conSet.size();

      // printf("Found %zd ink->comp and %zd comp->ink connections (%i total).\n",
      // numRead2Comp, numComp2Write, numRead2Comp + numComp2Write);

#ifdef OVCB_USE_GORDER
      // Gorder
      {
            Gorder::Graph g;
            std::vector   list(conList);
            g.readGraph(list, writeMap.n);

            std::vector<int> transformOrder;
            g.Transform(transformOrder);

            std::vector<int> order;
            g.GorderGreedy(order, 64);

            auto oldStates = states;
            states         = new InkState[writeMap.n];
            for (int i = 0; i < writeMap.n; ++i)
                  states[order[transformOrder[i]]] = oldStates[i];
            delete[] oldStates;

            for (auto &i : conList) {
                  auto edge   = i; // BUG: Is this copy needed?
                  edge.first  = order[transformOrder[edge.first]];
                  edge.second = order[transformOrder[edge.second]];
                  // printf("a %i %i\n", edge.first, edge.second);
                  i = edge;
            }
            for (size_t i = 0; i < width * height; ++i) {
                  int idx = indexImage[i];
                  if (idx >= 0)
                        indexImage[i] = order[transformOrder[idx]];
            }
      }
#endif

      // Stores rows per colume.
      std::vector accu(writeMap.n, 0);
      for (auto const &e : conList)
            ++accu[e.first];

      // Construct adjacentcy matrix
      writeMap.nnz             = static_cast<int>(conList.size());
      writeMap.ptr[writeMap.n] = writeMap.nnz;
      writeMap.rows            = new int[writeMap.nnz];

      // Prefix sum
      int c = 0;
      for (int i = 0; i < writeMap.n; ++i) {
            writeMap.ptr[i] = c;
            c += accu[i];
            accu[i] = 0;
      }

      // Populate
      for (auto const &[first, second] : conList) {
            // Set the active inputs of AND to be -numInputs
            Ink dstInk = stateInks[second];
            if (util::eq_any(dstInk, Ink::AndOff, Ink::NandOff))
                  --states[second].activeInputs;

            writeMap.rows[writeMap.ptr[first] + accu[first]++] = second;
      }

      // Sort rows
      for (int i = 0; i < writeMap.n; ++i) {
            int start = writeMap.ptr[i];
            int end   = writeMap.ptr[i + 1];
            std::sort(&writeMap.rows[start], &writeMap.rows[end]);
      }

      updateQ[0]       = new int[writeMap.n];
      updateQ[1]       = new int[writeMap.n];
      lastActiveInputs = new int16_t[writeMap.n];
      qSize            = 0;

      // Insert starting events into the queue
      for (int i = 0; i < writeMap.n; ++i) {
            Ink ink = stateInks[i];
            if (ink == Ink::ClockOff)
                  clockGIDs.push_back(i);
            else if (util::eq_any(ink, Ink::NandOff, Ink::NotOff, Ink::NorOff, Ink::XnorOff, Ink::Latch))
                  updateQ[0][qSize++] = i;
            if (ink == Ink::Latch)
                  states[i].activeInputs = 1;
      }
}

} // namespace openVCB