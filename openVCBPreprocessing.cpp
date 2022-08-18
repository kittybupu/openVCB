
#include "openVCB.h"
#include <unordered_set>
#include <algorithm>

// #define __BMI2__
// #include <libmorton/morton.h>

namespace openVCB {
	using namespace std;
	using namespace glm;

	void Project::preprocess(bool optimize) {
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

		using Group = tuple<long long, int, Ink>;
		// This translates from morton ordering to sequential ordering
		vector<Group> indexDict;

		// Connected Components Search
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				if (!visited[x + y * width]) {
					// Check what ink this group is of
					Ink ink = image[x + y * width];
					if (ink == Ink::Cross || ink == Ink::None)
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
					int mCount = 0;
					long long mSum = 0;
					stack.push_back(ivec2(x, y));
					while (stack.size()) {
						const ivec2 p = stack.back();
						stack.pop_back();

						const int idx = p.x + p.y * width;
						visited[idx] = true;
						indexImage[idx] = gid;

						// Calculate the average morton code
						mCount++;
						// mSum += libmorton::morton2D_32_encode((uint_fast16_t)p.x, (uint_fast16_t)p.y);

						for (int k = 0; k < 4; k++) {
							ivec2 np = p + fourNeighbors[k];
							if (np.x < 0 || np.x > width ||
								np.y < 0 || np.y > height) continue;

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
					indexDict.push_back({ mSum / mCount, gid, ink });
				}
		numGroups = writeMap.n;

		// Sort groups by ink vs. component then by morton code.
		if (optimize)
			sort(indexDict.begin(), indexDict.end(),
				[](const Group& a, const Group& b) -> bool {
					if (std::get<2>(a) == std::get<2>(b))
						return std::get<1>(a) < std::get<1>(b);
					return (int)std::get<2>(a) < (int)std::get<2>(b);
				});

		// List of connections
		std::vector<ivec2> conList;
		// Build state vector
		states = new InkState[writeMap.n];
		// Borrow writeMap for a reverse mapping
		writeMap.ptr = new int[writeMap.n + 1];
		for (int i = 0; i < writeMap.n; i++) {
			auto g = indexDict[i];
			InkState& s = states[i];
			s.ink = (unsigned char)std::get<2>(g);
			s.visited = 0;
			s.activeInputs = 0;

			writeMap.ptr[std::get<1>(g)] = i;
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
		// Stores rows per colume.
		std::vector<int> accu(writeMap.n, 0);

		// Add connections from inks to components
		for (ivec2 p : readInks) {
			const int srcGID = indexImage[p.x + p.y * width];

			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x > width ||
					np.y < 0 || np.y > height) continue;

				const int dstGID = indexImage[np.x + np.y * width];
				if (srcGID != dstGID && dstGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second) {
					conList.push_back(ivec2(srcGID, dstGID));
					accu[srcGID]++;
				}
			}
		}

		size_t numRead2Comp = conSet.size();
		conSet.clear();

		// Add connections from components to inks
		for (ivec2 p : writeInks) {
			const int dstGID = indexImage[p.x + p.y * width];

			for (int k = 0; k < 4; k++) {
				ivec2 np = p + fourNeighbors[k];
				if (np.x < 0 || np.x > width ||
					np.y < 0 || np.y > height) continue;

				const int srcGID = indexImage[np.x + np.y * width];
				if (srcGID != dstGID && srcGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second) {
					conList.push_back(ivec2(srcGID, dstGID));
					accu[srcGID]++;
				}
			}
		}
		size_t numComp2Write = conSet.size();

		// printf("Found %zd ink->comp and %zd comp->ink connections (%d total).\n", numRead2Comp, numComp2Write, numRead2Comp + numComp2Write);

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
			unsigned char dstInk = states[con.y].ink;
			if (dstInk == (unsigned char)Ink::AndOff ||
				dstInk == (unsigned char)Ink::NandOff)
				states[con.y].activeInputs--;

			writeMap.rows[writeMap.ptr[con.x] + (accu[con.x]++)] = con.y;
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