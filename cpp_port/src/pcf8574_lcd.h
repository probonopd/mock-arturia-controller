#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "hardware/i2c.h"

namespace mock_arturia_controller {

class Pcf8574Lcd {
 public:
  Pcf8574Lcd(i2c_inst_t* i2c, std::uint8_t address)
      : i2c_(i2c), address_(address) {}

  bool init();
  void clear();
  void clear_row(std::uint8_t row);
  void set_cursor(std::uint8_t col, std::uint8_t row);
  void write(std::string_view text);
  void create_char(std::uint8_t index, const std::array<std::uint8_t, 8>& pattern);
  void backlight(bool enabled);

 private:
  void write_nibble(std::uint8_t nibble, bool is_data);
  void write_byte(std::uint8_t value, bool is_data);
  void command(std::uint8_t value);
  void data(std::uint8_t value);

  i2c_inst_t* i2c_;
  std::uint8_t address_;
  bool backlight_enabled_ = true;
};

}  // namespace mock_arturia_controller
