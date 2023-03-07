#pragma once
#ifndef G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds
#define G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds

#include "openVCB.h"

namespace openVCB {


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
      ND auto       &def()       & noexcept { return i; }
      ND auto const &def() const & noexcept { return i; }
#endif

      //---------------------------------------------------------------------------------

      VMemWrapper() noexcept = default;

      VMemWrapper(uint32_t *ptr) noexcept : i(ptr) {}
      VMemWrapper(uint8_t *ptr)  noexcept : b(ptr) {}
      VMemWrapper(nullptr_t)     noexcept : i(nullptr) {}

      //---------------------------------------------------------------------------------

      // Automatically use the default member when indexing.
      ND auto const &operator[](size_t const idx) const & noexcept { return DEF_POINTER[idx]; }
      ND auto       &operator[](size_t const idx)       & noexcept { return DEF_POINTER[idx]; }

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

static_assert(sizeof(VMemWrapper) == sizeof(void *));

#undef DEF_POINTER

} // namespace openVCB
#endif
