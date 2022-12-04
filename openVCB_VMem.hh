#pragma once
#ifndef G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds
#define G1Gjjm08JMgUZALRqEUy6y5CngZGIr6L5gKrj321mPkr5Jds

#include "openVCB.hh"

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
      ND auto       &def()       & { return b; }
      ND auto const &def() const & { return b; }
#else
      using value_type = uint32_t;
      ND auto       &def()       & { return i; }
      ND auto const &def() const & { return i; }
#endif

      VMemWrapper() = default;
      VMemWrapper(uint32_t *ptr) : i(ptr) {}
      VMemWrapper(uint8_t *ptr)  : b(ptr) {}
      VMemWrapper(nullptr_t)     : i(nullptr) {}

      //---------------------------------------------------------------------------------

      // Assign pointers to the default union member.
      VMemWrapper &operator=(void *ptr)
      {
            def() = static_cast<value_type *>(ptr);
            return *this;
      }

      // Assign pointers to the default union member.
      VMemWrapper &operator=(value_type *ptr)
      {
            def() = ptr;
            return *this;
      }

      // Allow assigning nullptr directly.
      VMemWrapper &operator=(std::nullptr_t)
      {
            def() = nullptr;
            return *this;
      }

      // Allow comparing with nullptr directly.
      ND bool operator==(std::nullptr_t) const
      {
            return def() != nullptr;
      }

      // Allow checking whether the pointer null by placing it in a boolean context
      // just as if this were a bare pointer.
      ND explicit operator bool() const
      {
            return def() != nullptr;
      }

      // Automatically use the default member when indexing.
      ND auto const &operator[](size_t const idx) const &
      {
            return def()[idx];
      }

      // Automatically use the default member when indexing.
      ND auto &operator[](size_t const idx) &
      {
            return def()[idx];
      }

      ND uint32_t *word_at_byte(size_t const offset) const
      {
            return reinterpret_cast<uint32_t *>(b + offset);
      }
};


} // namespace openVCB
#endif
