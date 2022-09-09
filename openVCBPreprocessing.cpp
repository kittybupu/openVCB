// Code for image proprocessing and graph generation

#include "openVCB.h"
#include <unordered_set>
#include <algorithm>
#include <unordered_map>


#include "gorder/Graph.h"
#include "gorder/Util.h"

namespace openVCB {
	using namespace std;
	using namespace glm;

	const ivec2 fourNeighbors[4]{
		ivec2(-1, 0),
		ivec2(0, 1),
		ivec2(1, 0),
		ivec2(0, -1)
	};

	void exploreBundle(ivec2 pos, int mask, InkPixel* image, int width, int height,
		std::vector<int>& visited,
		std::vector<ivec2>& stack, std::vector<ivec2>& bundleStack,
		std::vector<ivec2>& readInks, std::vector<ivec2>& writeInks) {

		bundleStack.push_back(pos);
		while (bundleStack.size()) {
			const ivec2 p = bundleStack.back();
			bundleStack.pop_back();

			const int bidx = p.x + p.y * width;
			visited[bidx] |= mask;

			// Check four directions
			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x >= width ||
					np.y < 0 || np.y >= height) continue;

				int nidx = np.x + np.y * width;
				const int nvis = visited[nidx];
				// Check if already visited
				if (nvis & mask) continue;

				InkPixel newPix = image[nidx];
				Ink newInk = newPix.getInk();

				// Handle different inks
				if (newInk == Ink::ReadOff) {
					if (nvis & 1) continue;
					readInks.push_back(np);
					if ((mask >> 16) == 2)
						stack.push_back(np);
					continue;
				}
				else if (newInk == Ink::WriteOff) {
					if (nvis & 1) continue;
					writeInks.push_back(np);
					if ((mask >> 17) == 2)
						stack.push_back(np);
					continue;
				}
				else if (newInk == Ink::TraceOff) {
					if (nvis & 1) continue;
					// We will only connect to traces of the matching color
					if ((mask >> newPix.meta) == 2)
						stack.push_back(np);
					continue;
				}
				else if (newInk == Ink::Cross) {
					np += fourNeighbors[k];
					if (np.x < 0 || np.x > width ||
						np.y < 0 || np.y > height) continue;

					nidx = np.x + np.y * width;
					if (visited[nidx] & mask) continue;
					newInk = image[np.x + np.y * width].getInk();
				}

				if (newInk == Ink::BundleOff)
					bundleStack.push_back(np);
			}
		}
	}

	void Project::preprocess(bool useGorder) {
		// Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
		for (size_t i = 0; i < width * height; i++) {
			Ink ink = (Ink)image[i].ink;
			switch (ink) {
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
				ink = setOff(ink);
				image[i].ink = (int16_t)ink;
			}
		}

		std::vector<int> visited(width * height, 0);
		std::vector<ivec2> stack;
		std::vector<ivec2> bundleStack;
		std::vector<ivec2> readInks;
		std::vector<ivec2> writeInks;

		// Split up the ordering by ink vs. comp. 
		// Hopefully groups things better in memory
		writeMap.n = 0;

		indexImage = new int[width * height];
		for (int i = 0, lim = width * height; i < lim; i++)
			indexImage[i] = -1;

		using Group = tuple<int, Ink>;
		// This translates from morton ordering to sequential ordering
		vector<Group> indexDict;

		unordered_multimap<int, int> bundleCons;
		unordered_set<long long> bundleConsSet;

		// Connected Components Search
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				if (!(visited[x + y * width] & 1)) {
					// Check what ink this group is of
					Ink ink = image[x + y * width].getInk();
					if (ink == Ink::Cross || ink == Ink::None ||
						ink == Ink::Annotation || ink == Ink::Filler)
						continue;
					else if (ink == Ink::ReadOff) {
						readInks.push_back(ivec2(x, y));
						ink = Ink::TraceOff;
					}
					else if (ink == Ink::WriteOff) {
						writeInks.push_back(ivec2(x, y));
						ink = Ink::TraceOff;
					}

					// Allocate new group id
					const int gid = writeMap.n;
					writeMap.n++;

					// DFS
					stack.push_back(ivec2(x, y));
					while (stack.size()) {
						const ivec2 p = stack.back();
						stack.pop_back();

						const int idx = p.x + p.y * width;
						visited[idx] |= 1;
						indexImage[idx] = gid;

						for (int k = 0; k < 4; k++) {
							ivec2 np = p + fourNeighbors[k];
							if (np.x < 0 || np.x >= width ||
								np.y < 0 || np.y >= height) continue;

							int nidx = np.x + np.y * width;
							const int nvis = visited[nidx];

							// Handle wire bundles
							Ink newInk = image[nidx].getInk();
							if (ink == Ink::TraceOff && newInk == Ink::BundleOff) {
								// Whoo wire bundles
								// What kind of ink are we again? 
								InkPixel npix = image[idx];
								int mask;

								if (npix.ink == (int16_t)Ink::ReadOff)
									mask = 2 << 16;
								else if (npix.ink == (int16_t)Ink::WriteOff)
									mask = 2 << 17;
								else
									mask = 2 << npix.meta;

								if (nvis & mask) continue;

								// Hold my beer, we're jumping in.
								exploreBundle(np, mask, image, width, height, visited, stack, bundleStack, readInks, writeInks);

								if (nvis & 1) {
									// Try to insert new connection
									int otherIdx = indexImage[nidx];
									if (bundleConsSet.insert(((long long)otherIdx << 32) | gid).second)
										bundleCons.insert({ gid, otherIdx });
								}

								continue;
							}

							// Check if already visited
							if (nvis & 1) {
								if (ink == Ink::BundleOff && (newInk == Ink::TraceOff || newInk == Ink::ReadOff || newInk == Ink::WriteOff)) {
									// Try to insert new connection
									int otherIdx = indexImage[nidx];
									if (bundleConsSet.insert(((long long)gid << 32) | otherIdx).second)
										bundleCons.insert({ otherIdx, gid });
								}
								continue;
							}

							// Check ink type and handle crosses
							if (newInk == Ink::Cross) {
								np += fourNeighbors[k];
								if (np.x < 0 || np.x > width ||
									np.y < 0 || np.y > height) continue;

								nidx = np.x + np.y * width;
								if (visited[nidx] & 1) continue;
								newInk = image[np.x + np.y * width].getInk();
							}

							// Push back if Allowable
							if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
								readInks.push_back(np);
								stack.push_back(np);
							}
							else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
								writeInks.push_back(np);
								stack.push_back(np);
							}
							else if (newInk == ink)
								stack.push_back(np);
						}
					}

					// Add on the new group
					indexDict.push_back({ gid, ink });
				}
		numGroups = writeMap.n;

		// Sort groups by ink vs. component then by morton code.
		std::sort(indexDict.begin(), indexDict.end(),
			[](const Group& a, const Group& b) -> bool {
				if (std::get<1>(a) == std::get<1>(b))
					return std::get<0>(a) < std::get<0>(b);
				return (int)std::get<1>(a) < (int)std::get<1>(b);
			});

		// List of connections
		// Build state vector
		states = new InkState[writeMap.n];
		// Borrow writeMap for a reverse mapping
		writeMap.ptr = new int[writeMap.n + 1];
		for (int i = 0; i < writeMap.n; i++) {
			auto g = indexDict[i];
			InkState& s = states[i];
			s.ink = (unsigned char)std::get<1>(g);
			if (s.ink == (unsigned char)Ink::BundleOff)
				s.ink = (unsigned char)Ink::TraceOff;
			s.visited = 0;
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
		// printf("Found %d read inks and %d write inks.\n", readInks.size(), writeInks.size());

		// Hash sets to keep track of unique connections
		unordered_set<long long> conSet;
		std::vector<std::pair<int, int>> conList;
		// Add connections from inks to components
		for (ivec2 p : readInks) {
			const int srcGID = indexImage[p.x + p.y * width];

			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x >= width ||
					np.y < 0 || np.y >= height) continue;

				// Ignore any bundles
				if (image[np.x + np.y * width].ink == (int16_t)Ink::BundleOff)
					continue;

				const int dstGID = indexImage[np.x + np.y * width];
				if (srcGID != dstGID && dstGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second)
					conList.push_back({ srcGID, dstGID });
			}
		}

		size_t numRead2Comp = conSet.size();
		conSet.clear();

		// Add connections from components to inks
		for (ivec2 p : writeInks) {
			const int dstGID = indexImage[p.x + p.y * width];

			// Check if we got any wire bundles as baggage
			int oldTraceIdx = std::get<0>(indexDict[dstGID]);
			auto range = bundleCons.equal_range(oldTraceIdx);

			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x >= width ||
					np.y < 0 || np.y >= height) continue;

				// Ignore any bundles
				if (image[np.x + np.y * width].ink == (int16_t)Ink::BundleOff)
					continue;

				const int srcGID = indexImage[np.x + np.y * width];
				if (srcGID != dstGID && srcGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second) {
					conList.push_back({ srcGID, dstGID });

					// Tack on those for the bundle too
					for (auto itr = range.first; itr != range.second; itr++) {
						int bundleID = writeMap.ptr[itr->second];
						if (conSet.insert(((long long)srcGID << 32) | bundleID).second)
							conList.push_back({ srcGID, bundleID });
					}
				}
			}
		}
		size_t numComp2Write = conSet.size();

		// printf("Found %zd ink->comp and %zd comp->ink connections (%d total).\n", numRead2Comp, numComp2Write, numRead2Comp + numComp2Write);

		// Gorder
		if (useGorder) {
			Gorder::Graph g;
			vector<pair<int, int>> list(conList);
			g.readGraph(list, writeMap.n);

			std::vector<int> transformOrder;
			g.Transform(transformOrder);

			std::vector<int> order;
			g.GorderGreedy(order, 64);

			//for (size_t i = 0; i < order.size(); i++) {
			//	printf("%d -> %d\n", i, order[i]);
			//}

			auto oldStates = states;
			states = new InkState[writeMap.n];
			for (int i = 0; i < writeMap.n; i++)
				states[order[transformOrder[i]]] = oldStates[i];
			delete[] oldStates;

			for (size_t i = 0; i < conList.size(); i++) {
				auto edge = conList[i];
				edge.first = order[transformOrder[edge.first]];
				edge.second = order[transformOrder[edge.second]];
				// printf("a %d %d\n", edge.first, edge.second);
				conList[i] = edge;
			}
			for (size_t i = 0; i < width * height; i++) {
				int idx = indexImage[i];
				if (idx >= 0)
					indexImage[i] = order[transformOrder[idx]];
			}

			// g.PrintReOrderedGraph(order);
		}

		// Stores rows per colume.
		std::vector<int> accu(writeMap.n, 0);
		for (auto e : conList)
			accu[e.first]++;

		// Construct adjacentcy matrix
		writeMap.nnz = conList.size();
		writeMap.ptr[writeMap.n] = writeMap.nnz;
		writeMap.rows = new int[writeMap.nnz];

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
			unsigned char dstInk = states[con.second].ink;
			if (dstInk == (unsigned char)Ink::AndOff ||
				dstInk == (unsigned char)Ink::NandOff)
				states[con.second].activeInputs--;

			writeMap.rows[writeMap.ptr[con.first] + (accu[con.first]++)] = con.second;
		}

		// Sort rows
		for (int i = 0; i < writeMap.n; i++) {
			int start = writeMap.ptr[i];
			int end = writeMap.ptr[i + 1];
			std::sort(&writeMap.rows[start], &writeMap.rows[end]);
		}

		updateQ[0] = new int[writeMap.n];
		updateQ[1] = new int[writeMap.n];
		lastActiveInputs = new int16_t[writeMap.n];
		qSize = 0;

		// Insert starting events into the queue
		for (size_t i = 0; i < writeMap.n; i++) {
			if ((Ink)states[i].ink == Ink::NotOff ||
				(Ink)states[i].ink == Ink::NorOff ||
				(Ink)states[i].ink == Ink::NandOff ||
				(Ink)states[i].ink == Ink::XnorOff ||
				(Ink)states[i].ink == Ink::ClockOff ||
				(Ink)states[i].ink == Ink::Latch)
				updateQ[0][qSize++] = i;

			if ((Ink)states[i].ink == Ink::Latch) {
				states[i].ink = (unsigned char)Ink::LatchOff;
				states[i].activeInputs = 1;
			}
		}

		// printf("%d\n", writeMap.n);
	}
}