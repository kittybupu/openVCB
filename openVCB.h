#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Enable multithreading
// #define OVCB_MT

#ifdef OVCB_MT
#include <atomic>
#endif

/// <summary>
/// TODO: 
/// Try sorting by component as well. 
/// I see it going both ways
/// 
/// </summary>

namespace openVCB {
	struct LatchInterface {
		glm::ivec2 pos;
		glm::ivec2 stride;
		int numBits;
		int gids[64];
	};

	/// <summary>
	/// 5 bit state.
	/// 4 bit type + 1 active bit
	/// </summary>
	enum class Ink {
		None = 0,

		TraceOff = 1,
		ReadOff,
		WriteOff,
		Cross,

		BufferOff,
		OrOff,
		NandOff,

		NotOff,
		NorOff,
		AndOff,

		XorOff,
		XnorOff,

		ClockOff,
		LatchOff,
		LedOff,

		Trace = 17,
		Read,
		Write,

		Buffer = 21,
		Or,
		Nand,

		Not,
		Nor,
		And,

		Xor,
		Xnor,

		Clock,
		Latch,
		Led,

		size
	};

	extern const int colorPallet[];
	extern const char* inkNames[];

	// Sets the ink type to be on or off
	inline Ink setOn(Ink ink, bool state) {
		return (Ink)(((int)ink & 0xf) | (state << 4));
	}

	// Sets the ink type to be on
	inline Ink setOn(Ink ink) {
		return (Ink)(((int)ink & 0xf) | 16);
	}

	// Sets the ink type to be off
	inline Ink setOff(Ink ink) {
		return (Ink)((int)ink & 0xf);
	}

	// Gets the ink active state
	inline bool getOn(Ink ink) {
		return (int)ink >> 4;
	}

	namespace StateMask {
		constexpr int VISITED = 0x1;
	}

	struct InkState {
#ifdef OVCB_MT
		// Number of active inputs
		std::atomic<int16_t> activeInputs;

		// Flags for traversal
		std::atomic<unsigned char> visited;
#else
		// Number of active inputs
		int16_t activeInputs;
		// Flags for traversal
		unsigned char visited;
#endif

		// Current ink
		unsigned char ink;
	};

	struct SparseMat {
		// Size of the matrix
		int n;
		// Number of non-zero entries
		int nnz;
		// CSC sparse matrix ptr
		int* ptr;
		// CSC sparse matrix rows
		int* rows;
	};

	class Project {
	public:
		int* vmem; // null if vmem is not used
		size_t vmemSize = 0;
		std::string assembly;
		LatchInterface vmAddr;
		LatchInterface vmData;
		int lastVMemAddr = 0;

		~Project();

		int height;
		int width;

		// An image containing component indices
		unsigned char* originalImage;
		Ink* image;
		int* indexImage;
		int numGroups;

		// Adjacentcy matrix
		// By default, the indices from ink groups first and then component groups
		SparseMat writeMap;
		InkState* states;

		// Event queue
		int* updateQ[2];
		int16_t* lastActiveInputs;

#ifdef OVCB_MT
		std::atomic<int> qSize;
#else
		int qSize;
#endif

		// Builds a project from an image. Remember to configure VMem
		void readFromVCB(std::string p);

		// Samples the ink at a pixel. Returns ink and group id
		std::pair<Ink, int> sample(glm::ivec2 pos);

		// Assemble the vmem from this->assembly
		void assemblyVMem();

		// Dump vmem contents to file
		void dumpVMemToText(std::string p);

		void toggleLatch(glm::ivec2 pos);

		virtual void preprocess(bool optimize = true);
		virtual void tick(int numTicks = 1);

		// Emits an event if it is not yet in the queue
		inline bool tryEmit(int gid) {
#ifdef OVCB_MT
			unsigned char expect = 0;
			// Check if this event is already in queue
			if (states[gid].visited.compare_exchange_strong(expect, 1)) {
				updateQ[1][qSize.fetch_add(1)] = gid;
				return true;
			}
			return false;
#else
			// Check if this event is already in queue
			if (states[gid].visited) return false;
			states[gid].visited = 1;
			updateQ[1][qSize++] = gid;
			return true;
#endif
		}
	};

}