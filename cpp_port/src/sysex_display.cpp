#include "sysex_display.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace mock_arturia_controller {
namespace {

std::size_t find_marker(const std::uint8_t* data, std::size_t size, std::uint8_t marker) {
  for (std::size_t index = 0; index + 1 < size; ++index) {
    if (data[index] == marker) {
      return index;
    }
  }
  return size;
}

std::string extract_string(const std::uint8_t* data, std::size_t size, std::size_t start) {
  if (start >= size) {
    return {};
  }

  const std::size_t end = std::find(data + start, data + size, static_cast<std::uint8_t>(0x00)) - data;
  if (end <= start) {
    return {};
  }

  return std::string(reinterpret_cast<const char*>(data + start), end - start);
}

std::string trim_whitespace(std::string text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }

  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

std::string normalize_for_display(std::string text) {
  text = trim_whitespace(std::move(text));

  // Keep the display text compact for the MiniLab 3 style UI.
  for (std::size_t index = 0; index + 2 < text.size(); ++index) {
    if (text[index] == ' ' && text[index + 1] == ' ' && text[index + 2] == ' ') {
      text.erase(index + 1, 1);
      --index;
    }
  }

  for (std::size_t index = 0; index + 1 < text.size(); ++index) {
    if (text[index] == ' ' && text[index + 1] == ' ') {
      text.erase(index + 1, 1);
      --index;
    }
  }

  return text;
}

}  // namespace

std::optional<DisplaySysexPayload> ParseDisplaySysex(const std::uint8_t* data, std::size_t size) {
  if (size < 10) {
    return std::nullopt;
  }

  if (data[0] != 0xF0 || data[1] != 0x00 || data[2] != 0x20 ||
      data[3] != 0x6B || data[4] != 0x7F || data[5] != 0x42) {
    return std::nullopt;
  }

  if (data[6] != 0x04) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> bytes(data, data + size);

  // Keep the parser focused on the verified Minilab 3 display format.
  // The extra metadata prefix is not part of the real display text path.
  if (bytes.size() >= 15 && bytes[8] == 0x60 && bytes[9] != 0x01) {
    bytes.erase(bytes.begin() + 9, bytes.begin() + 15);
  }

  const std::size_t marker_1 = find_marker(bytes.data(), bytes.size(), 0x01);
  const std::size_t marker_2 = marker_1 < bytes.size()
                                   ? marker_1 + 1 + find_marker(bytes.data() + marker_1 + 1, bytes.size() - marker_1 - 1, 0x02)
                                   : bytes.size();
  const std::size_t marker_3 = marker_2 < bytes.size()
                                   ? marker_2 + 1 + find_marker(bytes.data() + marker_2 + 1, bytes.size() - marker_2 - 1, 0x03)
                                   : bytes.size();
  const std::size_t marker_4 = marker_3 < bytes.size()
                                   ? marker_3 + 1 + find_marker(bytes.data() + marker_3 + 1, bytes.size() - marker_3 - 1, 0x04)
                                   : bytes.size();

  if (marker_1 >= bytes.size() || marker_2 >= bytes.size()) {
    return std::nullopt;
  }

  DisplaySysexPayload payload;
  payload.line1 = normalize_for_display(extract_string(bytes.data(), bytes.size(), marker_1 + 1));
  payload.line2 = normalize_for_display(extract_string(bytes.data(), bytes.size(), marker_2 + 1));

  payload.show_heart = marker_4 < bytes.size() && marker_4 + 2 < bytes.size() &&
                       bytes[marker_4 + 1] == 0x46 && bytes[marker_4 + 2] == 0x20;

  if (payload.line1.empty() && payload.line2.empty()) {
    return std::nullopt;
  }

  return payload;
}

}  // namespace mock_arturia_controller
