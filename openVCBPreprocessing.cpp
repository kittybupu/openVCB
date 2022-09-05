// Code for image proprocessing and graph generation

#include "openVCB.h"
#include <unordered_set>
#include <algorithm>


#include "gorder/Graph.h"
#include "gorder/Util.h"

namespace openVCB {
	using namespace std;
	using namespace glm;

	void Project::preprocess(bool useGorder) {
		std::vector<bool> visited(width * height, false);
		std::vector<ivec2> stack;
		std::vector<ivec2> readInks;
		std::vector<ivec2> writeInks;

		const ivec2 fourNeighbors[4]{
			ivec2(-1, 0),
			ivec2(0, 1),
			ivec2(1, 0),
			ivec2(0, -1)
		};

		// Split up the ordering by ink vs. comp. 
		// Hopefully groups things better in memory
		writeMap.n = 0;

		indexImage = new int[width * height];
		for (int i = 0, lim = width * height; i < lim; i++)
			indexImage[i] = -1;

		using Group = tuple<int, Ink>;
		// This translates from morton ordering to sequential ordering
		vector<Group> indexDict;

		// Connected Components Search
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				if (!visited[x + y * width]) {
					// Check what ink this group is of
					Ink ink = image[x + y * width];
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
						visited[idx] = true;
						indexImage[idx] = gid;

						for (int k = 0; k < 4; k++) {
							ivec2 np = p + fourNeighbors[k];
							if (np.x < 0 || np.x >= width ||
								np.y < 0 || np.y >= height) continue;

							int nidx = np.x + np.y * width;

							// Check if already visited
							if (visited[nidx]) continue;

							// Check ink type and handle crosses
							Ink newInk = image[nidx];
							if (newInk == Ink::Cross) {
								np += fourNeighbors[k];
								if (np.x < 0 || np.x > width ||
									np.y < 0 || np.y > height) continue;

								nidx = np.x + np.y * width;
								if (visited[nidx]) continue;
								newInk = image[np.x + np.y * width];
							}

							// Push back if Allowable
							if (newInk == Ink::ReadOff && ink == Ink::TraceOff) {
								readInks.push_back(ivec2(np.x, np.y));
								stack.push_back(ivec2(np.x, np.y));
							}
							else if (newInk == Ink::WriteOff && ink == Ink::TraceOff) {
								writeInks.push_back(ivec2(np.x, np.y));
								stack.push_back(ivec2(np.x, np.y));
							}
							else if (newInk == ink)
								stack.push_back(ivec2(np.x, np.y));
						}
					}

					// Add on the new group
					indexDict.push_back({ gid, ink });
				}
		numGroups = writeMap.n;

		// Sort groups by ink vs. component then by morton code.
		sort(indexDict.begin(), indexDict.end(),
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

			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x >= width ||
					np.y < 0 || np.y >= height) continue;

				const int srcGID = indexImage[np.x + np.y * width];
				if (srcGID != dstGID && srcGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second)
					conList.push_back({ srcGID, dstGID });
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