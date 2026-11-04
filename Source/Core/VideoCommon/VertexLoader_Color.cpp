// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoCommon/VertexLoader_Color.h"

#include <cstring>

#include "Common/CommonTypes.h"
#include "Common/EnumMap.h"
#include "Common/MsgHandler.h"
#include "Common/Swap.h"

#include "VideoCommon/LookUpTables.h"
#include "VideoCommon/TextureDecoder_Util.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexLoaderUtils.h"

namespace
{
void Color_Read_Dummy(VertexLoader* loader)
{
  loader->m_colIndex++;
}

// Colors are written as their R, G, B and A bytes, which Read32 and Read24 take from the vertex
// as they are.
constexpr u32 alpha_mask = RGBA_OPAQUE_ALPHA;

void SetCol(VertexLoader* loader, u32 val)
{
  DataWrite(val);
  loader->m_colIndex++;
}

// Color comes in format RGBA, 4 bits each, as the two bytes of the vertex were read into val_
void SetCol4444(VertexLoader* loader, u16 val_)
{
  const u16 val = Common::FromBigEndian(val_);
  SetCol(loader, MakeRGBA(Convert4To8((val >> 12) & 0xF), Convert4To8((val >> 8) & 0xF),
                          Convert4To8((val >> 4) & 0xF), Convert4To8(val & 0xF)));
}

// Color comes in format RGBA
// RRRRRRGG GGGGBBBB BBAAAAAA
void SetCol6666(VertexLoader* loader, u32 val)
{
  SetCol(loader, MakeRGBA(Convert6To8((val >> 18) & 0x3F), Convert6To8((val >> 12) & 0x3F),
                          Convert6To8((val >> 6) & 0x3F), Convert6To8(val & 0x3F)));
}

// Color comes in RGB
// RRRRRGGG GGGBBBBB
void SetCol565(VertexLoader* loader, u16 val)
{
  SetCol(loader, MakeRGBA(Convert5To8((val >> 11) & 0x1F), Convert6To8((val >> 5) & 0x3F),
                          Convert5To8(val & 0x1F), 0xFF));
}

u32 Read32(const u8* addr)
{
  u32 value;
  std::memcpy(&value, addr, sizeof(u32));
  return value;
}

u32 Read24(const u8* addr)
{
  return Read32(addr) | alpha_mask;
}

template <typename I>
void Color_ReadIndex_16b_565(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* const address =
      VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
      (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);

  u16 value;
  std::memcpy(&value, address, sizeof(u16));

  SetCol565(loader, Common::swap16(value));
}

template <typename I>
void Color_ReadIndex_24b_888(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* address = VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
                      (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);
  SetCol(loader, Read24(address));
}

template <typename I>
void Color_ReadIndex_32b_888x(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* address = VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
                      (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);
  SetCol(loader, Read24(address));
}

template <typename I>
void Color_ReadIndex_16b_4444(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* const address =
      VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
      (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);

  u16 value;
  std::memcpy(&value, address, sizeof(u16));

  SetCol4444(loader, value);
}

template <typename I>
void Color_ReadIndex_24b_6666(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* data = VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
                   (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);
  const u32 val = Common::swap24(data);
  SetCol6666(loader, val);
}

template <typename I>
void Color_ReadIndex_32b_8888(VertexLoader* loader)
{
  const auto index = DataRead<I>();
  const u8* address = VertexLoaderManager::cached_arraybases[CPArray::Color0 + loader->m_colIndex] +
                      (index * g_main_cp_state.array_strides[CPArray::Color0 + loader->m_colIndex]);
  SetCol(loader, Read32(address));
}

void Color_ReadDirect_24b_888(VertexLoader* loader)
{
  SetCol(loader, Read24(DataGetPosition()));
  DataSkip(3);
}

void Color_ReadDirect_32b_888x(VertexLoader* loader)
{
  SetCol(loader, Read24(DataGetPosition()));
  DataSkip(4);
}

void Color_ReadDirect_16b_565(VertexLoader* loader)
{
  SetCol565(loader, DataRead<u16>());
}

void Color_ReadDirect_16b_4444(VertexLoader* loader)
{
  u16 value;
  std::memcpy(&value, DataGetPosition(), sizeof(u16));

  SetCol4444(loader, value);
  DataSkip(2);
}

void Color_ReadDirect_24b_6666(VertexLoader* loader)
{
  SetCol6666(loader, Common::swap24(DataGetPosition()));
  DataSkip(3);
}

void Color_ReadDirect_32b_8888(VertexLoader* loader)
{
  SetCol(loader, DataReadU32Unswapped());
}

using Row = Common::EnumMap<TPipelineFunction, ColorFormat::RGBA8888>;
using Table = Common::EnumMap<Row, VertexComponentFormat::Index16>;

constexpr Table s_table_read_color = {
    Row(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
    Row(Color_ReadDirect_16b_565, Color_ReadDirect_24b_888, Color_ReadDirect_32b_888x,
        Color_ReadDirect_16b_4444, Color_ReadDirect_24b_6666, Color_ReadDirect_32b_8888),
    Row(Color_ReadIndex_16b_565<u8>, Color_ReadIndex_24b_888<u8>, Color_ReadIndex_32b_888x<u8>,
        Color_ReadIndex_16b_4444<u8>, Color_ReadIndex_24b_6666<u8>, Color_ReadIndex_32b_8888<u8>),
    Row(Color_ReadIndex_16b_565<u16>, Color_ReadIndex_24b_888<u16>, Color_ReadIndex_32b_888x<u16>,
        Color_ReadIndex_16b_4444<u16>, Color_ReadIndex_24b_6666<u16>,
        Color_ReadIndex_32b_8888<u16>),
};

}  // Anonymous namespace

TPipelineFunction VertexLoader_Color::GetFunction(VertexComponentFormat type, ColorFormat format)
{
  if (format > ColorFormat::RGBA8888)
  {
    PanicAlertFmt("Invalid color format {}", format);
    return nullptr;
  }
  return s_table_read_color[type][format];
}

TPipelineFunction VertexLoader_Color::GetDummyFunction()
{
  return Color_Read_Dummy;
}
