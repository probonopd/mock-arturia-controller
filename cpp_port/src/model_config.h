#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace mock_arturia_controller {

enum class EmulatedModel : std::uint8_t {
  kKeyLabEssential61,
  kKeyLabEssentialMk3,
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
#elif defined(EMULATED_MODEL_KEYLAB_ESSENTIAL_MK3)
static constexpr ModelConfig kModelConfig{
  EmulatedModel::kKeyLabEssentialMk3,
  0x1c75,
  0x028c,
  "Arturia",
  "KL Essential 61 mk3",
  "KL Essential 61 mk3 MIDI",
  {{"KL Essential 61 mk3 ALV", "KL Essential 61 mk3 DINTHRU", "KL Essential 61 mk3 MCU/HUI", "KL Essential 61 mk3 MIDI"}},
  4,
  0x72,
  0x70,
  0x73,
  0x71,
  0x09,
  0x24,
};
#else
#error "EMULATED_MODEL must be set to MINILAB3, KEYLAB_ESSENTIAL_61, or KEYLAB_ESSENTIAL_MK3"
#endif

inline constexpr bool IsKeyLabModel(EmulatedModel model) {
  return model == EmulatedModel::kKeyLabEssential61 || model == EmulatedModel::kKeyLabEssentialMk3;
}

inline constexpr std::uint8_t MidiOutputPort() {
  return (kModelConfig.midi_port_count == 1) ? 0u : 3u;
}

inline constexpr std::uint8_t EncoderClickControlChange(bool shift_held) {
  return shift_held ? kModelConfig.encoder_click_shift_cc : kModelConfig.encoder_click_cc;
}

// The CC of a button; its press (127) and its release (0) use the same one, as
// on a real KeyLab Essential (firmware 1.1.10: the button scan reports both
// edges and the CC helper sends 127 or 0 on the button's own CC).
inline constexpr std::uint8_t KeyLabButtonControlChange(std::uint8_t index) {
  if (kModelConfig.model == EmulatedModel::kKeyLabEssentialMk3) {
    switch (index) {
      case 0: return 30;
      case 1: return 31;
      case 2: return 86;
      case 3: return 87;
      default: return 0;
    }
  }

  switch (index) {
    case 0: return 0x74;  // Category
    case 1: return 0x75;  // Preset
    case 2: return 0x1C;  // <-
    case 3: return 0x1D;  // ->
    default: return 0;
  }
}

inline constexpr std::uint8_t KeyLabEncoderControlChange(bool menu_active) {
  if (kModelConfig.model == EmulatedModel::kKeyLabEssentialMk3) {
    return 114;
  }
  return menu_active ? 0x70 : 0x72;
}

inline constexpr std::uint8_t KeyLabEncoderClickControlChange(bool menu_active) {
  if (kModelConfig.model == EmulatedModel::kKeyLabEssentialMk3) {
    return 115;
  }
  return menu_active ? 0x71 : 0x73;
}

inline constexpr const char* model_name() {
  if (kModelConfig.model == EmulatedModel::kMinilab3) {
    return "MINILAB3";
  }
  if (kModelConfig.model == EmulatedModel::kKeyLabEssentialMk3) {
    return "KEYLAB_ESSENTIAL_MK3";
  }
  return "KEYLAB_ESSENTIAL_61";
}

}  // namespace mock_arturia_controller
