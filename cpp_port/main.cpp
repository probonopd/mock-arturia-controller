#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "bsp/board_api.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "src/model_config.h"
#include "src/pcf8574_lcd.h"
#include "src/rotary_encoder.h"
#include "src/sysex_display.h"

namespace {

std::string merge_bottom_row(std::string current, std::string_view incoming) {
  if (current.size() < 16) {
    current.resize(16, ' ');
  }

  for (std::size_t index = 0; index < 16 && index < incoming.size(); ++index) {
    if (incoming[index] != ' ') {
      current[index] = incoming[index];
    }
  }

  return current;
}

enum class ControllerMode : std::uint8_t {
  kArturia,
  kDaw,
  kMcu,
};

constexpr uint kLedPin = PICO_DEFAULT_LED_PIN;
constexpr uint kButtonPins[] = {2, 3, 4, 5, 8};
constexpr uint kEncoderA = 6;
constexpr uint8_t kMidiPortAlv = 0;
constexpr uint kEncoderB = 7;
constexpr uint kEncoderSwitch = 8;
constexpr uint8_t kMidiPortMidi = mock_arturia_controller::MidiOutputPort();
constexpr uint kI2cSda = 0;
constexpr uint kI2cScl = 1;

constexpr std::array<std::uint8_t, 8> kHeartGlyph = {
    0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00,
};

constexpr std::array<std::uint8_t, 8> kArrowUpGlyph = {
    0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00,
};

constexpr std::array<std::uint8_t, 8> kArrowDownGlyph = {
    0x00, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00,
};

class ControllerApp {
 public:
  static constexpr std::size_t kDisplayColumns = 16;

  void init() {
    stdio_init_all();
    gpio_init(kLedPin);
    gpio_set_dir(kLedPin, GPIO_OUT);
    gpio_put(kLedPin, false);

    for (uint pin : kButtonPins) {
      gpio_init(pin);
      gpio_set_dir(pin, GPIO_IN);
      gpio_set_pulls(pin, true, false);
    }

    gpio_init(kEncoderA);
    gpio_set_dir(kEncoderA, GPIO_IN);
    gpio_set_pulls(kEncoderA, true, false);
    gpio_init(kEncoderB);
    gpio_set_dir(kEncoderB, GPIO_IN);
    gpio_set_pulls(kEncoderB, true, false);
    gpio_init(kEncoderSwitch);
    gpio_set_dir(kEncoderSwitch, GPIO_IN);
    gpio_set_pulls(kEncoderSwitch, true, false);
    encoder_.reset(read_encoder_state());

    i2c_init(i2c0, 100'000);
    gpio_set_function(kI2cSda, GPIO_FUNC_I2C);
    gpio_set_function(kI2cScl, GPIO_FUNC_I2C);
    gpio_pull_up(kI2cSda);
    gpio_pull_up(kI2cScl);

    lcd_.init();
    // Startup screen is the only non-ALV display write and intentionally
    // shows the controller name on line 1 and the emulated device on line 2.
    lcd_.clear();
    lcd_.backlight(true);
    lcd_.set_cursor(0, 0);
    lcd_.write("Protocol");
    lcd_.set_cursor(0, 1);
    lcd_.write(mock_arturia_controller::kModelConfig.product.data());

    mode_ = detect_mode();
    printf("Model: %s\n", mock_arturia_controller::model_name());
    printf("Mode selected: %s\n", mode_name(mode_));
  }

  void run() {
    while (true) {
      tud_task();
      gpio_put(kLedPin, false);
      handle_encoder();
      handle_buttons();
      handle_usb_midi();
    }
  }

 private:
  static const char* mode_name(ControllerMode mode) {
    switch (mode) {
      case ControllerMode::kArturia: return "ARTURIA";
      case ControllerMode::kDaw: return "DAW";
      case ControllerMode::kMcu: return "MCU";
    }
    return "ARTURIA";
  }

  ControllerMode detect_mode() const {
    const bool encoder_pressed = !gpio_get(kEncoderSwitch);
    const bool category_pressed = !gpio_get(kButtonPins[0]);
    if (encoder_pressed) {
      return ControllerMode::kMcu;
    }
    if (category_pressed) {
      return ControllerMode::kDaw;
    }
    return ControllerMode::kArturia;
  }

  std::uint8_t read_encoder_state() const {
    return static_cast<std::uint8_t>((gpio_get(kEncoderA) ? 1u : 0u) |
                                      (gpio_get(kEncoderB) ? 2u : 0u));
  }

  void handle_encoder() {
    const int delta = encoder_.poll_delta(read_encoder_state());
    if (delta == 0) {
      return;
    }

    if (mock_arturia_controller::IsKeyLabModel(mock_arturia_controller::kModelConfig.model)) {
      handle_keylab_encoder(delta);
      return;
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    const bool fast = (now - last_encoder_step_ms_) < 60;
    last_encoder_step_ms_ = now;

    const uint8_t cc = shift_held_ ? mock_arturia_controller::kModelConfig.encoder_shift_cc
                                   : mock_arturia_controller::kModelConfig.encoder_cc;
    const uint8_t first_value = 0x40;
    const uint8_t second_value = (delta > 0)
        ? (fast ? 0x42 : 0x41)
        : (fast ? 0x3D : 0x3E);

    send_control_change(kMidiPortMidi, cc, first_value);
    send_control_change(kMidiPortMidi, cc, second_value);
  }

  void handle_keylab_encoder(int delta) {
    const uint8_t first_value = 0x40;
    const uint8_t second_value = (delta > 0) ? 0x41 : 0x3F;

    switch (mode_) {
      case ControllerMode::kDaw:
        send_control_change(kMidiPortMidi, 0x1C, first_value);
        send_control_change(kMidiPortMidi, 0x1C, second_value);
        return;
      case ControllerMode::kMcu:
        send_control_change(kMidiPortMidi, 0x3C, delta > 0 ? 0x01 : 0x7F);
        return;
      case ControllerMode::kArturia:
        break;
    }

    const uint8_t cc = mock_arturia_controller::KeyLabEncoderControlChange(keylab_menu_);
    send_control_change(kMidiPortMidi, cc, first_value);
    send_control_change(kMidiPortMidi, cc, second_value);
  }

  void handle_buttons() {
    static std::array<bool, 5> pressed = {false, false, false, false, false};
    static std::uint32_t last_debounce_ms = 0;
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_debounce_ms < 50) {
      return;
    }
    last_debounce_ms = now;

    for (std::size_t index = 0; index < sizeof(kButtonPins) / sizeof(kButtonPins[0]); ++index) {
      const bool active = !gpio_get(kButtonPins[index]);
      if (active && !pressed[index]) {
        pressed[index] = true;
        on_button_down(static_cast<std::uint8_t>(index));
      } else if (!active && pressed[index]) {
        pressed[index] = false;
        on_button_up(static_cast<std::uint8_t>(index));
      }
    }
  }

  void on_button_down(std::uint8_t index) {
    printf("Button %u pressed\n", index);

    if (mock_arturia_controller::IsKeyLabModel(mock_arturia_controller::kModelConfig.model)) {
      on_keylab_button_down(index);
      return;
    }

    if (index == 0) {
      shift_held_ = true;
      send_control_change(kMidiPortMidi, mock_arturia_controller::kModelConfig.shift_cc, 0x7F);
      return;
    }

    if (index == 4) {
      const std::uint8_t cc = mock_arturia_controller::EncoderClickControlChange(shift_held_);
      send_control_change(kMidiPortMidi, cc, 0x7F);
      return;
    }

    const std::uint8_t note = static_cast<std::uint8_t>(mock_arturia_controller::kModelConfig.pad_base_note + index);
    send_note_on(kMidiPortMidi, note, 127);
  }

  void on_button_up(std::uint8_t index) {
    printf("Button %u released\n", index);

    if (mock_arturia_controller::IsKeyLabModel(mock_arturia_controller::kModelConfig.model)) {
      on_keylab_button_up(index);
      return;
    }

    if (index == 0) {
      shift_held_ = false;
      send_control_change(kMidiPortMidi, mock_arturia_controller::kModelConfig.shift_cc, 0x00);
      return;
    }

    if (index == 4) {
      const std::uint8_t cc = mock_arturia_controller::EncoderClickControlChange(shift_held_);
      send_control_change(kMidiPortMidi, cc, 0x00);
      return;
    }

    const std::uint8_t note = static_cast<std::uint8_t>(mock_arturia_controller::kModelConfig.pad_base_note + index);
    send_note_off(kMidiPortMidi, note);
  }

  void on_keylab_button_down(std::uint8_t index) {
    switch (mode_) {
      case ControllerMode::kArturia:
        if (index == 0) {
          // A real unit sends every press, also while its menu is already open
          send_control_change(kMidiPortMidi, mock_arturia_controller::KeyLabButtonControlChange(index), 0x7F);
          keylab_menu_ = true;
          return;
        }
        if (index == 1) {
          send_control_change(kMidiPortMidi, mock_arturia_controller::KeyLabButtonControlChange(index), 0x7F);
          keylab_menu_ = false;
          return;
        }
        if (index == 4) {
          // The release must go out on this CC too, even if Category or Preset changes the menu state meanwhile
          click_cc_ = mock_arturia_controller::KeyLabEncoderClickControlChange(keylab_menu_);
          send_control_change(kMidiPortMidi, click_cc_, 0x7F);
          return;
        }
        send_control_change(kMidiPortMidi, mock_arturia_controller::KeyLabButtonControlChange(index), 0x7F);
        return;
      case ControllerMode::kDaw:
        if (index == 0) {
          shift_held_ = true;
          return;
        }
        if (index == 4) {
          const std::uint8_t cc = shift_held_ ? 0x77 : 0x76;
          send_control_change(kMidiPortMidi, cc, 0x7F);
        }
        return;
      case ControllerMode::kMcu:
        if (index < 4) {
          send_note_on(kMidiPortMidi, static_cast<std::uint8_t>(0x65 - index), 0x7F);
        } else {
          send_note_on(kMidiPortMidi, 0x54, 0x7F);
        }
        return;
    }
  }

  void on_keylab_button_up(std::uint8_t index) {
    switch (mode_) {
      case ControllerMode::kArturia:
        if (index == 4) {
          send_control_change(kMidiPortMidi, click_cc_, 0x00);
          return;
        }
        send_control_change(kMidiPortMidi, mock_arturia_controller::KeyLabButtonControlChange(index), 0x00);
        return;
      case ControllerMode::kDaw:
        if (index == 0) {
          shift_held_ = false;
          return;
        }
        if (index == 4) {
          send_control_change(kMidiPortMidi, 0x76, 0x00);
          send_control_change(kMidiPortMidi, 0x77, 0x00);
        }
        return;
      case ControllerMode::kMcu:
        if (index < 4) {
          send_note_off(kMidiPortMidi, static_cast<std::uint8_t>(0x65 - index));
        } else {
          send_note_off(kMidiPortMidi, 0x54);
        }
        return;
    }
  }

  bool is_display_sysex(const std::uint8_t* data, std::size_t size) const {
    return size >= 7 && data[0] == 0xF0 && data[1] == 0x00 && data[2] == 0x20 &&
           data[3] == 0x6B && data[4] == 0x7F && data[5] == 0x42 && data[6] == 0x04;
  }

  void reset_display_on_first_sysex() {
    if (first_display_sysex_seen_) {
      return;
    }

    first_display_sysex_seen_ = true;
    lcd_.clear_row(1);
    cached_line1_.clear();
    cached_line2_.clear();
    cached_show_heart_ = false;
    bottom_row_buffer_.clear();
  }

  void handle_usb_midi() {
    while (tud_midi_n_available(kMidiPortAlv, 0)) {
      std::array<std::uint8_t, 4> packet{};
      tud_midi_n_stream_read(kMidiPortAlv, 0, packet.data(), packet.size());

      if (packet[0] == 0xF0) {
        sysex_buffer_.fill(0);
        sysex_length_ = 0;
        sysex_in_progress_ = true;
      }

      if (!sysex_in_progress_) {
        continue;
      }

      for (std::size_t index = 0; index < packet.size(); ++index) {
        if (sysex_length_ >= sysex_buffer_.size()) {
          sysex_in_progress_ = false;
          break;
        }

        sysex_buffer_[sysex_length_++] = packet[index];
        if (packet[index] == 0xF7) {
          if (is_display_sysex(sysex_buffer_.data(), sysex_length_)) {
            reset_display_on_first_sysex();
          }

          const auto parsed = mock_arturia_controller::ParseDisplaySysex(sysex_buffer_.data(), sysex_length_);
          if (parsed) {
            apply_display_sysex(*parsed);
          }
          sysex_in_progress_ = false;
          break;
        }
      }
    }
  }

  void show_bootloader_screen() {
    const std::string_view line1 = "Bootloader";
    const std::string_view line2 = "";

    write_line_if_changed(line1, cached_line1_, 0);
    write_line_if_changed(line2, cached_line2_, 1);

    cached_line1_ = line1;
    cached_line2_ = line2;
    cached_show_heart_ = false;
  }

  void apply_display_sysex(const mock_arturia_controller::DisplaySysexPayload& payload) {
    std::string line1 = payload.line1;
    std::string line2 = payload.line2;

    if (payload.bottom_row_selector_present) {
      const std::string selector_row = payload.bottom_row.empty() ? line2 : payload.bottom_row;
      if (bottom_row_buffer_.empty()) {
        lcd_.clear_row(1);
      }
      line2 = merge_bottom_row(bottom_row_buffer_, selector_row);
      bottom_row_buffer_ = line2;
    } else {
      bottom_row_buffer_.clear();
    }

    const bool needs_heart = payload.show_heart;
    const bool needs_up = payload.show_arrow_up;
    const bool needs_down = payload.show_arrow_down;

    if (needs_heart || needs_up || needs_down) {
      if (!heart_char_loaded_ && needs_heart) {
        lcd_.create_char(0, kHeartGlyph);
        heart_char_loaded_ = true;
      }
      if (!up_arrow_loaded_ && needs_up) {
        lcd_.create_char(1, kArrowUpGlyph);
        up_arrow_loaded_ = true;
      }
      if (!down_arrow_loaded_ && needs_down) {
        lcd_.create_char(2, kArrowDownGlyph);
        down_arrow_loaded_ = true;
      }

      for (char& ch : line1) {
        if (ch == '*') ch = '\0';
        else if (ch == '^') ch = static_cast<char>(1);
        else if (ch == 'v' || ch == 'V') ch = static_cast<char>(2);
      }
      for (char& ch : line2) {
        if (ch == '*') ch = '\0';
        else if (ch == '^') ch = static_cast<char>(1);
        else if (ch == 'v' || ch == 'V') ch = static_cast<char>(2);
      }
    }

    if (line1 == cached_line1_ && line2 == cached_line2_ && payload.show_heart == cached_show_heart_) {
      return;
    }

    write_line_if_changed(line1, cached_line1_, 0);
    write_line_if_changed(line2, cached_line2_, 1);

    cached_line1_ = std::move(line1);
    cached_line2_ = std::move(line2);
    cached_show_heart_ = payload.show_heart;
  }

  // A changed line is written whole: writing only the characters that differ
  // trusts that every earlier write reached the LCD, and one that got lost on
  // the I2C bus would then stay on the display for good.
  void write_line_if_changed(std::string_view new_line, std::string_view old_line, std::uint8_t row) {
    if (new_line == old_line) {
      return;
    }

    std::string line(new_line.substr(0, kDisplayColumns));
    line.resize(kDisplayColumns, ' ');
    lcd_.set_cursor(0, row);
    lcd_.write(line);
  }

  void send_control_change(std::uint8_t port, std::uint8_t control, std::uint8_t value) {
    const std::uint8_t packet[] = {static_cast<std::uint8_t>(0xB0), control, value};
    tud_midi_n_stream_write(port, 0, packet, sizeof(packet));
  }

  void send_note_on(std::uint8_t port, std::uint8_t note, std::uint8_t velocity) {
    const std::uint8_t packet[] = {static_cast<std::uint8_t>(0x90), note, velocity};
    tud_midi_n_stream_write(port, 0, packet, sizeof(packet));
  }

  void send_note_off(std::uint8_t port, std::uint8_t note) {
    const std::uint8_t packet[] = {static_cast<std::uint8_t>(0x80), note, 0};
    tud_midi_n_stream_write(port, 0, packet, sizeof(packet));
  }

  mock_arturia_controller::RotaryEncoder encoder_{2};
  mock_arturia_controller::Pcf8574Lcd lcd_{i2c0, 0x27};
  ControllerMode mode_ = ControllerMode::kArturia;
  bool keylab_menu_ = false;
  std::uint8_t click_cc_ = 0x73;
  bool shift_held_ = false;
  uint32_t last_encoder_step_ms_ = 0;
  std::string cached_line1_;
  std::string cached_line2_;
  std::string bottom_row_buffer_;
  bool cached_show_heart_ = false;
  bool heart_char_loaded_ = false;
  bool up_arrow_loaded_ = false;
  bool down_arrow_loaded_ = false;
  bool first_display_sysex_seen_ = false;
  std::array<std::uint8_t, 256> sysex_buffer_{};
  std::size_t sysex_length_ = 0;
  bool sysex_in_progress_ = false;
};

}  // namespace

void tud_mount_cb(void) {
}

void tud_umount_cb(void) {
}

void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
}

void tud_resume_cb(void) {
}

int main(void) {
  board_init();

  for (uint pin : kButtonPins) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, true, false);
  }

  sleep_ms(10);
  const bool bootloader_requested = !gpio_get(kButtonPins[3]);

  if (bootloader_requested) {
    i2c_init(i2c0, 100'000);
    gpio_set_function(kI2cSda, GPIO_FUNC_I2C);
    gpio_set_function(kI2cScl, GPIO_FUNC_I2C);
    gpio_pull_up(kI2cSda);
    gpio_pull_up(kI2cScl);

    mock_arturia_controller::Pcf8574Lcd lcd{i2c0, 0x27};
    lcd.init();
    lcd.clear();
    lcd.backlight(true);
    lcd.set_cursor(0, 0);
    lcd.write("Bootloader");
    lcd.set_cursor(0, 1);
    lcd.write(mock_arturia_controller::kModelConfig.product.data());
    sleep_ms(150);

    reset_usb_boot(0, 0);
  }

  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  ControllerApp app;
  app.init();
  app.run();
}
