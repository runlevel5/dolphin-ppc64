// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bit>

#include "Common/CommonTypes.h"
#include "Common/Swap.h"

struct DXTBlock
{
  u16 color1;
  u16 color2;
  u8 lines[4];
};

// A pixel packed so that its bytes in memory are R, G, B, A, the order the backends' texture and
// vertex formats consume. As an integer, the value depends on the host's byte order, so take it
// apart with UnpackRGBA rather than with shifts.
constexpr u32 MakeRGBA(int r, int g, int b, int a)
{
  if constexpr (Common::kHostBigEndian)
    return (static_cast<u32>(r) << 24) | (g << 16) | (b << 8) | a;
  else
    return (static_cast<u32>(a) << 24) | (b << 16) | (g << 8) | r;
}

// The R, G, B and A of a pixel packed by MakeRGBA.
constexpr std::array<u8, 4> UnpackRGBA(u32 rgba)
{
  return std::bit_cast<std::array<u8, 4>>(rgba);
}

constexpr u32 RGBA_OPAQUE_ALPHA = MakeRGBA(0, 0, 0, 0xFF);

constexpr int DXTBlend(int v1, int v2)
{
  // 3/8 blend, which is close to 1/3
  return ((v1 * 3 + v2 * 5) >> 3);
}
