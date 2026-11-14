// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>

#include "Common/BitField.h"
#include "Common/Buffer.h"
#include "Common/CommonTypes.h"
#include "Common/WorkQueueThread.h"
#include "Core/HW/EXI/EXI_Device.h"

struct cubeb;
struct cubeb_stream;

namespace ExpansionInterface
{
class CEXIMic : public IEXIDevice
{
public:
  CEXIMic(Core::System& system, const int index);
  ~CEXIMic() override;
  void SetCS(int cs) override;
  bool IsInterruptSet() override;
  bool IsPresent() const override;

private:
  static constexpr std::array<u8, 5> EXI_ID{0, 0x0a, 0, 0, 0};
  static constexpr u32 SAMPLE_SIZE = sizeof(s16);
  static constexpr u32 RATE_BASE = 11025;
  static constexpr u32 RING_BASE = 32;

  enum
  {
    cmdID = 0x00,
    cmdGetStatus = 0x40,
    cmdSetStatus = 0x80,
    cmdGetBuffer = 0x20,
    cmdReset = 0xFF,
  };

  int m_slot;

  u32 m_position = 0;
  int m_command = 0;
  union Status
  {
    u16 hex = 0;
    BitField<0, 4, u16> out;           // MICSet/GetOut...???
    BitField<4, 1, u16> id;            // Used for MICGetDeviceID (always 0)
    BitField<5, 3, u16> button_unk;    // Button bits which appear unused
    BitField<8, 1, u16> button;        // The actual button on the mic
    BitField<9, 1, u16> buff_ovrflw;   // Ring buffer wrote over bytes which weren't read by console
    BitField<10, 1, u16> gain;         // Gain: 0dB or 15dB
    BitField<11, 2, u16> sample_rate;  // Sample rate, 00-11025, 01-22050, 10-44100, 11-??
    BitField<13, 2, u16> buff_size;    // Ring buffer size in bytes, 00-32, 01-64, 10-128, 11-???
    BitField<15, 1, u16> is_active;    // If we are sampling or not
  };

  static long DataCallback(cubeb_stream* stream, void* user_data, const void* input_buffer,
                           void* output_buffer, long nframes);

  void TransferByte(u8& byte) override;

  void StreamInit();
  void StreamTerminate();
  void StreamStart();
  void StreamStop();
  void StreamReadOne();

  // 64 is the max size, can be 16 or 32 as well
  std::size_t m_ring_pos = 0;
  std::array<u8, 64 * SAMPLE_SIZE> m_ring_buffer{};

  // 0 to disable interrupts, else it will be checked against current CPU ticks
  // to determine if interrupt should be raised
  u64 m_next_int_ticks = 0;
  void UpdateNextInterruptTicks();

  // Streaming input interface
  std::shared_ptr<cubeb> m_cubeb_ctx = nullptr;
  cubeb_stream* m_cubeb_stream = nullptr;

  Status m_status{};

  std::mutex m_ring_lock;

  // status bits converted to nice numbers
  u32 m_sample_rate = RATE_BASE;
  u32 m_buff_size = RING_BASE;
  u32 m_buff_size_samples = m_buff_size / SAMPLE_SIZE;

  // Arbitrarily small ringbuffer used by audio input backend in order to
  // keep delay tolerable
  Common::UniqueBuffer<s16> m_stream_buffer;
  u32 m_stream_size;
  u32 m_stream_wpos;
  u32 m_stream_rpos;
  u32 m_samples_avail;

#ifdef _WIN32
  Common::AsyncWorkThread m_work_queue;
  bool m_coinit_success = false;
  bool m_should_couninit = false;
#endif
};
}  // namespace ExpansionInterface
