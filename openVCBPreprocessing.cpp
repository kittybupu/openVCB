// Code for image proprocessing and graph generation

#include "openVCB.hh"
#include "utils.hh"

#include <algorithm>
#include <unordered_set>

#include "gorder/Graph.h"
#include "gorder/Util.h"


namespace openVCB {

constexpr glm::ivec2 fourNeighbors[4]{glm::ivec2(-1, 0), glm::ivec2(0, 1),
                                      glm::ivec2(1, 0), glm::ivec2(0, -1)};


void
exploreBundle(glm::ivec2 const         pos,  // XXX: Should be reference but I don't want to change signatures
              int const                mask,
              InkPixel                *image,
              int const                width,
              int const                height,
              std::vector<int>        &visited,
              std::vector<glm::ivec2> &stack,
              std::vector<glm::ivec2> &bundleStack,
              std::vector<glm::ivec2> &readInks,
              std::vector<glm::ivec2> &writeInks)
{

      bundleStack.push_back(pos);
      visited[pos.x + pos.y * width] |= mask;

      while (!bundleStack.empty()) {
            glm::ivec2 const p = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (auto const &fourNeighbor : fourNeighbors)
            {
                  glm::ivec2 np = p + fourNeighbor;
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  int       nidx = np.x + np.y * width;
                  int const nvis = visited[nidx];
                  // Check if already visited
                  if (nvis & mask)
                        continue;

                  InkPixel newPix = image[nidx];
                  Ink      newInk = newPix.ink;

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
                        np += fourNeighbor;
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

void
Project::preprocess(bool useGorder)
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

      std::vector<int>   visited(width * height, 0);
      std::vector<glm::ivec2> stack;
      std::vector<glm::ivec2> bundleStack;
      std::vector<glm::ivec2> readInks;
      std::vector<glm::ivec2> writeInks;

      // Split up the ordering by ink vs. comp.
      // Hopefully groups things better in memory
      writeMap.n = 0;

      indexImage = new int[width * height];
      for (int i = 0, lim = width * height; i < lim; i++)
            indexImage[i] = -1;

      using Group = std::tuple<int, Logic, Ink>;
      // This translates from morton ordering to sequential ordering
      std::vector<Group> indexDict;

      std::unordered_multimap<int, int> bundleCons;
      std::unordered_set<long long>     bundleConsSet;

      // Connected Components Search
      for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                  if (visited[x + y * width] & 1)
                        continue;

                  // Check what ink this group is of
                  Ink ink = image[x + y * width].ink;
                  if (util::eq_any(ink, Ink::Cross , Ink::None , Ink::Annotation , Ink::Filler))
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

                                    if (npix.ink == (int16_t)Ink::ReadOff)
                                          mask = 2 << 16;
                                    else if (npix.ink == (int16_t)Ink::WriteOff)
                                          mask = 2 << 17;
                                    else
                                          mask = 2 << npix.meta;

                                    if (nvis & mask)
                                          continue;

                                    // Hold my beer, we're jumping in.
                                    exploreBundle(np, mask, image, width, height,
                                                  visited, stack, bundleStack,
                                                  readInks, writeInks);

                                    if (nvis & 1) {
                                          // Try to insert new connection
                                          int otherIdx = indexImage[nidx];
                                          if (bundleConsSet .insert(((int64_t)otherIdx << 32) | gid).second)
                                                bundleCons.insert({gid, otherIdx});
                                    }

                                    continue;
                              }

                              // Check if already visited
                              if (nvis & 1) {
                                    if (ink == Ink::BundleOff &&
                                        //(newInk == Ink::TraceOff || newInk == Ink::ReadOff || newInk == Ink::WriteOff)
                                        util::eq_any(newInk, Ink::TraceOff, Ink::ReadOff, Ink::WriteOff)
                                        )
                                    {
                                          // Try to insert new connection
                                          int otherIdx = indexImage[nidx];
                                          if (bundleConsSet.insert(((uint64_t)gid << 32) | otherIdx).second)
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
      std::sort(indexDict.begin(), indexDict.end(),
                [](Group const &a, Group const &b) -> bool {
                      if (std::get<2>(a) == std::get<2>(b))
                            return std::get<0>(a) < std::get<0>(b);
                      return (int)std::get<2>(a) < (int)std::get<2>(b);
                });

      // List of connections
      // Build state vector
      states    = new InkState[writeMap.n];
      stateInks = new Ink[writeMap.n];
      // Borrow writeMap for a reverse mapping
      writeMap.ptr = new int[writeMap.n + 1];
      for (int i = 0; i < writeMap.n; i++) {
            auto g = indexDict[i];

            stateInks[i] = std::get<2>(g);

            InkState &s    = states[i];
            s.logic        = std::get<1>(g);
            s.visited      = 0;
            s.activeInputs = 0;

            writeMap.ptr[std::get<0>(g)] = i;
      }

      // Remap indices
      for (size_t i = 0, lim = width * height; i < lim; i++) {
            int idx = indexImage[i];
            if (idx >= 0)
                  indexImage[i] = writeMap.ptr[idx];
      }

      // printf("Found %d groups.\n", numGroups);
      // printf("Found %d read inks and %d write inks.\n", readInks.size(),
      // writeInks.size());

      // Hash sets to keep track of unique connections
      std::unordered_set<long long>         conSet;
      std::vector<std::pair<int, int>> conList;
      // Add connections from inks to components
      for (glm::ivec2 p : readInks) {
            int const srcGID = indexImage[p.x + p.y * width];

            for (int k = 0; k < 4; k++) {
                  glm::ivec2 np = p + fourNeighbors[k];
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  // Ignore any bundles
                  if (image[np.x + np.y * width].ink == Ink::BundleOff)
                        continue;

                  int const dstGID = indexImage[np.x + np.y * width];
                  if (srcGID != dstGID && dstGID != -1 &&
                      conSet.insert(((long long)srcGID << 32) | dstGID).second)
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

            for (int k = 0; k < 4; k++) {
                  glm::ivec2 np = p + fourNeighbors[k];
                  if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height)
                        continue;

                  // Ignore any bundles
                  if (image[np.x + np.y * width].ink == Ink::BundleOff)
                        continue;

                  int const srcGID = indexImage[np.x + np.y * width];
                  if (srcGID != dstGID && srcGID != -1 &&
                      conSet.insert(((long long)srcGID << 32) | dstGID).second) {
                        conList.emplace_back(srcGID, dstGID);

                        // Tack on those for the bundle too
                        for (auto itr = range.first; itr != range.second; ++itr) {
                              int bundleID = writeMap.ptr[itr->second];
                              if (conSet.insert(((long long)srcGID << 32) | bundleID)
                                      .second)
                                    conList.emplace_back(srcGID, bundleID);
                        }
                  }
            }
      }
      size_t numComp2Write = conSet.size();

      // printf("Found %zd ink->comp and %zd comp->ink connections (%d total).\n",
      // numRead2Comp, numComp2Write, numRead2Comp + numComp2Write);

#ifdef OVCB_USE_GORDER
      // Gorder
      if (useGorder) {
            Gorder::Graph g;
            std::vector   list(conList);
            g.readGraph(list, writeMap.n);

            std::vector<int> transformOrder;
            g.Transform(transformOrder);

            std::vector<int> order;
            g.GorderGreedy(order, 64);

            auto oldStates = states;
            states         = new InkState[writeMap.n];
            for (int i = 0; i < writeMap.n; i++)
                  states[order[transformOrder[i]]] = oldStates[i];
            delete[] oldStates;

            for (auto &i : conList) {
                  auto edge   = i; // BUG: Is this copy needed?
                  edge.first  = order[transformOrder[edge.first]];
                  edge.second = order[transformOrder[edge.second]];
                  // printf("a %d %d\n", edge.first, edge.second);
                  i = edge;
            }
            for (size_t i = 0; i < width * height; i++) {
                  int idx = indexImage[i];
                  if (idx >= 0)
                        indexImage[i] = order[transformOrder[idx]];
            }
      }
#endif

      // Stores rows per colume.
      std::vector<int> accu(writeMap.n, 0);
      for (auto e : conList)
            accu[e.first]++;

      // Construct adjacentcy matrix
      writeMap.nnz             = conList.size();
      writeMap.ptr[writeMap.n] = writeMap.nnz;
      writeMap.rows            = new int[writeMap.nnz];

      // Prefix sum
      int c = 0;
      for (size_t i = 0; i < writeMap.n; i++) {
            writeMap.ptr[i] = c;
            c += accu[i];
            accu[i] = 0;
      }

      // Populate
      for (int i = 0; i < conList.size(); i++) {
            auto con = conList[i];

            // Set the active inputs of AND to be -numInputs
            Ink dstInk = stateInks[con.second];
            if (dstInk == Ink::AndOff || dstInk == Ink::NandOff)
                  --states[con.second].activeInputs;

            writeMap.rows[writeMap.ptr[con.first] + (accu[con.first]++)] = con.second;
      }

      // Sort rows
      for (int i = 0; i < writeMap.n; i++) {
            int start = writeMap.ptr[i];
            int end   = writeMap.ptr[i + 1];
            std::sort(&writeMap.rows[start], &writeMap.rows[end]);
      }

      updateQ[0]       = new int[writeMap.n];
      updateQ[1]       = new int[writeMap.n];
      lastActiveInputs = new int16_t[writeMap.n];
      qSize            = 0;

      // Insert starting events into the queue
      for (size_t i = 0; i < writeMap.n; i++) {
            Ink ink = stateInks[i];
            //if (ink == Ink::NotOff || ink == Ink::NorOff || ink == Ink::NandOff || ink == Ink::XnorOff || ink == Ink::ClockOff || ink == Ink::Latch)
            if (util::eq_any(ink, Ink::NotOff, Ink::NorOff, Ink::NandOff, Ink::XnorOff, Ink::ClockOff, Ink::Latch))
                  updateQ[0][qSize++] = i;
            if (ink == Ink::Latch)
                  states[i].activeInputs = 1;
      }
}

} // namespace openVCB