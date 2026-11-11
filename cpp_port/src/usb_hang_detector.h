#pragma once

#include <cstdint>

namespace mock_arturia_controller {

// Tells when the USB device controller has hung (RP2040 erratum E15, see main.cpp): a configured device keeps receiving a
// start-of-frame packet every millisecond until the host suspends the bus, which the controller reports. A hung controller
// stops decoding them without reporting a suspend, so its frame number stands still.
class UsbHangDetector {
 public:
  // Long enough for a bus reset (10 to 20 ms of SE0, no frames) to unconfigure the device first
  static constexpr std::uint32_t kStalledFramesMs = 100;

  // True once the frame number has not changed for kStalledFramesMs while the device was configured and the bus not
  // suspended.
  bool hung(std::uint32_t now_ms, std::uint32_t frame, bool configured, bool suspended) {
    if (!configured || suspended || !watching_ || frame != last_frame_) {
      watching_ = configured && !suspended;
      last_frame_ = frame;
      since_ms_ = now_ms;
      return false;
    }
    return now_ms - since_ms_ >= kStalledFramesMs;
  }

 private:
  bool watching_ = false;
  std::uint32_t last_frame_ = 0;
  std::uint32_t since_ms_ = 0;
};

}  // namespace mock_arturia_controller
