#include <array>
#include <cstdint>
#include <iostream>

#include "src/sysex_display.h"

int main() {
  using mock_arturia_controller::DisplaySysexPayload;
  using mock_arturia_controller::ParseDisplaySysex;

  constexpr std::uint8_t kBytes[] = {
      0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60,
      0x01, 0x41, 0x52, 0x50, 0x20, 0x32, 0x36, 0x30, 0x30, 0x00,
      0x02, 0x2A, 0x42, 0x6C, 0x6F, 0x6F, 0x64, 0x79, 0x20, 0x53,
      0x77, 0x69, 0x6E, 0x67, 0x00, 0x03, 0x4E, 0x6F, 0x69, 0x73,
      0x65, 0x00, 0x04, 0x46, 0x20, 0x00, 0xF7,
  };

  const auto payload = ParseDisplaySysex(kBytes, sizeof(kBytes));
  if (!payload.has_value()) {
    std::cerr << "Expected display sysex to parse" << std::endl;
    return 1;
  }

  const DisplaySysexPayload& parsed = *payload;
  if (parsed.line1 != "ARP 2600" || parsed.line2 != "*Bloody Swing" || !parsed.show_heart) {
    std::cerr << "Unexpected parsed display text for the standard display sysex format" << std::endl;
    return 2;
  }

  constexpr std::uint8_t kMk3Bytes[] = {
      0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x02, 0x60,
      0x1F, 0x02, 0x01, 0x00, 0x00, 0x01, 0x00,
      0x01, 'L', 'i', 'n', 'e', '1', 0x00,
      0x02, 'L', 'i', 'n', 'e', '2', 0x00,
      0x03, 'T', 'y', 'p', 'e', 0x00,
      0x04, 0x46, 0x20, 0x00,
      0xF7,
  };

  const auto mk3_payload = ParseDisplaySysex(kMk3Bytes, sizeof(kMk3Bytes));
  if (!mk3_payload.has_value()) {
    std::cerr << "Expected the wiki-style two-line display sysex format to parse" << std::endl;
    return 3;
  }

  if (mk3_payload->line1 != "Line1" || mk3_payload->line2 != "Line2" || !mk3_payload->show_heart) {
    std::cerr << "Unexpected parsed display text for the wiki-style display sysex format" << std::endl;
    return 4;
  }
  constexpr std::uint8_t kFreshMk3DumpBytes[] = {
      0xF0, 0x00, 0x20, 0x04, 0x6B, 0x7F, 0x42, 0x04, 0x04, 0x00, 0x60,
      0x04, 0x1B, 0x01, 0x38, 0x04, 0x30, 0x38, 0x00, 0x04, 0x08, 0x03,
      0x00, 0x00, 0x05, 0xF7,
  };

  const auto fresh_dump_payload = ParseDisplaySysex(kFreshMk3DumpBytes, sizeof(kFreshMk3DumpBytes));
  if (!fresh_dump_payload.has_value()) {
    std::cerr << "Expected the fresh Mk3 dump bytes to parse" << std::endl;
    return 5;
  }

  if (fresh_dump_payload->line1 != "808") {
    std::cerr << "Unexpected parsed display text for the fresh Mk3 dump packet" << std::endl;
    return 6;
  }

  constexpr std::uint8_t kFreshMk3BassBytes[] = {
      0xF0, 0x00, 0x20, 0x04, 0x6B, 0x7F, 0x42, 0x04, 0x04, 0x00, 0x60,
      0x04, 0x01, 0x01, 0x04, 0x04, 0x00, 0x02, 0x42, 0x04, 0x61, 0x73,
      0x73, 0x06, 0x00, 0xF7,
  };

  const auto bass_payload = ParseDisplaySysex(kFreshMk3BassBytes, sizeof(kFreshMk3BassBytes));
  if (!bass_payload.has_value()) {
    std::cerr << "Expected the Bass Mk3 dump bytes to parse" << std::endl;
    return 7;
  }

  if (!bass_payload->line1.empty() || bass_payload->line2 != "Bass") {
    std::cerr << "Unexpected parsed display text for the Bass Mk3 dump packet" << std::endl;
    return 8;
  }

  constexpr std::uint8_t kRow3HeartBytes[] = {
      0xF0, 0x00, 0x20, 0x04, 0x6B, 0x7F, 0x42, 0x04, 0x04, 0x00, 0x60,
      0x04, 0x19, 0x2A, 0x2A, 0x06, 0x2A, 0xF7,
  };

  const auto row3_heart = ParseDisplaySysex(kRow3HeartBytes, sizeof(kRow3HeartBytes));
  if (!row3_heart.has_value() || row3_heart->bottom_row.empty() || !row3_heart->show_heart || row3_heart->show_arrow_up || row3_heart->show_arrow_down || !row3_heart->bottom_row_selector_present) {
    std::cerr << "Expected the row-3 heart selector to set the heart glyph explicitly" << std::endl;
    return 9;
  }

  constexpr std::uint8_t kRow3ArrowsBytes[] = {
      0xF0, 0x00, 0x20, 0x04, 0x6B, 0x7F, 0x42, 0x04, 0x04, 0x00, 0x60,
      0x04, 0x1A, 0x2A, 0x2A, 0x06, 0x2A, 0xF7,
  };

  const auto row3_arrows = ParseDisplaySysex(kRow3ArrowsBytes, sizeof(kRow3ArrowsBytes));
  if (!row3_arrows.has_value() || row3_arrows->bottom_row.empty() || !row3_arrows->show_arrow_up || row3_arrows->show_arrow_down || row3_arrows->show_heart) {
    std::cerr << "Expected the row-3 arrow selector to set the arrow glyph explicitly" << std::endl;
    return 10;
  }

  std::cout << "sysex display parser ok" << std::endl;
  // A host that lets go of the display sends two blank lines (KeyLab: 16 spaces
  // each) or two empty ones (MiniLab 3); a real unit then shows nothing, so the
  // message must clear the display, not be ignored.
  std::array<std::uint8_t, 46> blank_keylab{};
  {
    std::size_t n = 0;
    for (std::uint8_t b : {0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60, 0x01}) {
      blank_keylab[n++] = b;
    }
    for (int i = 0; i < 16; ++i) {
      blank_keylab[n++] = ' ';
    }
    blank_keylab[n++] = 0x00;
    blank_keylab[n++] = 0x02;
    for (int i = 0; i < 16; ++i) {
      blank_keylab[n++] = ' ';
    }
    blank_keylab[n++] = 0x00;
    blank_keylab[n++] = 0xF7;
  }
  const auto blank = ParseDisplaySysex(blank_keylab.data(), blank_keylab.size());
  if (!blank.has_value() || !blank->line1.empty() || !blank->line2.empty()) {
    std::cerr << "Expected two blank lines to clear the display" << std::endl;
    return 10;
  }

  constexpr std::uint8_t kEmptyMinilab[] = {
      0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x02, 0x60, 0x01, 0x00, 0x02, 0x00, 0xF7,
  };
  const auto empty = ParseDisplaySysex(kEmptyMinilab, sizeof(kEmptyMinilab));
  if (!empty.has_value() || !empty->line1.empty() || !empty->line2.empty()) {
    std::cerr << "Expected two empty lines to clear the display" << std::endl;
    return 11;
  }

  return 0;
}
