#pragma once
#ifndef Sh0BhWjNiz6BDPKRGtzGLQfTq3U0OXv1
#define Sh0BhWjNiz6BDPKRGtzGLQfTq3U0OXv1

#include <cinttypes>
#include <cstdint>
#include <string_view>

namespace openVCB {
// clang-format off


constexpr uint32_t colorPallet[] = {
	// OFF
	0x000000,

	0x5A5845,
	0x2E475D,
	0x4D383E,
	0x66788E,

	0x3E4D3E,
	0x384B56,
	0x4D392F,

	0x4D3744,
	0x1A3C56,
	0x4D453E,

	0x433D56,
	0x3B2854,

	0x4D243C,
	0x384D47,
	0x323841,
	0x6C9799,

	0xA1AB8C,
	0x3F4B5B,

	// ON
	0x000000,

	0xA19856,
	0x63B1FF,
	0xFF5E5E,
	0x66788E,

	0x92FF63,
	0x63F2FF,
	0xFFA200,

	0xFF628A,
	0x30D9FF,
	0xFFC663,

	0xAE74FF,
	0xA600FF,

	0xFF0041,
	0x63FF9F,
	0xFFFFFF,
	0xB2FBFE,

	0xA1AB8C,
	0x3F4B5B
};

constexpr std::string_view inkNames[] = {
	"None",

	"Trace (Off)",
	"Read (Off)",
	"Write (Off)",
	"Cross",

	"Buffer (Off)",
	"OR (Off)",
	"NAND (Off)",

	"NOT (Off)",
	"NOR (Off)",
	"AND (Off)",

	"XOR (Off)",
	"NXOR (Off)",

	"Clock (Off)",
	"Latch (Off)",
	"LED (Off)",
	"Bundle (Off)",

	"Filler",
	"Annotation",

	"UNDEFINED",

	"Trace (On)",
	"Read (On)",
	"Write (On)",
	"UNDEFINED",

	"Buffer (On)",
	"OR (On)",
	"NAND (On)",

	"NOT (On)",
	"NOR (On)",
	"AND (On)",

	"XOR (On)",
	"NXOR (On)",

	"Clock (On)",
	"Latch (On)",
	"LED (On)",
	"Bundle (On)",

	"UNDEFINED",
	"UNDEFINED"
};

constexpr uint32_t traceColors[] = {
	0x2A3541,
	0x9FA8AE,
	0xA1555E,
	0xA16C56,
	0xA18556,
	0xA19856,
	0x99A156,
	0x88A156,
	0x6CA156,
	0x56A18D,
	0x5693A1,
	0x567BA1,
	0x5662A1,
	0x6656A1,
	0x8756A1,
	0xA15597
};


// clang-format on

} // namespace openVCB
#endif
