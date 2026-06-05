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


  std::cout << "sysex display parser ok" << std::endl;
  return 0;
}
