#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mock_arturia_controller {

struct DisplaySysexPayload {
  std::string line1;
  std::string line2;
  bool show_heart = false;
};

std::optional<DisplaySysexPayload> ParseDisplaySysex(const std::uint8_t* data, std::size_t size);

}  // namespace mock_arturia_controller
