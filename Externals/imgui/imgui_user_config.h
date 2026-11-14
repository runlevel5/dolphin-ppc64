#pragma once

#include "Common/Assert.h"

#define IM_ASSERT(_EXPR) ASSERT(_EXPR)

#define IMGUI_DISABLE_DEMO_WINDOWS

// ImU32 colors are consumed as R,G,B,A bytes in memory, so on a big-endian host the red channel
// goes in the most significant byte.
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define IM_COL32_R_SHIFT 24
#define IM_COL32_G_SHIFT 16
#define IM_COL32_B_SHIFT 8
#define IM_COL32_A_SHIFT 0
#define IM_COL32_A_MASK 0x000000FF
#endif
