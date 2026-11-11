#include <cstdint>
#include <iostream>

#include "src/usb_hang_detector.h"

namespace {

int failed = 0;

void check(bool condition, const char* what) {
  if (!condition) {
    std::cerr << "failed: " << what << std::endl;
    ++failed;
  }
}

}  // namespace

int main() {
  using mock_arturia_controller::UsbHangDetector;

  {
    UsbHangDetector detector;
    bool hung = false;
    for (std::uint32_t ms = 0; ms < 1000; ++ms) {
      hung = hung || detector.hung(ms, ms & 0x7FF, true, false);
    }
    check(!hung, "frames that advance are no hang");
  }
  {
    UsbHangDetector detector;
    detector.hung(0, 509, true, false);
    check(!detector.hung(99, 509, true, false), "a frame number still for 99 ms is no hang yet");
    check(detector.hung(100, 509, true, false), "a frame number still for 100 ms is a hang");
  }
  {
    UsbHangDetector detector;
    detector.hung(0, 509, true, true);
    check(!detector.hung(5000, 509, true, true), "a suspended bus has no frames and is no hang");
    detector.hung(5001, 509, true, false);
    check(!detector.hung(5100, 510, true, false), "after a resume the frames advance again");
    check(!detector.hung(5199, 510, true, false), "the time counts from the last change");
    check(detector.hung(5200, 510, true, false), "frames that stop after a resume are a hang");
  }
  {
    UsbHangDetector detector;
    detector.hung(0, 0, false, false);
    check(!detector.hung(5000, 0, false, false), "an unconfigured device (before enumeration, in a bus reset) is no hang");
    check(!detector.hung(5001, 0, true, false), "the time counts from the configuration");
    check(detector.hung(5101, 0, true, false), "frames still since the configuration are a hang");
  }
  return failed;
}
