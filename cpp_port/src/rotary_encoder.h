#pragma once

#include <cstdint>
#include <cstdlib>

namespace mock_arturia_controller {

class RotaryEncoder {
 public:
  explicit RotaryEncoder(std::uint8_t transitions_per_step = 2)
      : transitions_per_step_(transitions_per_step) {}

  void reset(std::uint8_t state) {
    last_state_ = state;
    accumulated_steps_ = 0;
  }

  int poll_delta(std::uint8_t state) {
    const std::uint8_t old_a = last_state_ & 0x1u;
    const std::uint8_t old_b = (last_state_ >> 1) & 0x1u;
    const std::uint8_t new_a = state & 0x1u;
    const std::uint8_t new_b = (state >> 1) & 0x1u;
    const std::uint8_t index = (old_a << 3) | (old_b << 2) | (new_a << 1) | new_b;
    const std::int8_t direction = direction_table_[index];

    if (direction != 0) {
      accumulated_steps_ += direction;
    }

    last_state_ = state;

    const std::int16_t threshold = static_cast<std::int16_t>(transitions_per_step_);
    if (accumulated_steps_ >= threshold) {
      accumulated_steps_ = 0;
      return 1;
    }
    if (accumulated_steps_ <= -threshold) {
      accumulated_steps_ = 0;
      return -1;
    }

    return 0;
  }

 private:
  static constexpr std::int8_t direction_table_[] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
     -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  std::uint8_t last_state_ = 0;
  std::int16_t accumulated_steps_ = 0;
  std::uint8_t transitions_per_step_ = 2;
};

}  // namespace mock_arturia_controller
