#pragma once
#ifndef G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds
#define G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds

#include "openVCB.h"

#if defined _MSC_VER
#  define OVCB_CONSTEXPR constexpr __forceinline
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


/*
 * Hideous boilerplate garbage to make using `enum class` objects less infuriatingly
 * tedious and "cast-tastic". This nonsense takes up less space on the page with the
 * long lines than if properly formatted, which is a win in my book.
 *
 * Incidentally, resharper whines ever so much about things not being "const",
 * so I shut it up.
 */
#define intc   int const
#define uintc  uint const
#define Logicc Logic const
#define Inkc   Ink const

template <typename T> concept Integral = std::is_integral<T>::value;

ND OVCB_CONSTEXPR Logic operator>>(Logicc val,  uintc  n)    { return static_cast<Logic>(static_cast<uint>(val) >> n); }
ND OVCB_CONSTEXPR Logic operator<<(Logicc val,  uintc  n)    { return static_cast<Logic>(static_cast<uint>(val) << n); }
ND OVCB_CONSTEXPR Logic operator& (Logicc val1, uintc  val2) { return static_cast<Logic>(static_cast<uint>(val1) & val2); }
ND OVCB_CONSTEXPR Logic operator| (Logicc val1, uintc  val2) { return static_cast<Logic>(static_cast<uint>(val1) | val2); }
ND OVCB_CONSTEXPR Logic operator& (Logicc val1, Logicc val2) { return static_cast<Logic>(static_cast<uint>(val1) & static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR Logic operator| (Logicc val1, Logicc val2) { return static_cast<Logic>(static_cast<uint>(val1) | static_cast<uint>(val2)); }

template <Integral T>
ND OVCB_CONSTEXPR bool operator==(Logic const op1, T const op2)
{
      return op1 == static_cast<Logic>(op2);
}


ND OVCB_CONSTEXPR Ink operator>>(Inkc val,  uintc n)    { return static_cast<Ink>(static_cast<uint>(val) >> n); }
ND OVCB_CONSTEXPR Ink operator<<(Inkc val,  uintc n)    { return static_cast<Ink>(static_cast<uint>(val) << n); }
ND OVCB_CONSTEXPR Ink operator& (Inkc val1, uintc val2) { return static_cast<Ink>(static_cast<uint>(val1) & val2); }
ND OVCB_CONSTEXPR Ink operator| (Inkc val1, uintc val2) { return static_cast<Ink>(static_cast<uint>(val1) | val2); }
ND OVCB_CONSTEXPR Ink operator& (Inkc val1, Inkc  val2) { return static_cast<Ink>(static_cast<uint>(val1) & static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR Ink operator| (Inkc val1, Inkc  val2) { return static_cast<Ink>(static_cast<uint>(val1) | static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR int operator+ (Inkc val1, Inkc  val2) { return static_cast<int>(val1) + static_cast<int>(val2); }
ND OVCB_CONSTEXPR int operator+ (Inkc val1, intc  val2) { return static_cast<int>(val1) + val2; }
ND OVCB_CONSTEXPR int operator- (Inkc val1, Inkc  val2) { return static_cast<int>(val1) - static_cast<int>(val2); }
ND OVCB_CONSTEXPR int operator- (Inkc val1, intc  val2) { return static_cast<int>(val1) - val2; }

template <Integral T>
ND OVCB_CONSTEXPR bool operator==(Ink const op1, T const op2)
{
      return op1 == static_cast<Ink>(op2);
}

ND OVCB_CONSTEXPR Ink   operator|(Inkc val1, Logicc val2) { return static_cast<Ink>(static_cast<uint>(val1) | static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR Ink   operator&(Inkc val1, Logicc val2) { return static_cast<Ink>(static_cast<uint>(val1) & static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR Logic operator|(Logicc val1, Inkc val2) { return static_cast<Logic>(static_cast<uint>(val1) | static_cast<uint>(val2)); }
ND OVCB_CONSTEXPR Logic operator&(Logicc val1, Inkc val2) { return static_cast<Logic>(static_cast<uint>(val1) & static_cast<uint>(val2)); }

#undef intc
#undef uintc
#undef Logicc
#undef Inkc


constexpr bool
operator==(InkPixel const &first, InkPixel const &second) noexcept
{
      return first.ink == second.ink && first.meta == second.meta;
}

/*--------------------------------------------------------------------------------------*/
/* Helper routines for Logic objects. */

/**
 * \brief Sets the ink (logic) type to be as specified by state.
 * \param logic The Ink (logic) value to modify.
 * \param state Should be 0 to turn off, 1 to turn on.
 * \return The modified value.
 */
ND OVCB_CONSTEXPR Logic SetOn(Logic const logic, uint const state)
{
      return (logic & 0x7F) | (state << 7);
}

// Sets the ink type to be on
ND OVCB_CONSTEXPR Logic SetOn(Logic const logic)
{
      return logic | Logic::_ink_on;
}

// Sets the ink type to be off
ND OVCB_CONSTEXPR Logic SetOff(Logic const logic)
{
      return logic & 0x7F;
}

// Gets the ink active state
ND OVCB_CONSTEXPR bool IsOn(Logic const logic)
{
      return static_cast<bool>(logic & 0x80);
}

/*--------------------------------------------------------------------------------------*/
/* Helper routines for Ink objects. */

/**
 * \brief Sets the ink type to be as specified by state.
 * \param ink The Ink value to modify.
 * \param state Should be 0 to turn off, 1 to turn on.
 * \return The modified value.
 */
ND OVCB_CONSTEXPR Ink SetOn(Ink const ink, uint const state)
{
      return (ink & 0x7F) | (state << 7);
}

// Sets the ink type to be on.
ND OVCB_CONSTEXPR Ink SetOn(Ink const ink)
{
      return ink | Ink::_ink_on;
}

// Sets the ink type to be off
ND OVCB_CONSTEXPR Ink SetOff(Ink const ink)
{
      return ink & 0x7F;
}

// Gets the ink active state
ND OVCB_CONSTEXPR bool IsOn(Ink const ink)
{
      return static_cast<bool>(ink & 0x80);
}


// Gets the logic type of said ink
// Define the logics of each ink here.
inline Logic inkLogicType(Ink ink)
{
      ink = SetOff(ink);

      switch (ink) {  // NOLINT(clang-diagnostic-switch-enum)
      case Ink::XorOff:    return Logic::XorOff;
      case Ink::XnorOff:   return Logic::XnorOff;
      case Ink::LatchOff:  return Logic::LatchOff;
      case Ink::ClockOff:  return Logic::ClockOff;
      case Ink::RandomOff: return Logic::RandomOff;
      case Ink::TimerOff:  return Logic::TimerOff;

      case Ink::NotOff:
      case Ink::NorOff:
      case Ink::AndOff:
            return Logic::ZeroOff;

      case Ink::BreakpointOff:
            return Logic::BreakpointOff;

      default:
            return Logic::NonZeroOff;
      }
}

// Gets the string name of the ink
extern char const *getInkString(Ink ink);

/*--------------------------------------------------------------------------------------*/

/**
 * \brief Stores the vmem array. This union makes the byte-addressed variant of openVCB
 * simpler to implement and should have no runtime impact at all.
 * @note This level of obsessive nonsense is excessive even for me.
 */
union VMemWrapper
{
      uint32_t *i;
      uint8_t  *b;

#ifdef OVCB_BYTE_ORIENTED_VMEM
      using value_type = uint8_t;
      ND auto       &def()       & noexcept { return b; }
      ND auto const &def() const & noexcept { return b; }
# define DEF_POINTER b
#else
# define DEF_POINTER i
      using value_type = uint32_t;
      ND auto &      def() & noexcept { return i; }
      ND auto const &def() const & noexcept { return i; }
#endif

      //---------------------------------------------------------------------------------

      VMemWrapper() noexcept = default;
      explicit VMemWrapper(uint32_t *ptr) noexcept
          : i(ptr)
      {}
      explicit VMemWrapper(uint8_t *ptr) noexcept
          : b(ptr)
      {}
      VMemWrapper(nullptr_t) noexcept
          : i(nullptr)
      {}

      //---------------------------------------------------------------------------------

      // Automatically use the default member when indexing.
      ND auto const &operator[](size_t const idx) const & noexcept
      {
            return DEF_POINTER[idx];
      }

      ND auto &operator[](size_t const idx) & noexcept { return DEF_POINTER[idx]; }

      // Assign pointers to the default union member.
      VMemWrapper &operator=(void *ptr) noexcept
      {
            DEF_POINTER = static_cast<value_type *>(ptr);
            return *this;
      }

      // Assign pointers to the default union member.
      VMemWrapper &operator=(value_type *ptr) noexcept
      {
            DEF_POINTER = ptr;
            return *this;
      }

      // Allow assigning nullptr directly.
      VMemWrapper &operator=(std::nullptr_t) noexcept
      {
            DEF_POINTER = nullptr;
            return *this;
      }

      // Allow comparing with nullptr directly.
      ND bool constexpr operator==(std::nullptr_t) const noexcept
      {
            return DEF_POINTER == nullptr;
      }

      // Allow checking whether the pointer null by placing it in a boolean
      // context just as if this were a bare pointer.
      ND explicit constexpr operator bool() const noexcept
      {
            return DEF_POINTER != nullptr;
      }

      ND uint32_t *word_at_byte(size_t const offset) const noexcept
      {
            return reinterpret_cast<uint32_t *>(b + offset);
      }
};

#undef DEF_POINTER

/*--------------------------------------------------------------------------------------*/

class StringArray
{
      char   **array_;
      uint32_t capacity_;
      uint32_t qty_ = 0;

      static constexpr size_t default_capacity = 32;

    public:
      explicit StringArray(uint32_t const capacity = default_capacity)
          : array_(new char *[capacity]),
            capacity_(capacity)
      {}

      ~StringArray()
      {
            if (array_) {
                  for (unsigned i = 0; i < qty_; ++i) {
                        delete[] array_[i];
                        array_[i] = nullptr;
                  }
                  delete[] array_;
                  capacity_ = qty_ = 0;
                  array_ = nullptr;
            }
      }

      StringArray(StringArray const &)                = delete;
      StringArray &operator=(StringArray const &)     = delete;
      StringArray(StringArray &&) noexcept            = delete;
      StringArray &operator=(StringArray &&) noexcept = delete;

      ND char *push_blank(size_t const len)
      {
            if (qty_ + 1 == capacity_) {
                  capacity_ += default_capacity;
                  auto **tmp = new char *[capacity_];
                  memmove(tmp, array_, qty_ * sizeof(char *));
                  delete[] array_;
                  array_ = tmp;
            }

            auto *str = new char[len + 1];
            array_[qty_++] = str;
            return str;
      }

      void push(char const *orig, size_t const len)
      {
            char *str = push_blank(len);
            memcpy(str, orig, len + 1);
      }

      void push(char const *orig)             { push(orig,        strlen(orig)); }
      void push(std::string const &orig)      { push(orig.data(), orig.size());  }
      void push(std::string_view const &orig) { push(orig.data(), orig.size());  }

      template <size_t N>
      void push(char const (&str)[N])
      {
            push(str, N);
      }

      ND bool     empty()    const { return qty_ == 0; }
      ND uint32_t size()     const { return qty_; }
      ND uint32_t capacity() const { return capacity_; }

      ND char const *const *data() const
      {
            return array_;
      }
};

/*--------------------------------------------------------------------------------------*/

class RandomBitProvider
{
      using random_type = std::mt19937;

    public:
      RandomBitProvider()
          : random_engine_(rnd_device_()), current_(random_engine_())
      {}

      explicit RandomBitProvider(uint32_t const seed)
          : random_engine_(seed), current_(random_engine_())
      {}

      ND NOINLINE unsigned operator()()
      {
            if (avail_ > 0) [[likely]] {
                  --avail_;
            } else {
                  avail_   = num_bits - 1U;
                  current_ = random_engine_();
            }

            unsigned const ret = current_ & 1U;
            current_ >>= 1;
            return ret;
      }

    private:
      static constexpr int num_bits = 32U;
      inline static std::random_device rnd_device_{};

      random_type random_engine_;
      uint32_t    current_;
      int         avail_ = num_bits;
};

/*--------------------------------------------------------------------------------------*/


class ClockCounter
{
    public:
      ClockCounter() = default;
      explicit ClockCounter(uint16_t const low, uint16_t const high)
          : current_(low), low_period_(low), high_period_(high)
      {}

      bool tick()
      {
            if (++counter_ >= current_) {
                  state_   = !state_;
                  counter_ = 0;
                  current_ = get_period();
                  return true;
            }
            return false;
      }

      ND bool is_zero() const { return counter_ == 0; }
      ND int  counter() const { return counter_; }

      void set_period(uint const high, uint const low)
      {
            high_period_ = static_cast<uint16_t>(high);
            low_period_  = static_cast<uint16_t>(low);
      }

      std::vector<int32_t> GIDs;

    private:
      alignas(int)
      uint16_t  current_     = 1;
      uint16_t  counter_     = 0;
      uint16_t  low_period_  = 1;
      uint16_t  high_period_ = 1;
      bool      state_       = false;

      ND uint16_t get_period() const { return state_ ? high_period_ : low_period_; }
};


class TimerCounter
{
    public:
      explicit TimerCounter(uint32_t const period = 1)
          : period_(period)
      {}

      bool tick()
      {
            if (++counter_ >= period_) [[unlikely]] {
                  counter_ = 0;
                  return true;
            }
            return false;
      }

      ND bool     is_zero() const { return counter_ == 0; }
      ND uint32_t counter() const { return counter_; }

      void set_period(uint32_t const val) { period_ = val; }

      std::vector<int32_t> GIDs;

    private:
      uint32_t period_;
      uint32_t counter_ = 0;
};


/****************************************************************************************/
} // namespace openVCB
#endif
