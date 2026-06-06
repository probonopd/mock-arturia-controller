#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mock_arturia_controller {

struct DisplaySysexPayload {
  std::string line1;
  std::string line2;
  std::string bottom_row;
  bool show_heart = false;
  bool show_arrow_up = false;
  bool show_arrow_down = false;
  bool bottom_row_selector_present = false;
};

std::optional<DisplaySysexPayload> ParseDisplaySysex(const std::uint8_t* data, std::size_t size);

}  // namespace mock_arturia_controller
