/*
 * This is the primary header file for openVCB.
 */
#pragma once
#ifndef C8kWpReCttGxHsWkLLl1RDjAweb3HDua
#define C8kWpReCttGxHsWkLLl1RDjAweb3HDua

// Toggle experimental byte addressed VMem.
#define OVCB_BYTE_ORIENTED_VMEM

#include "openVCB_Utils.hh"
#include "openVCB_Data.hh"

#if defined _MSC_VER
#  define OVCB_CONSTEXPR constexpr __forceinline
#  define OVCB_INLINE    __forceinline
#  if !(defined __GNUC__ || defined __clang__ || defined __INTEL_COMPILER || defined __INTEL_LLVM_COMPILER)
#    pragma warning(disable: 5030)  // Unrecognized attribute
#  endif
#elif defined __GNUC__ || defined __clang__
#  define OVCB_CONSTEXPR __attribute__((__always_inline__)) constexpr inline
#  define OVCB_INLINE    __attribute__((__always_inline__)) inline
#else
#  define OVCB_CONSTEXPR constexpr inline
#  define OVCB_INLINE    inline
#endif


namespace openVCB {
/****************************************************************************************/


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

      TraceOff,
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
      BusOff,

      Filler,
      Annotation,

      TunnelOff,
      Mesh,
      TimerOff,
      RandomOff,
      BreakpointOff,
      Wireless1Off,
      Wireless2Off,
      Wireless3Off,
      Wireless4Off,

      numTypes,
      _ink_on = 0x80,

      Trace        = TraceOff  | _ink_on,
      Read         = ReadOff   | _ink_on,
      Write        = WriteOff  | _ink_on,
      InvalidCross = Cross     | _ink_on,
      Buffer       = BufferOff | _ink_on,
      Or           = OrOff     | _ink_on,
      Nand         = NandOff   | _ink_on,
      Not          = NotOff    | _ink_on,
      Nor          = NorOff    | _ink_on,
      And          = AndOff    | _ink_on,
      Xor          = XorOff    | _ink_on,
      Xnor         = XnorOff   | _ink_on,
      Clock        = ClockOff  | _ink_on,
      Latch        = LatchOff  | _ink_on,
      Led          = LedOff    | _ink_on,
      Bus          = BusOff    | _ink_on,

      InvalidFiller     = Filler     | _ink_on,
      InvalidAnnotation = Annotation | _ink_on,

      Tunnel      = TunnelOff     | _ink_on,
      InvalidMesh = Mesh          | _ink_on,
      Timer       = TimerOff      | _ink_on,
      Random      = RandomOff     | _ink_on,
      Breakpoint  = BreakpointOff | _ink_on,
      Wireless1   = Wireless1Off  | _ink_on,
      Wireless2   = Wireless2Off  | _ink_on,
      Wireless3   = Wireless3Off  | _ink_on,
      Wireless4   = Wireless4Off  | _ink_on,
};


/**
 * @brief 8 bit state. 7 bit type + 1 active bit.
 * @note
 * - Add new logic behavior here. All inks define their logic from this.
 * - This is seperate from Ink to simplify switch logic
 */
enum class Logic : uint8_t {
      None = 0,

      NonZeroOff,
      ZeroOff,
      XorOff,
      XnorOff,
      LatchOff,
      ClockOff,
      TimerOff,
      RandomOff,
      BreakpointOff,

      _numTypes,
      _ink_on = 0x80,

      NonZero    = NonZeroOff    | _ink_on,
      Zero       = ZeroOff       | _ink_on,
      Xor        = XorOff        | _ink_on,
      Xnor       = XnorOff       | _ink_on,
      Latch      = LatchOff      | _ink_on,
      Clock      = ClockOff      | _ink_on,
      Timer      = TimerOff      | _ink_on,
      Random     = RandomOff     | _ink_on,
      Breakpoint = BreakpointOff | _ink_on,
};


/*--------------------------------------------------------------------------------------*/


struct InkState {
      int16_t activeInputs; // Number of active inputs
      bool    visited;      // Flags for traversal
      Logic   logic;        // Current logic state
};

struct SparseMat {
      int  n;    // Size of the matrix
      int  nnz;  // Number of non-zero entries
      int *ptr;  // CSC sparse matrix ptr
      int *rows; // CSC sparse matrix rows
};

// This represents a pixel with meta data as well as simulation ink type
// Ths is for inks that have variants which do not affect simulation
// i.e. Colored traces.
struct InkPixel {
      Ink     ink;
      int16_t meta;
};

// This is for asyncronous exchange of data.
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

struct LatchInterface {
      glm::ivec2 pos;     // Coordinates of the LSB latch.
      glm::ivec2 stride;  // Space between each latch (both x and y).
      glm::ivec2 size;    // Physical ize of the latch in squares as (x,y) pair.
      int        numBits; // Value always 32 or lower
      int        gids[64];
};

/*--------------------------------------------------------------------------------------*/
} // namespace openVCB

#include "openVCB_Helpers.hh"

namespace openVCB {
/*--------------------------------------------------------------------------------------*/


/**
 * \brief Main data structure for a compiled simulation.
 */
class Project
{
    public:
      // This remains null if VMem is not actually used.
      VMemWrapper vmem         = nullptr;
      uint32_t    vmemSize     = 0;
      uint32_t    lastVMemAddr = 0;

      LatchInterface vmAddr    = {{}, {}, {}, -1, {}};
      LatchInterface vmData    = {{}, {}, {}, -1, {}};
      std::string    assembly  = {};
      int32_t        height    = 0;
      int32_t        width     = 0;
      int32_t        numGroups = 0;

      // An image containing component indices.
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

      ClockCounter tickClock;
      TimerCounter realtimeClock;

      // Adjacentcy matrix.
      // By default, the indices from ink groups first and then component groups.
      SparseMat writeMap = {0, 0, nullptr, nullptr};

      // Stores the logic states of each group.
      InkState *states = nullptr;

      // Stores the actual ink type of each group.
      Ink *stateInks = nullptr;

      // Map of symbols during assembleVmem().
      std::map<std::string, uint32_t> assemblySymbols;
      std::map<uint32_t, uint32_t>    lineNumbers;
      std::vector<InstrumentBuffer>   instrumentBuffers;

      uint64_t tickNum = 0;

      // Event queue
      int32_t   *updateQ[2]       = {nullptr, nullptr};
      int16_t   *lastActiveInputs = nullptr;
      uint32_t   qSize            = 0;
      bool       states_is_native = false;
      bool const vmemIsBytes;

      StringArray      *error_messages  = nullptr;
      RandomBitProvider random;

      //---------------------------------------------------------------------------------

      explicit Project(int64_t seed, bool vmemIsBytes);
      ~Project();

      // Clang complains unless *all* possible constructors are defined.
      Project(Project const &)                = delete;
      Project &operator=(Project const &)     = delete;
      Project(Project &&) noexcept            = delete;
      Project &operator=(Project &&) noexcept = delete;

      //---------------------------------------------------------------------------------

      // Builds a project from an image. Remember to configure VMem.
      void readFromVCB(std::string const &filePath);

      // Decode base64 data from clipboard, then process logic data.
      bool readFromBlueprint(std::string clipboardData);

      // Decompress zstd logic data to an image.
      bool processLogicData(std::vector<uint8_t> const &logicData, int32_t headerSize);

      // Decompress zstd decoration data to an image.
      static void processDecorationData(std::vector<uint8_t> const &decorationData,
                                        int32_t                   *&decoData);

      // Samples the ink at a pixel. Returns ink and group id.
      ND std::pair<Ink, int32_t> sample(glm::ivec2 pos) const;

      // Assemble the vmem from this->assembly.
      void assembleVmem(char *errp = nullptr, size_t errSize = 256);

      // Dump vmem contents to file.
      void dumpVMemToText(std::string const &p) const;

      // Toggles the latch at position.
      // Does nothing if it's not a latch.
      void toggleLatch(glm::ivec2 pos);

      // Toggles the latch at position.
      // Does nothing if it's not a latch.
      void toggleLatch(int32_t gid);

      // Preprocesses the image into the simulation format.
      // NOTE: Gorder is often slower. It is here as an experiment.
      [[__gnu__::__hot__]]
      void preprocess();

      // Methods to add and remove breakpoints.
      void addBreakpoint(int32_t gid);
      void removeBreakpoint(int32_t gid);

      /// Advances the simulation by n ticks
      [[__gnu__::__hot__]]
      SimulationResult tick(int32_t numTicks = 1, int64_t maxEvents = INT64_MAX);

      //---------------------------------------------------------------------------------

    private:
      [[__gnu__::__hot__]]
      ND OVCB_INLINE bool resolve_state(SimulationResult &res, InkState curInk, bool lastActive, int lastInputs);

      [[__gnu__::__hot__]]
      OVCB_CONSTEXPR bool tryEmit(int32_t gid);

      [[__gnu__::__hot__]]
      OVCB_CONSTEXPR void handleWordVMemTick();
#ifdef OVCB_BYTE_ORIENTED_VMEM
      OVCB_CONSTEXPR void handleByteVMemTick();
#endif

      OVCB_INLINE bool GetRandomBit() { return static_cast<bool>(random()); }
};


/****************************************************************************************/
} // namespace openVCB
#endif
