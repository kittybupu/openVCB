
#include "openVCB.h"

namespace openVCB {
	using namespace std;
	using namespace glm;

	void Project::toggleLatch(ivec2 pos) {
		if (pos.x < 0 || pos.x >= width ||
			pos.y < 0 || pos.y >= height)
			return;
		const int gid = indexImage[pos.x + pos.y * width];
		if (setOff((Ink)states[gid].ink) != Ink::LatchOff)
			return;
		states[gid].activeInputs = 1;
		if (states[gid].visited) return;
		states[gid].visited = 1;
		updateQ[0][qSize++] = gid;
	}

	Project::~Project() {
		if (originalImage) delete[] originalImage;
		if (image) delete[] image;
		if (indexImage) delete[] indexImage;
		if (writeMap.ptr) delete[] writeMap.ptr;
		if (writeMap.rows) delete[] writeMap.rows;
		if (states) delete[] states;
		if (updateQ[0]) delete[] updateQ[0];
		if (updateQ[1]) delete[] updateQ[1];
		if (lastActiveInputs) delete[] lastActiveInputs;
	}

	std::pair<Ink, int> Project::sample(ivec2 pos) {
		if (pos.x < 0 || pos.x >= width ||
			pos.y < 0 || pos.y >= height)
			return { Ink::None, -1 };

		int idx = pos.x + pos.y * width;
		Ink type = image[idx];
		idx = indexImage[idx];
		if (type == Ink::Cross) return { Ink::Cross, idx };

		if (idx == -1) return { Ink::None, -1 };

		unsigned char state = states[idx].ink;
		type = (Ink)(((int)type & 0xef) | (state & 0x10));
		return { type, idx };
	}
}