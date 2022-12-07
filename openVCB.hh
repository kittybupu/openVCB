#pragma once
#ifndef C8kWpReCttGxHsWkLLl1RDjAweb3HDua
#define C8kWpReCttGxHsWkLLl1RDjAweb3HDua

/*
 * This is the primary header ifle for openVCB
 */

#include "utils.hh"
#include <glm/glm.hpp>

/// Enable multithreading
#if !defined _DEBUG && false
# define OVCB_MT 1
#endif

//#define OVCB_USE_GORDER 1
//#define OVCB_BYTE_ORIENTED_VMEM 1

#include "openVCB_Data.hh"
#include "openVCB_VMem.hh"


#if defined _MSC_VER
#  define OVCB_INLINE constexpr __forceinline
#  if !(defined __GNUC__ || defined __clang__ || defined __INTEL_COMPILER || defined __INTEL_LLVM_COMPILER)
#    pragma warning(disable: 5030)  // Unrecognized attribute
#  endif
#elif defined __GNUC__ || defined __clang__
#  define OVCB_INLINE __attribute__((__always_inline__)) constexpr inline
#else
#  define OVCB_INLINE constexpr inline
#endif


namespace openVCB {
/****************************************************************************************/


struct LatchInterface {
      glm::ivec2 pos;     // Coordinates of the LSB latch.
      glm::ivec2 stride;  // Space between each latch (both x and y).
      glm::ivec2 size;    // Physical ize of the latch in squares as (x,y) pair.
      int        numBits; // Value always 32 or lower
      int        gids[64];
};


/**
 * @brief 8 bit state. 7 bit type + 1 active bit.
 * @note
 * - Add new logic behavior here. All inks define their logic from this.
 * - This is seperate from Ink to simplify switch logic
 */
enum class Logic : uint8_t {
      NonZeroOff = 0x01,
      ZeroOff    = 0x02,
      XorOff     = 0x04,
      XnorOff    = 0x08,
      LatchOff   = 0x10,
      ClockOff   = 0x20,

      numTypes = 6,
      _ink_on = 0x80,

      NonZero = _ink_on | NonZeroOff,
      Zero    = _ink_on | ZeroOff,
      Xor     = _ink_on | XorOff,
      Xnor    = _ink_on | XnorOff,
      Latch   = _ink_on | LatchOff,
      Clock   = _ink_on | ClockOff,
};


/// <summary>
/// 8 bit state.
/// 7 bit type + 1 active bit
///
/// Add potential ink types here.
/// It is VERY important to keep components in pairs for on and off to
/// to make sure the values are aligned correctly.
/// </summary>
enum class Ink : uint8_t {
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
      _ink_on = 0x80,

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


/*--------------------------------------------------------------------------------------*/


/*
 * Hideous boilerplate garbage to make using `enum class` objects less infuriatingly
 * tedious and "cast-tastic". This nonsense takes up less space on the page with the
 * long lines than if properly formatted, which is a win in my book.
 *
 * Incidentally, resharper whines ever so much about things not being "const",
 * so I shut it up.
 */
#define uintc  unsigned const
#define Logicc Logic const
#define Inkc   Ink const

ND OVCB_INLINE Logic operator>>(Logicc val,  uintc  n)    { return static_cast<Logic>(static_cast<unsigned>(val) >> n); }
ND OVCB_INLINE Logic operator<<(Logicc val,  uintc  n)    { return static_cast<Logic>(static_cast<unsigned>(val) << n); }
ND OVCB_INLINE Logic operator& (Logicc val1, uintc  val2) { return static_cast<Logic>(static_cast<unsigned>(val1) & val2); }
ND OVCB_INLINE Logic operator| (Logicc val1, uintc  val2) { return static_cast<Logic>(static_cast<unsigned>(val1) | val2); }
ND OVCB_INLINE Logic operator& (Logicc val1, Logicc val2) { return static_cast<Logic>(static_cast<unsigned>(val1) & static_cast<unsigned>(val2)); }
ND OVCB_INLINE Logic operator| (Logicc val1, Logicc val2) { return static_cast<Logic>(static_cast<unsigned>(val1) | static_cast<unsigned>(val2)); }

template <typename T> requires std::is_integral_v<T>
ND OVCB_INLINE bool operator==(Logic const op1, T const op2)
{
      return op1 == static_cast<Logic>(op2);
}


ND OVCB_INLINE Ink operator>>(Inkc val,  uintc n)    { return static_cast<Ink>(static_cast<unsigned>(val) >> n); }
ND OVCB_INLINE Ink operator<<(Inkc val,  uintc n)    { return static_cast<Ink>(static_cast<unsigned>(val) << n); }
ND OVCB_INLINE Ink operator& (Inkc val1, uintc val2) { return static_cast<Ink>(static_cast<unsigned>(val1) & val2); }
ND OVCB_INLINE Ink operator| (Inkc val1, uintc val2) { return static_cast<Ink>(static_cast<unsigned>(val1) | val2); }
ND OVCB_INLINE Ink operator& (Inkc val1, Inkc  val2) { return static_cast<Ink>(static_cast<unsigned>(val1) & static_cast<unsigned>(val2)); }
ND OVCB_INLINE Ink operator| (Inkc val1, Inkc  val2) { return static_cast<Ink>(static_cast<unsigned>(val1) | static_cast<unsigned>(val2)); }

template <typename T> requires std::is_integral_v<T>
ND OVCB_INLINE bool operator==(Ink const op1, T const op2)
{
      return op1 == static_cast<Ink>(op2);
}

#undef uintc
#undef Logicc
#undef Inkc


/*--------------------------------------------------------------------------------------*/
/* Helper routines for Logic objects. */

/**
 * \brief Sets the ink (logic) type to be as specified by state.
 * \param logic The Ink (logic) value to modify.
 * \param state Should be 0 to turn off, 1 to turn on.
 * \return The modified value.
 */
OVCB_INLINE Logic setOn(Logic const logic, unsigned const state)
{
      return (logic & 0x7F) | (state << 7);
}

// Sets the ink type to be on
OVCB_INLINE Logic setOn(Logic const logic)
{
      return logic | Logic::_ink_on;
}

// Sets the ink type to be off
OVCB_INLINE Logic setOff(Logic const logic)
{
      return logic & 0x7F;
}

// Gets the ink active state
ND OVCB_INLINE bool getOn(Logic const logic)
{
      return static_cast<bool>(logic >> 7);
}


/*--------------------------------------------------------------------------------------*/
/* Helper routines for Ink objects. */

/**
 * \brief Sets the ink type to be as specified by state.
 * \param ink The Ink value to modify.
 * \param state Should be 0 to turn off, 1 to turn on.
 * \return The modified value.
 */
OVCB_INLINE Ink setOn(Ink const ink, unsigned const state)
{
      return (ink & 0x7F) | (state << 7);
}

// Sets the ink type to be on.
OVCB_INLINE Ink setOn(Ink const ink)
{
      return ink | Ink::_ink_on;
}
// Sets the ink type to be off
OVCB_INLINE Ink setOff(Ink const ink)
{
      return ink & 0x7F;
}
// Gets the ink active state
ND OVCB_INLINE bool getOn(Ink const ink)
{
      return static_cast<bool>(ink >> 7);
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
#ifdef OVCB_MT
      std::atomic<int16_t> activeInputs; // Number of active inputs
      std::atomic<uint8_t> visited;      // Flags for traversal
#else
      int16_t activeInputs; // Number of active inputs
      bool visited;         // Flags for traversal
#endif
      Logic logic; // Current logic state
};

/* Paranoia. */
static_assert(sizeof(InkState) == 4 && offsetof(InkState, logic) == 3);


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

static_assert(sizeof(InkPixel) == 4 && offsetof(InkPixel, meta) == 2);


// This is for asyncronous exchange of data
// for stuff like audio and signal scopes
struct InstrumentBuffer {
      InkState *buffer;
      int32_t   bufferSize;
      int32_t   idx;
};

struct SimulationResult {
      int64_t numEventsProcessed;
      int32_t numTicksProcessed;
      bool    breakpoint;
};


/*--------------------------------------------------------------------------------------*/


/**
 * \brief Main data structure for a compiled simulation.
 */
class Project
{
    public:
      // This remains null if VMem is not actually used.
      VMemWrapper vmem = nullptr;

      size_t         vmemSize = 0;
      std::string    assembly;
      LatchInterface vmAddr{};
      LatchInterface vmData{};
      uint32_t       lastVMemAddr = 0;

      int32_t height = 0;
      int32_t width  = 0;

      // An image containing component indices
      uint8_t  *originalImage = nullptr;
      InkPixel *image         = nullptr;
      int32_t  *indexImage    = nullptr;
      int32_t  *decoration[3] = {nullptr, nullptr, nullptr}; // on / off / unknown

      uint32_t ledPalette[16] = {
            0x323841, 0xFFFFFF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00, 0x0000FF,
            0xFF0000, 0x00FF00
      };

      int32_t numGroups = 0;

	std::vector<int32_t> clockGIDs;
      uint64_t             clockCounter = 0;
      uint64_t             clockPeriod  = 2;

      // Adjacentcy matrix
      // By default, the indices from ink groups first and then component groups
      SparseMat writeMap = {};

      // Stores the logic states of each group
      InkState *states = nullptr;

      // Stores the actual ink type of each group
      Ink *stateInks = nullptr;

      // Map of symbols during assembleVmem()
      std::map<std::string, int64_t>     assemblySymbols;
      std::map<int64_t, int64_t>         lineNumbers;
      std::unordered_map<int32_t, Logic> breakpoints;
      std::vector<InstrumentBuffer>      instrumentBuffers;

      uint64_t tickNum = 0;

      // Event queue
      int32_t *updateQ[2]{nullptr, nullptr};
      int16_t *lastActiveInputs = nullptr;

#ifdef OVCB_MT
      std::atomic<int32_t> qSize;
#else
      int32_t qSize = 0;
#endif

      //---------------------------------------------------------------------------------

      Project() = default;
      ~Project();

      // Clang complains unless *all* possible constructors are defined.
      Project(Project const &)                = delete;
      Project &operator=(Project const &)     = delete;
      Project(Project &&) noexcept            = delete;
      Project &operator=(Project &&) noexcept = delete;

      //---------------------------------------------------------------------------------

      // Builds a project from an image. Remember to configure VMem
      void readFromVCB(std::string const &p);

      // Decode base64 data from clipboard, then process logic data
      bool readFromBlueprint(std::string clipboardData);

      // Decompress zstd logic data to an image
      bool processLogicData(std::vector<uint8_t> logicData, int32_t headerSize);

      // Decompress zstd decoration data to an image
      static void processDecorationData(std::vector<uint8_t> decorationData,
                                        int32_t            *&decoData);

      // Samples the ink at a pixel. Returns ink and group id
      ND std::pair<Ink, int32_t> sample(glm::ivec2 pos) const;

      // Assemble the vmem from this->assembly
      // NOTE: Don't call the parameter `err'. That's a standard functinon on most Un*xes.
      void assembleVmem(char *errp = nullptr, size_t errSize = 256);

      // Dump vmem contents to file
      void dumpVMemToText(std::string const &p) const;

      // Toggles the latch at position.
      // Does nothing if it's not a latch
      void toggleLatch(glm::ivec2 pos);

      // Toggles the latch at position.
      // Does nothing if it's not a latch
      void toggleLatch(int32_t gid);

      // Preprocesses the image into the simulation format
      // Note: Gorder is often slower. It is here as an experiment
      void preprocess();

      // Methods to add and remove breakpoints
      void addBreakpoint(int32_t gid);
      void removeBreakpoint(int32_t gid);

      /// Advances the simulation by n ticks
      [[__gnu__::__hot__]]
      SimulationResult tick(int32_t numTicks = 1, int64_t maxEvents = INT64_MAX);

      /// Emits an event if it is not yet in the queue
      constexpr bool tryEmit(int32_t gid)
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
            states[gid].visited = true;
            updateQ[1][qSize++] = gid;
            return true;
#endif
      }
};


/****************************************************************************************/
} // namespace openVCB
#undef OVCB_INLINE
#endif
