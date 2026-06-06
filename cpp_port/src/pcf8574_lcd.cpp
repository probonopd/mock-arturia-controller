#include "pcf8574_lcd.h"

#include <cstdio>

#include "pico/stdlib.h"

namespace mock_arturia_controller {
namespace {

constexpr std::uint8_t kEnableMask = 0x04;
constexpr std::uint8_t kRegisterSelectMask = 0x01;
constexpr std::uint8_t kBacklightMask = 0x08;

std::uint8_t make_byte(std::uint8_t value, bool is_data, bool backlight_enabled) {
  return static_cast<std::uint8_t>((value & 0xF0) |
                                   (backlight_enabled ? kBacklightMask : 0) |
                                   (is_data ? kRegisterSelectMask : 0));
}

}  // namespace

bool Pcf8574Lcd::init() {
  sleep_ms(50);
  write_nibble(0x03, false);
  sleep_ms(5);
  write_nibble(0x03, false);
  sleep_us(100);
  write_nibble(0x03, false);
  sleep_us(100);
  write_nibble(0x02, false);
  sleep_us(100);

  command(0x28);  // 4-bit, 2-line, 5x8 font
  command(0x08);  // display off
  command(0x01);  // clear
  sleep_ms(2);
  command(0x06);  // entry mode
  command(0x0C);  // display on, cursor off
  return true;
}

void Pcf8574Lcd::clear() {
  command(0x01);
  sleep_ms(2);
}

void Pcf8574Lcd::clear_row(std::uint8_t row) {
  set_cursor(0, row);
  for (std::uint8_t index = 0; index < 16; ++index) {
    data(' ');
  }
}

void Pcf8574Lcd::set_cursor(std::uint8_t col, std::uint8_t row) {
  const std::uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  command(static_cast<std::uint8_t>(0x80 | (col + row_offsets[row])));
}

void Pcf8574Lcd::write(std::string_view text) {
  for (char ch : text) {
    data(static_cast<std::uint8_t>(ch));
  }
}

void Pcf8574Lcd::create_char(std::uint8_t index, const std::array<std::uint8_t, 8>& pattern) {
  command(static_cast<std::uint8_t>(0x40 | ((index & 0x07) << 3)));
  for (std::uint8_t value : pattern) {
    data(value);
  }
  command(0x80);
}

void Pcf8574Lcd::backlight(bool enabled) {
  backlight_enabled_ = enabled;
}

void Pcf8574Lcd::write_nibble(std::uint8_t nibble, bool is_data) {
  const std::uint8_t data_byte = make_byte(static_cast<std::uint8_t>(nibble << 4), is_data, backlight_enabled_);
  i2c_write_blocking(i2c_, address_, &data_byte, 1, false);
  sleep_us(1);
  i2c_write_blocking(i2c_, address_, &data_byte, 1, false);
  sleep_us(1);
  const std::uint8_t pulse = static_cast<std::uint8_t>(data_byte | kEnableMask);
  i2c_write_blocking(i2c_, address_, &pulse, 1, false);
  sleep_us(1);
  i2c_write_blocking(i2c_, address_, &pulse, 1, false);
  sleep_us(1);
  const std::uint8_t release = static_cast<std::uint8_t>(data_byte & ~kEnableMask);
  i2c_write_blocking(i2c_, address_, &release, 1, false);
  sleep_us(50);
}

void Pcf8574Lcd::write_byte(std::uint8_t value, bool is_data) {
  write_nibble(static_cast<std::uint8_t>(value >> 4), is_data);
  write_nibble(static_cast<std::uint8_t>(value & 0x0F), is_data);
}

void Pcf8574Lcd::command(std::uint8_t value) {
  write_byte(value, false);
}

void Pcf8574Lcd::data(std::uint8_t value) {
  write_byte(value, true);
}

}  // namespace mock_arturia_controller
