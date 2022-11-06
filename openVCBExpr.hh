#pragma once
#ifndef FxIvIe7lbTaVdxJuoZVONoHWXEuiTwiC
#define FxIvIe7lbTaVdxJuoZVONoHWXEuiTwiC

/*
 * Instruction expression parsing code.
 */

#include <string>
#include <unordered_map>

namespace openVCB {

long long evalExpr(char const                                 *expr,
                   std::unordered_map<std::string, long long> &symbols,
                   char                                       *err = nullptr);

} // namespace openVCB

#endif