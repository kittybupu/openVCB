#pragma once
#ifndef D7LrV6gurVPUY6pqZZjdoPIPam68V1Xa
#define D7LrV6gurVPUY6pqZZjdoPIPam68V1Xa

#include "Common.hh"

namespace openVCB::util {
/****************************************************************************************/


namespace concepts {

template <typename Ty>
concept Integral = std::is_integral_v<std::remove_cvref_t<Ty>>;

}


template <typename T1>
constexpr bool eq_any(T1 const &left, T1 const &right)
{
      return left == right;
}

template <typename T1, typename ...Types>
constexpr bool eq_any(T1 const &left, T1 const &right, Types const &...rest)
{
      return left == right || eq_any(left, rest...);
}


/****************************************************************************************/
} // namespace openVCB::util
#endif
