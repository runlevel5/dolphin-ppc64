// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <random>
#include <span>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "Common/Align.h"
#include "Common/CommonTypes.h"
#include "VideoCommon/TextureDecoder.h"
#include "VideoCommon/TextureDecoder_Util.h"

static constexpr auto texture_sizes = {8, 16, 32, 64, 128, 256};
static constexpr u32 max_texture_size = *(texture_sizes.end() - 1);

static std::array<u8, 2 * sizeof(u32) * max_texture_size * max_texture_size> s_input_memory;
static std::array<u8, sizeof(u32) * max_texture_size * max_texture_size> s_output_memory;

class TextureDecoderTest : public testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    static constexpr DXTBlock cmpr_test_0 = DXTBlock{0xAA20, 0x20AA, {0x1B, 0xE4, 0x4B, 0xB1}};
    static constexpr DXTBlock cmpr_test_1 = DXTBlock{0xBB30, 0x30BB, {0x1B, 0xE4, 0x4B, 0xB1}};

    auto it = s_input_memory.data();

    // Write 32-bit values with all bytes same
    for (u32 i = 0; i < 0x100; ++i)
    {
      *it++ = i;
      *it++ = i;
      *it++ = i;
      *it++ = i;
    }

    // Check that CMPR decoding is selecting the correct interpolation mode
    std::memcpy(it, reinterpret_cast<const u8*>(&cmpr_test_0), sizeof(DXTBlock));
    it += sizeof(DXTBlock);
    std::memcpy(it, reinterpret_cast<const u8*>(&cmpr_test_1), sizeof(DXTBlock));
    it += sizeof(DXTBlock);

    // Write 32-bit values with one non-zero byte
    for (u32 i = 1; i < 0x100; ++i)
    {
      *it++ = i;
      *it++ = 0;
      *it++ = 0;
      *it++ = 0;
    }
    for (u32 i = 1; i < 0x100; ++i)
    {
      *it++ = 0;
      *it++ = i;
      *it++ = 0;
      *it++ = 0;
    }
    for (u32 i = 1; i < 0x100; ++i)
    {
      *it++ = 0;
      *it++ = 0;
      *it++ = i;
      *it++ = 0;
    }
    for (u32 i = 1; i < 0x100; ++i)
    {
      *it++ = 0;
      *it++ = 0;
      *it++ = 0;
      *it++ = i;
    }

    // Write all 16-bit values
    for (u32 i = 0; i < 0x10000; i++)
    {
      *it++ = i >> 8;
      *it++ = i & 0xFF;
    }

    // Write random bytes
    std::default_random_engine engine(0);
    // uniform_int_distribution is guaranteed to work with short, but not with char
    std::uniform_int_distribution<u16> dist;
    while (it != s_input_memory.data() + s_input_memory.size())
      *it++ = static_cast<u8>(dist(engine));
  }

  void SetUp() override { s_output_memory.fill(0); }

  static std::chrono::nanoseconds TestTexture(u32 width, u32 height, TextureFormat texformat,
                                              TLUTFormat tlutfmt)
  {
    // For the TLUT, we use some bytes from input_memory that don't overlap with the texture itself.
    // Exactly which bytes we use isn't so important.
    const u8* tlut = s_input_memory.data() +
                     Common::AlignUp(width, 8) * Common::AlignUp(height, 8) * sizeof(u32);
    const std::span<const u8> tlut_span(tlut, s_input_memory.data() + s_input_memory.size());

    u8* output = s_output_memory.data();
    const auto start_time = std::chrono::steady_clock::now();
    TexDecoder_Decode(output, s_input_memory.data(), width, height, texformat, tlut, tlutfmt);
    const auto end_time = std::chrono::steady_clock::now();

    for (u32 y = 0; y < height; ++y)
    {
      for (u32 x = 0; x < width; ++x)
      {
        u32 expected;
        TexDecoder_DecodeTexel(reinterpret_cast<u8*>(&expected), std::span(s_input_memory), x, y,
                               width - 1, texformat, tlut_span, tlutfmt);
        u32 actual;
        std::memcpy(&actual, output + (y * width + x) * sizeof(u32), sizeof(u32));
        EXPECT_EQ(expected, actual) << "x=" << x << ", y=" << y;
      }
    }

    return end_time - start_time;
  }

  static void TestTextures(TextureFormat texformat, TLUTFormat tlutfmt, std::string_view name)
  {
    std::chrono::nanoseconds cumulative_time{0};
    u32 iterations = 0;
    for (u32 width : texture_sizes)
    {
      for (u32 height : texture_sizes)
      {
        cumulative_time += TestTexture(width, height, texformat, tlutfmt);
        ++iterations;
      }
    }

    fmt::println("{} took {} ns on average ({} iterations)", name,
                 cumulative_time.count() / iterations, iterations);
  }
};

TEST_F(TextureDecoderTest, I4)
{
  TestTextures(TextureFormat::I4, TLUTFormat::IA8, "I4");
}

TEST_F(TextureDecoderTest, I8)
{
  TestTextures(TextureFormat::I8, TLUTFormat::IA8, "I8");
}

TEST_F(TextureDecoderTest, IA4)
{
  TestTextures(TextureFormat::IA4, TLUTFormat::IA8, "IA4");
}

TEST_F(TextureDecoderTest, IA8)
{
  TestTextures(TextureFormat::IA8, TLUTFormat::IA8, "IA8");
}

TEST_F(TextureDecoderTest, RGB565)
{
  TestTextures(TextureFormat::RGB565, TLUTFormat::IA8, "RGB565");
}

TEST_F(TextureDecoderTest, RGB5A3)
{
  TestTextures(TextureFormat::RGB5A3, TLUTFormat::IA8, "RGB5A3");
}

TEST_F(TextureDecoderTest, RGBA8)
{
  TestTextures(TextureFormat::RGBA8, TLUTFormat::IA8, "RGBA8");
}

TEST_F(TextureDecoderTest, C4_IA8)
{
  TestTextures(TextureFormat::C4, TLUTFormat::IA8, "C4 (IA8)");
}

TEST_F(TextureDecoderTest, C4_RGB565)
{
  TestTextures(TextureFormat::C4, TLUTFormat::RGB565, "C4 (RGB565)");
}

TEST_F(TextureDecoderTest, C4_RGB5A3)
{
  TestTextures(TextureFormat::C4, TLUTFormat::RGB5A3, "C4 (RGB5A3)");
}

TEST_F(TextureDecoderTest, C8_IA8)
{
  TestTextures(TextureFormat::C8, TLUTFormat::IA8, "C8 (IA8)");
}

TEST_F(TextureDecoderTest, C8_RGB565)
{
  TestTextures(TextureFormat::C8, TLUTFormat::RGB565, "C8 (RGB565)");
}

TEST_F(TextureDecoderTest, C8_RGB5A3)
{
  TestTextures(TextureFormat::C8, TLUTFormat::RGB5A3, "C8 (RGB5A3)");
}

TEST_F(TextureDecoderTest, C14X2_IA8)
{
  TestTextures(TextureFormat::C14X2, TLUTFormat::IA8, "C14X2 (IA8)");
}

TEST_F(TextureDecoderTest, C14X2_RGB565)
{
  TestTextures(TextureFormat::C14X2, TLUTFormat::RGB565, "C14X2 (RGB565)");
}

TEST_F(TextureDecoderTest, C14X2_RGB5A3)
{
  TestTextures(TextureFormat::C14X2, TLUTFormat::RGB5A3, "C14X2 (RGB5A3)");
}

TEST_F(TextureDecoderTest, CMPR)
{
  TestTextures(TextureFormat::CMPR, TLUTFormat::IA8, "CMPR");
}

namespace
{
// The generic decoder, with each operation rounded on its own, as on hosts without fused
// multiply-add, whatever the compiler does with this file.
u8 Clamp(float value)
{
  return static_cast<u8>(std::clamp(static_cast<int>(value), 0, 255));
}

void ReferenceDecodeXFB(u8* dst, const u8* src, u32 width, u32 height, u32 stride)
{
  for (u32 y = 0; y < height; y++)
  {
    const u8* row = src + y * stride;
    for (u32 x = 0; x < width; x += 2)
    {
      const int y1 = row[0] - 16;
      const int u = row[1] - 128;
      const int y2 = row[2] - 16;
      const int v = row[3] - 128;
      row += 4;
      for (const int luma : {y1, y2})
      {
        volatile float yy = 1.164f * luma;
        volatile float rv = 1.596f * v;
        volatile float gu = 0.392f * u;
        volatile float gv = 0.813f * v;
        volatile float bu = 2.017f * u;
        volatile float g = yy - gu;
        *dst++ = Clamp(yy + rv);
        *dst++ = Clamp(g - gv);
        *dst++ = Clamp(yy + bu);
        *dst++ = 0xff;
      }
    }
  }
}
}  // namespace

TEST(TextureDecoder, DecodeXFBMatchesForAllColors)
{
  // Every Y with every U and V, in rows of 256 color samples, one row for each U.
  constexpr u32 width = 2 * 256 * 256;
  std::vector<u8> src(2 * width);
  for (u32 u = 0; u < 256; u++)
  {
    for (u32 i = 0; i < width / 2; i++)
    {
      src[4 * i + 0] = static_cast<u8>(i);
      src[4 * i + 1] = static_cast<u8>(u);
      src[4 * i + 2] = static_cast<u8>(i * 7 + 3);
      src[4 * i + 3] = static_cast<u8>(i >> 8);
    }
    std::vector<u8> expected(4 * width), actual(4 * width);
    ReferenceDecodeXFB(expected.data(), src.data(), width, 1, 2 * width);
    TexDecoder_DecodeXFB(actual.data(), src.data(), width, 1, 2 * width);
    ASSERT_EQ(expected, actual) << "U " << u;
  }
}

TEST(TextureDecoder, DecodeXFBSizesAndStrides)
{
  std::mt19937 rng(1);
  for (u32 width = 1; width <= 40; width++)
  {
    const u32 height = 3;
    const u32 stride = 2 * width + 2 + 4 * (width % 3);
    std::vector<u8> src(stride * height + 4);
    for (u8& byte : src)
      byte = static_cast<u8>(rng());
    // The decoders write two pixels for each color sample.
    const u32 pixels = (width + 1) / 2 * 2 * height;
    std::vector<u8> expected(4 * pixels + 64, 0x55), actual(4 * pixels + 64, 0x55);
    ReferenceDecodeXFB(expected.data(), src.data(), width, height, stride);
    TexDecoder_DecodeXFB(actual.data(), src.data(), width, height, stride);
    EXPECT_EQ(expected, actual) << "width " << width;
  }
}
