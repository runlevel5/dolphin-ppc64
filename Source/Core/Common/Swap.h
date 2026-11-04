// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bit>
#include <cstring>
#include <type_traits>

#include <fmt/format.h>

#include "Common/CommonTypes.h"

#if defined(ANDROID) || defined(__OpenBSD__)
#undef swap16
#undef swap32
#undef swap64
#endif

namespace Common
{
// The emulated consoles are big-endian. On a big-endian host, the conversions between console and
// host byte order below are identities; the bswap* functions always reverse bytes.
constexpr bool kHostBigEndian = std::endian::native == std::endian::big;
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
              "Mixed-endian hosts are not supported");
static_assert(kHostBigEndian == (DOLPHIN_HOST_BIG_ENDIAN != 0),
              "DOLPHIN_HOST_BIG_ENDIAN disagrees with std::endian::native");

// Byte reversal, whatever the host's byte order.
constexpr u8 bswap8(u8 data)
{
  return data;
}
constexpr u16 bswap16(u16 data)
{
#ifdef __cpp_lib_byteswap
  return std::byteswap(data);
#else
  return static_cast<u16>((data >> 8) | (data << 8));
#endif
}
constexpr u32 bswap32(u32 data)
{
#ifdef __cpp_lib_byteswap
  return std::byteswap(data);
#else
  return (static_cast<u32>(bswap16(static_cast<u16>(data))) << 16) |
         bswap16(static_cast<u16>(data >> 16));
#endif
}
constexpr u64 bswap64(u64 data)
{
#ifdef __cpp_lib_byteswap
  return std::byteswap(data);
#else
  return (static_cast<u64>(bswap32(static_cast<u32>(data))) << 32) |
         bswap32(static_cast<u32>(data >> 32));
#endif
}

static_assert(bswap16(0x0102) == 0x0201);
static_assert(bswap32(0x01020304) == 0x04030201);
static_assert(bswap64(0x0102030405060708) == 0x0807060504030201);

// Byte reversal of any arithmetic type, including floating-point types.
template <typename T>
constexpr T ByteSwap(T data)
{
  static_assert(std::is_arithmetic_v<T>, "function only makes sense with arithmetic types");
  if constexpr (sizeof(T) == 1)
    return data;
  else if constexpr (sizeof(T) == 2)
    return std::bit_cast<T>(bswap16(std::bit_cast<u16>(data)));
  else if constexpr (sizeof(T) == 4)
    return std::bit_cast<T>(bswap32(std::bit_cast<u32>(data)));
  else
  {
    static_assert(sizeof(T) == 8, "unsupported size");
    return std::bit_cast<T>(bswap64(std::bit_cast<u64>(data)));
  }
}

// Conversions between the host's byte order and a fixed byte order. Each is its own inverse.
template <typename T>
constexpr T FromBigEndian(T data)
{
  if constexpr (kHostBigEndian)
    return data;
  else
    return ByteSwap(data);
}
template <typename T>
constexpr T ToBigEndian(T data)
{
  return FromBigEndian(data);
}
template <typename T>
constexpr T FromLittleEndian(T data)
{
  if constexpr (kHostBigEndian)
    return ByteSwap(data);
  else
    return data;
}
template <typename T>
constexpr T ToLittleEndian(T data)
{
  return FromLittleEndian(data);
}

static_assert(FromBigEndian(std::bit_cast<u32>(std::array<u8, 4>{1, 2, 3, 4})) == 0x01020304);
static_assert(FromLittleEndian(std::bit_cast<u32>(std::array<u8, 4>{1, 2, 3, 4})) == 0x04030201);

// Reads and writes of values stored in a fixed byte order, at any alignment.
template <typename T>
inline T ReadBigEndian(const u8* data)
{
  T value;
  std::memcpy(&value, data, sizeof(T));
  return FromBigEndian(value);
}
template <typename T>
inline T ReadLittleEndian(const u8* data)
{
  T value;
  std::memcpy(&value, data, sizeof(T));
  return FromLittleEndian(value);
}
template <typename T>
inline void WriteBigEndian(u8* data, T value)
{
  value = ToBigEndian(value);
  std::memcpy(data, &value, sizeof(T));
}
template <typename T>
inline void WriteLittleEndian(u8* data, T value)
{
  value = ToLittleEndian(value);
  std::memcpy(data, &value, sizeof(T));
}

// Historic names. swap8/16/32/64 convert between the console's byte order and the host's, which
// reverses bytes on a little-endian host and does nothing on a big-endian one. Use bswap* to
// always reverse bytes. The overloads on a pointer read a value in the console's byte order.
constexpr u8 swap8(u8 data)
{
  return data;
}
constexpr u16 swap16(u16 data)
{
  return FromBigEndian(data);
}
constexpr u32 swap32(u32 data)
{
  return FromBigEndian(data);
}
constexpr u64 swap64(u64 data)
{
  return FromBigEndian(data);
}

inline u32 swap24(const u8* data)
{
  return (data[0] << 16) | (data[1] << 8) | data[2];
}
inline u16 swap16(const u8* data)
{
  return ReadBigEndian<u16>(data);
}
inline u32 swap32(const u8* data)
{
  return ReadBigEndian<u32>(data);
}
inline u64 swap64(const u8* data)
{
  return ReadBigEndian<u64>(data);
}

inline void WriteSwap16(u8* data, u16 value)
{
  WriteBigEndian(data, value);
}
inline void WriteSwap32(u8* data, u32 value)
{
  WriteBigEndian(data, value);
}
inline void WriteSwap64(u8* data, u64 value)
{
  WriteBigEndian(data, value);
}

// A value stored in a fixed byte order, for structures that mirror files, packets or console
// memory. Reading and assigning convert to and from the host's byte order.
template <typename value_type>
struct BigEndianValue
{
  static_assert(std::is_arithmetic<value_type>(), "value_type must be an arithmetic type");
  BigEndianValue() = default;
  explicit BigEndianValue(value_type val) { *this = val; }
  operator value_type() const { return FromBigEndian(raw); }
  BigEndianValue& operator=(value_type v)
  {
    raw = ToBigEndian(v);
    return *this;
  }

private:
  value_type raw;
};

template <typename value_type>
struct LittleEndianValue
{
  static_assert(std::is_arithmetic<value_type>(), "value_type must be an arithmetic type");
  LittleEndianValue() = default;
  explicit LittleEndianValue(value_type val) { *this = val; }
  operator value_type() const { return FromLittleEndian(raw); }
  LittleEndianValue& operator=(value_type v)
  {
    raw = ToLittleEndian(v);
    return *this;
  }

private:
  value_type raw;
};
}  // Namespace Common

template <typename value_type>
struct fmt::formatter<Common::BigEndianValue<value_type>>
{
  fmt::formatter<value_type> m_formatter;
  constexpr auto parse(format_parse_context& ctx) { return m_formatter.parse(ctx); }
  template <typename FormatContext>
  auto format(const Common::BigEndianValue<value_type>& value, FormatContext& ctx) const
  {
    return m_formatter.format(value.operator value_type(), ctx);
  }
};

template <typename value_type>
struct fmt::formatter<Common::LittleEndianValue<value_type>>
{
  fmt::formatter<value_type> m_formatter;
  constexpr auto parse(format_parse_context& ctx) { return m_formatter.parse(ctx); }
  template <typename FormatContext>
  auto format(const Common::LittleEndianValue<value_type>& value, FormatContext& ctx) const
  {
    return m_formatter.format(value.operator value_type(), ctx);
  }
};
