#pragma once

#include <cstddef>
#include <cstdint>

namespace mock_arturia_controller {

inline constexpr std::uint8_t MidiCableNumberForPort(std::size_t port_index) {
  return static_cast<std::uint8_t>(port_index + 1);
}

}  // namespace mock_arturia_controller
