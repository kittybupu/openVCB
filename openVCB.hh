#pragma once
#ifndef C8kWpReCttGxHsWkLLl1RDjAweb3HDua
#define C8kWpReCttGxHsWkLLl1RDjAweb3HDua

/*
 * This is the primary header ifle for openVCB
 */

#include "Common.hh"

#include "openVCB_Data.hh"


/// Enable multithreading
#if !defined _DEBUG && 0
# define OVCB_MT 1
#endif


// Should we use gorder?
//#define OVCB_USE_GORDER 1


namespace openVCB {
/****************************************************************************************/


struct LatchInterface {
      glm::ivec2 pos;
      glm::ivec2 stride;
      glm::ivec2 size;
      int        numBits;
      int        gids[64];
};


/// <summary>
/// 8 bit state.
/// 7 bit type + 1 active bit
///
/// Add new logic behaivor here.
/// All inks define their logic from this.
/// This is seperate from Ink to simplify switch logic
/// </summary>
enum class Logic : uint8_t {
      NonZeroOff,
      ZeroOff,
      XorOff,
      XnorOff,
      LatchOff,
      ClockOff,

      numTypes,

      NonZero = 128,
      Zero,
      Xor,
      Xnor,
      Latch,
      Clock
};

constexpr Logic operator>>(Logic val, int n) { return static_cast<Logic>(static_cast<unsigned>(val) >> n); }
constexpr Logic operator<<(Logic val, int n) { return static_cast<Logic>(static_cast<unsigned>(val) << n); }
constexpr Logic operator&(Logic val1, uint8_t val2) { return static_cast<Logic>(static_cast<unsigned>(val1) & val2); }
constexpr Logic operator|(Logic val1, uint8_t val2) { return static_cast<Logic>(static_cast<unsigned>(val1) | val2); }

template <typename T>
    requires util::concepts::Integral<T>
constexpr bool operator==(Logic const op1, T const op2)
{
      return op1 == static_cast<Logic>(op2);
}


// Sets the ink type to be on or off
constexpr Logic setOn(Logic const logic, unsigned const state)
{
      return (logic & 0x7F) | (state << 7);
}

// Sets the ink type to be on
constexpr Logic setOn(Logic const logic)
{
      return (logic & 0x7F) | 128;
}

// Sets the ink type to be off
constexpr Logic setOff(Logic const logic)
{
      return logic & 0x7F;
}

// Gets the ink active state
constexpr bool getOn(Logic const logic)
{
      return (logic >> 7) != 0;
}


/*--------------------------------------------------------------------------------------*/


/// <summary>
/// 8 bit state.
/// 7 bit type + 1 active bit
///
/// Add potential ink types here.
/// It is VERY important to keep components in pairs for on and off to
/// to make sure the values are aligned correctly.
/// </summary>
enum class Ink : uint16_t {
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
      BundleOff,

      Filler,
      Annotation,

      numTypes,

      Trace = 129,
      Read,
      Write,
      InvalidCross,

      Buffer,
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
      Bundle,

      InvalidFiller,
      InvalidAnnotation
};

constexpr Ink operator>>(Ink const val, unsigned const n) { return static_cast<Ink>(static_cast<unsigned>(val) >> n); }
constexpr Ink operator<<(Ink const val, unsigned const n) { return static_cast<Ink>(static_cast<unsigned>(val) << n); }
constexpr Ink operator&(Ink const val1, unsigned const val2) { return static_cast<Ink>(static_cast<unsigned>(val1) & val2); }
constexpr Ink operator|(Ink const val1, unsigned const val2) { return static_cast<Ink>(static_cast<unsigned>(val1) | val2); }

template <typename T>
    requires util::concepts::Integral<T>
constexpr bool operator==(Ink const op1, T const op2)
{
      return op1 == static_cast<Ink>(op2);
}


// Sets the ink type to be on or off
constexpr Ink setOn(Ink const ink, unsigned const state)
{
      return (ink & 0x7F) | (state << 7);
}

// Sets the ink type to be on
constexpr Ink setOn(Ink const ink)
{
      return (ink & 0x7F) | 128;
}

// Sets the ink type to be off
constexpr Ink setOff(Ink const ink)
{
      return ink & 0x7F;
}

// Gets the ink active state
constexpr bool getOn(Ink const ink)
{
      return (ink >> 7) != 0;
}


// Gets the logic type of said ink
// Define the logics of each ink here.
inline Logic inkLogicType(Ink ink)
{
      ink = setOff(ink);

      switch (ink) {
      case Ink::NotOff:
      case Ink::NorOff:
      case Ink::AndOff:
            return Logic::ZeroOff;

      case Ink::XorOff:
            return Logic::XorOff;

      case Ink::XnorOff:
            return Logic::XnorOff;

      case Ink::LatchOff:
            return Logic::LatchOff;

      case Ink::ClockOff:
            return Logic::ClockOff;

      default:
            return Logic::NonZeroOff;
      }
}

// Gets the string name of the ink
extern char const *getInkString(Ink ink);


/*--------------------------------------------------------------------------------------*/


struct InkState
{
      /*
       * The constructors are unfortunately manditory if std::atomic derived types are
       * used. They cannot be copied or moved by default.
       */
#ifdef OVCB_MT
      InkState()  = default;
      ~InkState() = default;

      InkState(InkState const &other)
          : activeInputs(other.activeInputs.load(std::memory_order::relaxed)),
            visited(other.visited.load(std::memory_order::relaxed)),
            logic(other.logic)
      {}

      InkState(InkState &&other) noexcept
          : activeInputs(other.activeInputs.exchange(0, std::memory_order::relaxed)),
            visited(other.visited.exchange(0, std::memory_order::relaxed)),
            logic(other.logic)
      {
            other.logic = {};
      }

      InkState &operator=(InkState const &other)
      {
            activeInputs = other.activeInputs.load(std::memory_order::relaxed);
            visited      = other.visited.load(std::memory_order::relaxed);
            logic        = other.logic;
            return *this;
      }

      InkState &operator=(InkState &&other) noexcept
      {
            activeInputs = other.activeInputs.exchange(0, std::memory_order::relaxed);
            visited      = other.visited.exchange(0, std::memory_order::relaxed);
            logic        = other.logic;
            other.logic  = {};
            return *this;
      }

      // Number of active inputs
      std::atomic<int16_t> activeInputs;

      // Flags for traversal
      std::atomic<uint8_t> visited;
#else
      // Number of active inputs
      int16_t activeInputs;
      // Flags for traversal
      uint8_t visited;
#endif

      // Current logic state
      Logic logic;
};

static_assert(sizeof(InkState) == 4);

struct SparseMat {
      // Size of the matrix
      int n;
      // Number of non-zero entries
      int nnz;
      // CSC sparse matrix ptr
      int *ptr;
      // CSC sparse matrix rows
      int *rows;
};

// This represents a pixel with meta data as well as simulation ink type
// Ths is for inks that have variants which do not affect simulation
// i.e. Colored traces.
struct InkPixel {
      Ink     ink;
      int16_t meta;
};

static_assert(sizeof(InkPixel) == 4);

// This is for asyncronous exchange of data
// for stuff like audio and signal scopes
struct InstrumentBuffer {
      InkState *buffer;
      int       bufferSize;
      int       idx;
};

struct SimulationResult {
      int64_t numEventsProcessed;
      int     numTicksProcessed;
      bool    breakpoint;
};

class Project
{
    public:
      int           *vmem     = nullptr; // null if vmem is not used
      size_t         vmemSize = 0;
      std::string    assembly;
      LatchInterface vmAddr{};
      LatchInterface vmData{};
      int            lastVMemAddr = 0;

      int height = 0;
      int width  = 0;

      // An image containing component indices
      uint8_t  *originalImage = nullptr;
      InkPixel *image         = nullptr;
      int      *indexImage    = nullptr;
      int      *decoration[3] = {nullptr, nullptr, nullptr}; // on / off / unknown

      uint32_t ledPalette[16] = {
            0x323841, 0xFFFFFF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00
      };

      int numGroups = 0;

      // Adjacentcy matrix
      // By default, the indices from ink groups first and then component groups
      SparseMat writeMap = {};

      // Stores the logic states of each group
      InkState *states = nullptr;

      // Stores the actual ink type of each group
      Ink *stateInks = nullptr;

      // Map of symbols during assembleVmem()
      std::unordered_map<std::string, int64_t> assemblySymbols;
      std::unordered_map<int64_t, int64_t>     lineNumbers;
      std::vector<InstrumentBuffer>            instrumentBuffers;
      std::map<int, Logic>                     breakpoints;

      uint64_t tickNum = 0;

      // Event queue
      int     *updateQ[2]{nullptr, nullptr};
      int16_t *lastActiveInputs = nullptr;

#ifdef OVCB_MT
      std::atomic<int> qSize;
#else
      int qSize;
#endif

      //---------------------------------------------------------------------------------

      Project() = default;
      ~Project();

      Project(Project const &)                = delete;
      Project &operator=(Project const &)     = delete;
      Project(Project &&) noexcept            = delete;
      Project &operator=(Project &&) noexcept = delete;

      //---------------------------------------------------------------------------------

      // Builds a project from an image. Remember to configure VMem
      void readFromVCB(std::string p);

      // Decode base64 data from clipboard, then process logic data
      bool readFromBlueprint(std::string clipboardData);

      // Decompress zstd logic data to an image
      bool processLogicData(std::vector<unsigned char> logicData, int headerSize);

      // Decompress zstd decoration data to an image
      static void processDecorationData(std::vector<unsigned char> decorationData,
                                        int                      *&originalImage);

      // Samples the ink at a pixel. Returns ink and group id
      [[nodiscard]] std::pair<Ink, int> sample(glm::ivec2 pos) const;

      // Assemble the vmem from this->assembly
      void assembleVmem(char *err = nullptr, size_t errSize = 256);

      // Dump vmem contents to file
      void dumpVMemToText(std::string const &p) const;

      // Toggles the latch at position.
      // Does nothing if it's not a latch
      void toggleLatch(glm::ivec2 pos);

      // Toggles the latch at position.
      // Does nothing if it's not a latch
      void toggleLatch(int gid);

      // Preprocesses the image into the simulation format
      // Note: Gorder is often slower. It is here as an experiment
      void preprocess(bool useGorder = false);

      // Methods to add and remove breakpoints
      void addBreakpoint(int gid);
      void removeBreakpoint(int gid);

      /// Advances the simulation by n ticks
      SimulationResult tick(int numTicks = 1, int64_t maxEvents = INT64_C(0x7FFFFFFFFFFFFFFF));

      /// Emits an event if it is not yet in the queue
      bool tryEmit(int gid)
      {
            // Check if this event is already in queue
#ifdef OVCB_MT
            uint8_t expect = 0;
            if (states[gid].visited.compare_exchange_strong(expect, 1,
                                                            std::memory_order::relaxed))
            {
                  updateQ[1][qSize.fetch_add(1, std::memory_order::relaxed)] = gid;
                  return true;
            }
            return false;
#else
            if (states[gid].visited)
                  return false;
            states[gid].visited = 1;
            updateQ[1][qSize++] = gid;
            return true;
#endif
      }
};


/****************************************************************************************/
} // namespace openVCB
#endif
