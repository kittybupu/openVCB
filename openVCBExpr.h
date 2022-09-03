#pragma once
/*
* Instruction expression parsing code.
*/

#include <unordered_map>
#include <string>

namespace openVCB {
	long long evalExpr(const char* expr, std::unordered_map<std::string, long long>& symbols, char* err = nullptr);
}