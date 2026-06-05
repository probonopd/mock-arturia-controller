#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace mock_arturia_controller {

enum class EmulatedModel : std::uint8_t {
  kKeyLabEssential61,
  kMinilab3,
};

struct ModelConfig {
  EmulatedModel model;
  std::uint16_t vid;
  std::uint16_t pid;
  std::string_view manufacturer;
  std::string_view product;
  std::string_view midi_interface_name;
  std::array<std::string_view, 4> midi_port_names;
  std::uint8_t midi_port_count;
  std::uint8_t encoder_cc;
  std::uint8_t encoder_shift_cc;
  std::uint8_t encoder_click_cc;
  std::uint8_t encoder_click_shift_cc;
  std::uint8_t shift_cc;
  std::uint8_t pad_base_note;
};

#if defined(EMULATED_MODEL_MINILAB3)
static constexpr ModelConfig kModelConfig{
  EmulatedModel::kMinilab3,
  0x1c75,
  0x020b,
  "Arturia",
  "Minilab3",
  "Minilab3 MIDI",
  {{"Minilab3 ALV", "Minilab3 DIN THRU", "Minilab3 MCU/HUI", "Minilab3 MIDI"}},
  4,
  0x72,
  0x70,
  0x73,
  0x71,
  0x09,
  0x24,
};
#elif defined(EMULATED_MODEL_KEYLAB_ESSENTIAL_61)
static constexpr ModelConfig kModelConfig{
  EmulatedModel::kKeyLabEssential61,
  0x1c75,
  0x028a,
  "Arturia",
  "Arturia KeyLab Essential 61",
  "Arturia KeyLab Essential 61",
  {{"Arturia KeyLab Essential 61", "Arturia KeyLab Essential 61", "Arturia KeyLab Essential 61", "Arturia KeyLab Essential 61"}},
  1,
  0x72,
  0x70,
  0x73,
  0x71,
  0x09,
  0x24,
};
#else
#error "EMULATED_MODEL must be set to MINILAB3 or KEYLAB_ESSENTIAL_61"
#endif

inline constexpr std::uint8_t MidiOutputPort() {
  return (kModelConfig.midi_port_count == 1) ? 0u : 3u;
}

inline constexpr std::uint8_t EncoderClickControlChange(bool shift_held) {
  return shift_held ? kModelConfig.encoder_click_shift_cc : kModelConfig.encoder_click_cc;
}

inline constexpr std::uint8_t KeyLabButtonControlChange(std::uint8_t index, bool pressed) {
  switch (index) {
    case 0: return pressed ? 0x74 : 0x75;
    case 1: return pressed ? 0x75 : 0x1C;
    case 2: return pressed ? 0x1C : 0x1D;
    case 3: return pressed ? 0x1D : 0x74;
    default: return 0;
  }
}

inline constexpr std::uint8_t KeyLabEncoderControlChange(bool menu_active) {
  return menu_active ? 0x70 : 0x72;
}

inline constexpr std::uint8_t KeyLabEncoderClickControlChange(bool menu_active) {
  return menu_active ? 0x71 : 0x73;
}

inline constexpr const char* model_name() {
  return (kModelConfig.model == EmulatedModel::kMinilab3) ? "MINILAB3" : "KEYLAB_ESSENTIAL_61";
}

}  // namespace mock_arturia_controller
