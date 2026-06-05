#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_set>

#include "src/midi_descriptor_helpers.h"
#include "src/model_config.h"

namespace {

using mock_arturia_controller::MidiCableNumberForPort;

std::uint8_t inline JackIdInEmb(std::uint8_t cable) {
  return static_cast<std::uint8_t>(((cable - 1) * 4) + 1);
}

std::uint8_t inline JackIdInExt(std::uint8_t cable) {
  return static_cast<std::uint8_t>(((cable - 1) * 4) + 2);
}

std::uint8_t inline JackIdOutEmb(std::uint8_t cable) {
  return static_cast<std::uint8_t>(((cable - 1) * 4) + 3);
}

std::uint8_t inline JackIdOutExt(std::uint8_t cable) {
  return static_cast<std::uint8_t>(((cable - 1) * 4) + 4);
}

}  // namespace

int main() {
  if (mock_arturia_controller::kModelConfig.midi_port_count != 4) {
    std::cerr << "Expected the Minilab3 profile to expose four MIDI ports" << std::endl;
    return 1;
  }

  std::array<std::uint8_t, 4> cable_numbers = {
      MidiCableNumberForPort(0),
      MidiCableNumberForPort(1),
      MidiCableNumberForPort(2),
      MidiCableNumberForPort(3),
  };

  if (cable_numbers != std::array<std::uint8_t, 4>{1, 2, 3, 4}) {
    std::cerr << "Expected the four MIDI ports to use cable numbers 1..4" << std::endl;
    return 1;
  }

  std::unordered_set<std::uint8_t> seen_ids;
  for (std::uint8_t cable : cable_numbers) {
    const std::uint8_t ids[] = {
        JackIdInEmb(cable),
        JackIdInExt(cable),
        JackIdOutEmb(cable),
        JackIdOutExt(cable),
    };

    for (std::uint8_t id : ids) {
      if (!seen_ids.insert(id).second) {
        std::cerr << "Duplicate MIDI jack id detected: " << static_cast<int>(id) << std::endl;
        return 2;
      }
    }
  }

  std::cout << "usb descriptor jack ids are unique" << std::endl;
  return 0;
}
