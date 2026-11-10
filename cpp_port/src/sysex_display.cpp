#include "sysex_display.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace mock_arturia_controller {
namespace {

std::string extract_string(const std::uint8_t* data, std::size_t size, std::size_t start);

bool looks_like_display_text(const std::string& text) {
  if (text.size() < 2) {
    return false;
  }

  for (const unsigned char value : text) {
    if (value < 0x20 || value == 0x7F) {
      return false;
    }
  }

  return true;
}

std::size_t find_text_marker(const std::uint8_t* data, std::size_t size, std::uint8_t marker) {
  for (std::size_t index = 0; index + 1 < size; ++index) {
    if (data[index] != marker) {
      continue;
    }

    const auto text = extract_string(data, size, index + 1);
    if (looks_like_display_text(text)) {
      return index;
    }
  }

  return size;
}

std::string extract_string(const std::uint8_t* data, std::size_t size, std::size_t start) {
  if (start >= size) {
    return {};
  }

  std::string text;
  for (std::size_t index = start; index < size; ++index) {
    const auto value = data[index];
    if (value == 0x00) {
      break;
    }

    // Fresh Mk3 dumps mix the text bytes with metadata bytes (for example 0x04
    // / 0x06) and only have two physical display lines. Keep the printable
    // characters and ignore the control bytes so the visible text still renders.
    if (value >= 0x20 && value < 0x7F) {
      text.push_back(static_cast<char>(value));
    }
  }

  return text;
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

std::string build_bottom_row(const std::vector<std::uint8_t>& bytes) {
  std::string bottom_row(16, ' ');

  if (std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x18)) != bytes.end()) {
    bottom_row.replace(0, 4, "Types");
  }
  if (std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x19)) != bytes.end()) {
    bottom_row[5] = static_cast<char>(0);
  }
  if (std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x1A)) != bytes.end()) {
    bottom_row[9] = static_cast<char>(1);
  }
  if (std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x1B)) != bytes.end()) {
    bottom_row[13] = static_cast<char>(2);
  }

  return bottom_row;
}

}  // namespace

std::optional<DisplaySysexPayload> ParseDisplaySysex(const std::uint8_t* data, std::size_t size) {
  if (size < 10) {
    return std::nullopt;
  }

  const bool has_standard_header = data[0] == 0xF0 && data[1] == 0x00 && data[2] == 0x20 &&
                                   data[3] == 0x6B && data[4] == 0x7F && data[5] == 0x42;
  const bool has_mk3_dump_header = data[0] == 0xF0 && data[1] == 0x00 && data[2] == 0x20 &&
                                   data[3] == 0x04 && data[4] == 0x6B && data[5] == 0x7F && data[6] == 0x42;

  if (!has_standard_header && !has_mk3_dump_header) {
    return std::nullopt;
  }

  const bool has_display_marker = data[6] == 0x04 || (size >= 8 && data[7] == 0x04);
  if (!has_display_marker) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> bytes(data, data + size);

  // The Mk3 display path uses a small metadata prefix before the actual line text.
  // Strip that prefix only when it precedes the real text markers.
  if (bytes.size() >= 15 && bytes[6] == 0x04 && bytes[7] == 0x02 && bytes[8] == 0x60 && bytes[9] == 0x1F) {
    const std::size_t prefix_end = 9 + 6;
    if (prefix_end < bytes.size()) {
      bytes.erase(bytes.begin() + 9, bytes.begin() + prefix_end);
    }
  }

  const std::size_t marker_1 = find_text_marker(bytes.data(), bytes.size(), 0x01);
  const std::size_t marker_2 = find_text_marker(bytes.data(), bytes.size(), 0x02);
  const std::size_t marker_4 = find_text_marker(bytes.data(), bytes.size(), 0x04);

  DisplaySysexPayload payload;

  const bool has_heart_symbol = std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x19)) != bytes.end();
  const bool has_arrow_up_symbol = std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x1A)) != bytes.end();
  const bool has_arrow_down_symbol = std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x1B)) != bytes.end();
  const bool has_bottom_row_selector = has_heart_symbol || has_arrow_up_symbol || has_arrow_down_symbol ||
                                       std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>(0x18)) != bytes.end();

  payload.show_heart = has_heart_symbol;
  payload.show_arrow_up = has_arrow_up_symbol;
  payload.show_arrow_down = has_arrow_down_symbol;
  payload.bottom_row_selector_present = has_bottom_row_selector;

  // Use the real display markers for line 1 / line 2 only. The fresh Mk3 dumps
  // contain metadata bytes that look like text, so falling back to a raw scan of
  // every byte reintroduces stray fragments such as "kB" into both lines.
  if (marker_1 < bytes.size()) {
    payload.line1 = normalize_for_display(extract_string(bytes.data(), bytes.size(), marker_1 + 1));
  }
  if (marker_2 < bytes.size()) {
    payload.line2 = normalize_for_display(extract_string(bytes.data(), bytes.size(), marker_2 + 1));
  }

  payload.show_heart = payload.show_heart ||
                       (marker_4 < bytes.size() && marker_4 + 2 < bytes.size() &&
                        bytes[marker_4 + 1] == 0x46 && bytes[marker_4 + 2] == 0x20);

  if (has_bottom_row_selector) {
    payload.bottom_row = build_bottom_row(bytes);
  }

  if (payload.line1.empty() && payload.line2.empty() && has_bottom_row_selector) {
    payload.line2 = payload.bottom_row;
    return payload;
  }

  // A plain two-line text command (04 xx 60 01 ...) with empty or blank lines is
  // how a host lets go of the display; a real unit then shows nothing, so it
  // clears the display. Other messages without text stay ignored.
  const bool plain_text_command = bytes[6] == 0x04 && bytes[8] == 0x60 && bytes[9] == 0x01;
  if (payload.line1.empty() && payload.line2.empty() && !plain_text_command) {
    return std::nullopt;
  }

  return payload;
}

}  // namespace mock_arturia_controller
