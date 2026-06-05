#include <cstdint>
#include <iostream>

#include "src/rotary_encoder.h"

namespace {

bool Expect(int actual, int expected, const char* label) {
  if (actual != expected) {
    std::cerr << label << " expected " << expected << " but got " << actual << std::endl;
    return false;
  }
  return true;
}

}  // namespace

int main() {
  using mock_arturia_controller::RotaryEncoder;

  RotaryEncoder encoder(2);  // two valid quadrature transitions per emitted step
  encoder.reset(0x0);

  if (!Expect(encoder.poll_delta(0x1), 0, "first valid transition should accumulate")) {
    return 1;
  }

  if (!Expect(encoder.poll_delta(0x3), 1, "second valid transition should emit one step")) {
    return 2;
  }

  encoder.reset(0x0);
  if (!Expect(encoder.poll_delta(0x2), 0, "first opposite transition should accumulate")) {
    return 3;
  }

  if (!Expect(encoder.poll_delta(0x3), -1, "second opposite transition should emit one step")) {
    return 4;
  }

  std::cout << "rotary encoder half-step logic ok" << std::endl;
  return 0;
}
