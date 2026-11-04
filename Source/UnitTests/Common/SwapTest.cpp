// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <bit>

#include <gtest/gtest.h>

#include "Common/CommonTypes.h"
#include "Common/Swap.h"

namespace
{
// The host's representation of a value, as bytes, for tests that hold on either byte order.
template <typename T>
std::array<u8, sizeof(T)> HostBytes(T value)
{
  return std::bit_cast<std::array<u8, sizeof(T)>>(value);
}
}  // namespace

TEST(Swap, SwapByValue)
{
  EXPECT_EQ(0xf0, Common::swap8(0xf0));
  EXPECT_EQ(0x1234, Common::swap16(0x3412));
  EXPECT_EQ(0x12345678u, Common::swap32(0x78563412));
  EXPECT_EQ(0x123456789abcdef0ull, Common::swap64(0xf0debc9a78563412ull));
}

TEST(Swap, ByteSwapAlwaysReverses)
{
  EXPECT_EQ(0x1234, Common::bswap16(0x3412));
  EXPECT_EQ(0x12345678u, Common::bswap32(0x78563412));
  EXPECT_EQ(0x123456789abcdef0ull, Common::bswap64(0xf0debc9a78563412ull));
  EXPECT_EQ(s16(-2), Common::ByteSwap(s16(0xfeff)));
  EXPECT_EQ(1.0f, Common::ByteSwap(std::bit_cast<float>(0x0000803fu)));
}

TEST(Swap, FixedByteOrderMatchesMemory)
{
  constexpr std::array<u8, 8> bytes = {1, 2, 3, 4, 5, 6, 7, 8};

  EXPECT_EQ(0x0102, Common::FromBigEndian(std::bit_cast<u16>(std::array<u8, 2>{1, 2})));
  EXPECT_EQ(0x0201, Common::FromLittleEndian(std::bit_cast<u16>(std::array<u8, 2>{1, 2})));
  EXPECT_EQ(0x01020304u, Common::ReadBigEndian<u32>(bytes.data()));
  EXPECT_EQ(0x04030201u, Common::ReadLittleEndian<u32>(bytes.data()));
  EXPECT_EQ(0x0102030405060708ull, Common::ReadBigEndian<u64>(bytes.data()));
  EXPECT_EQ(0x0807060504030201ull, Common::ReadLittleEndian<u64>(bytes.data()));
  EXPECT_EQ(0x0102030405060708ull, Common::swap64(bytes.data()));

  EXPECT_EQ(HostBytes(Common::ToBigEndian<u32>(0x01020304)), (std::array<u8, 4>{1, 2, 3, 4}));
  EXPECT_EQ(HostBytes(Common::ToLittleEndian<u32>(0x01020304)), (std::array<u8, 4>{4, 3, 2, 1}));

  std::array<u8, 8> written{};
  Common::WriteBigEndian<u32>(written.data(), 0x01020304);
  Common::WriteLittleEndian<u32>(written.data() + 4, 0x05060708);
  EXPECT_EQ(written, (std::array<u8, 8>{1, 2, 3, 4, 8, 7, 6, 5}));
  Common::WriteSwap16(written.data(), 0x0102);
  EXPECT_EQ(written[0], 1);
  EXPECT_EQ(written[1], 2);
}

TEST(Swap, FixedByteOrderValues)
{
  Common::BigEndianValue<u32> big(0x01020304);
  Common::LittleEndianValue<u32> little(0x01020304);
  EXPECT_EQ(0x01020304u, u32(big));
  EXPECT_EQ(0x01020304u, u32(little));
  EXPECT_EQ(HostBytes(big), (std::array<u8, 4>{1, 2, 3, 4}));
  EXPECT_EQ(HostBytes(little), (std::array<u8, 4>{4, 3, 2, 1}));

  big = 0x05060708;
  little = 0x05060708;
  EXPECT_EQ(HostBytes(big), (std::array<u8, 4>{5, 6, 7, 8}));
  EXPECT_EQ(HostBytes(little), (std::array<u8, 4>{8, 7, 6, 5}));
  EXPECT_EQ("84281096", fmt::format("{}", big));
  EXPECT_EQ("84281096", fmt::format("{}", little));
}
