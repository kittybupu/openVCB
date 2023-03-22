#pragma once
#ifndef Sh0BhWjNiz6BDPKRGtzGLQfTq3U0OXv1
#define Sh0BhWjNiz6BDPKRGtzGLQfTq3U0OXv1

#include "openVCB_Utils.hh"

namespace openVCB {
/****************************************************************************************/


constexpr uint32_t colorPallet[56] = {
      // Off
      0x000000,
      0x5A5845, // Trace_Off
      0x2E475D, // Read_Off
      0x4D383E, // Write_Off
      0x66788E, // Cross_Off

      0x3E4D3E, // Buffer_Off
      0x384B56, // OR_Off
      0x4D392F, // NAND_Off

      0x4D3744, // NOT_Off
      0x1A3C56, // NOR_Off
      0x4D453E, // AND_Off

      0x433D56, // XOR_Off
      0x3B2854, // XNOR_Off

      0x4D243C, // Clock_Off
      0x384D47, // Latch_Off
      0x323841, // LED_Off
      0x283769, // Bus_Off

      0xA1AB8C, // Filler_Off
      0x3F4B5B, // Annotation_Off

      0x393A52, // Tunnel_Off
      0x646A57, // Mesh_Off
      0x543836, // Timer_Off
      0x4E5537, // Random_Off
      0x542A39, // Breakpoint_Off
      0x562660, // Wireless_1_Off
      0x55265E, // Wireless_2_Off
      0x57265C, // Wireless_3_Off
      0x582758, // Wireless_4_Off

      // On
      0x000000,
      0xA19856, // Trace
      0x63B1FF, // Read
      0xFF5E5E, // Write
      0x66788E, // InvalidCross

      0x92FF63, // Buffer
      0x63F2FF, // OR
      0xFFA200, // NAND

      0xFF628A, // NOT
      0x30D9FF, // NOR
      0xFFC663, // AND

      0xAE74FF, // XOR
      0xA600FF, // XNOR

      0xFF0041, // Clock
      0x63FF9F, // Latch
      0xFFFFFF, // LED

      0x24417A, // Bus

      0xA1AB8C, // Filler (INVALID)
      0x3F4B5B, // Annotation (INVALID)

      0x535572, // Tunnel
      0x646A57, // Mesh
      0xFF6700, // Timer
      0xE5FF00, // Random
      0xE00000, // Breakpoint
      0xFF00BF, // Wireless_1
      0xFF00AF, // Wireless_2
      0xFF009F, // Wireless_3
      0xFF008F, // Wireless_4
};

constexpr std::string_view inkNames[56] = {
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
      "Bus (Off)",

      "Filler",
      "Annotation",

      "Tunnel (Off)",
      "Mesh",
      "Timer (Off)",
      "Random (Off)",
      "Break (Off)",
      "RX1 (Off)",
      "RX2 (Off)",
      "RX3 (Off)",
      "RX4 (Off)",

      "UNDEFINED",
      "Trace (On)",
      "Read (On)",
      "Write (On)",
      "INVALID Cross",
      
      "Buffer (On)",
      "OR (On)",
      "NAND (On)",

      "NOT (On)",
      "NOR (On)",
      "AND (On)",

      "XOR (On)",
      "XNOR (On)",

      "Clock (On)",
      "Latch (On)",
      "LED (On)",
      "Bus (On)",
      
      "INVALID Filler",
      "INVALID Annotation",

      "Tunnel (On)",
      "INVALID Mesh",
      "Timer (On)",
      "Random (On)",
      "Break (On)",
      "RX1 (On)",
      "RX2 (On)",
      "RX3 (On)",
      "RX4 (On)",
};

constexpr uint32_t traceColors[16] = {
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
      0xA15597,
};

constexpr glm::ivec2 fourNeighbors[4] = {
      glm::ivec2(-1, 0), glm::ivec2(0,  1),
      glm::ivec2( 1, 0), glm::ivec2(0, -1)
};


/****************************************************************************************/
} // namespace openVCB
#endif
