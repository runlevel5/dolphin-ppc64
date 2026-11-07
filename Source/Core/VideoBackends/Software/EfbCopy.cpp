// Copyright 2009 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoBackends/Software/EfbCopy.h"

#include <algorithm>

#include "Common/CommonTypes.h"
#include "Common/Swap.h"
#include "VideoBackends/Software/SWEfbInterface.h"

#include "VideoCommon/BPMemory.h"
#include "VideoCommon/VideoCommon.h"

namespace EfbCopy
{
void ClearEfb()
{
  // The color's bytes are A, B, G, R: the little-endian bytes of the word R, G, B, A.
  const u32 clear_color_rgba = (bpmem.clearcolorAR & 0xff) << 24 | bpmem.clearcolorGB << 8 |
                               (bpmem.clearcolorAR & 0xff00) >> 8;
  u8 clearColor[4];
  Common::WriteLittleEndian<u32>(clearColor, clear_color_rgba);

  int left = bpmem.copyTexSrcXY.x;
  int top = bpmem.copyTexSrcXY.y;
  int right = std::min(left + bpmem.copyTexSrcWH.x, EFB_WIDTH - 1);
  int bottom = std::min(top + bpmem.copyTexSrcWH.y, EFB_HEIGHT - 1);

  for (u16 y = top; y <= bottom; y++)
  {
    for (u16 x = left; x <= right; x++)
    {
      EfbInterface::SetColor(x, y, clearColor);
      EfbInterface::SetDepth(x, y, bpmem.clearZValue);
    }
  }
}
}  // namespace EfbCopy
