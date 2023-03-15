#pragma once
#ifndef FxIvIe7lbTaVdxJuoZVONoHWXEuiTwiC
#define FxIvIe7lbTaVdxJuoZVONoHWXEuiTwiC

/*
 * Instruction expression parsing code.
 */

#include <string>
#include <map>

#include <cstdint>

namespace openVCB
{
long long evalExpr(char const *                    expr,
                   std::map<std::string, int64_t> &symbols,
                   char *                          errp    = nullptr,
                   size_t                          errSize = 256);
} // namespace openVCB

#endif
