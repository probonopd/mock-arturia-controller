#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mock_arturia_controller {

// Collects the system exclusive messages out of the MIDI byte stream the host sends. The USB MIDI receive stream gives bytes,
// not packets: a read may return fewer bytes than asked for (a message that ends at a USB packet boundary) and a message may
// start anywhere in a read, so the bytes are taken one at a time, only those that arrived. Taking every read as 4 bytes put
// zero bytes into the display text ("Volume" showed as "Vo").
class SysexAssembler {
 public:
  // Feeds one byte; true when it completed a message, which data() / size() then hold until the next byte.
  bool feed(std::uint8_t byte) {
    if (byte == 0xF0) {
      length_ = 0;
      in_progress_ = true;
    }
    if (!in_progress_) {
      return false;
    }
    if (length_ >= buffer_.size()) {
      in_progress_ = false; /* too long for the buffer: dropped whole, not shown cut */
      return false;
    }
    buffer_[length_++] = byte;
    if (byte != 0xF7) {
      return false;
    }
    in_progress_ = false;
    return true;
  }

  const std::uint8_t* data() const { return buffer_.data(); }
  std::size_t size() const { return length_; }

 private:
  std::array<std::uint8_t, 256> buffer_{};
  std::size_t length_ = 0;
  bool in_progress_ = false;
};

}  // namespace mock_arturia_controller
