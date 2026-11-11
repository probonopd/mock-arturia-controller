#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "src/sysex_assembler.h"
#include "src/sysex_display.h"

namespace {

// The display message for "Volume" / "Off", fed in reads of the given sizes, as the USB MIDI stream hands them over.
bool shows_volume(const std::vector<std::size_t>& reads, const std::vector<std::uint8_t>& before) {
  std::vector<std::uint8_t> stream = before;
  const std::vector<std::uint8_t> message = {0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60, 0x01, 'V', 'o',
                                             'l',  'u',  'm',  'e',  0x00, 0x02, 'O',  'f',  'f',  0x00, 0xF7};
  stream.insert(stream.end(), message.begin(), message.end());
  mock_arturia_controller::SysexAssembler assembler;
  std::size_t at = 0, read = 0;
  bool shown = false;
  while (at < stream.size()) {
    const std::size_t n = std::min(reads[read++ % reads.size()], stream.size() - at);
    for (std::size_t i = 0; i < n; ++i) {
      if (assembler.feed(stream[at + i])) {
        const auto parsed = mock_arturia_controller::ParseDisplaySysex(assembler.data(), assembler.size());
        shown = parsed && parsed->line1 == "Volume" && parsed->line2 == "Off";
      }
    }
    at += n;
  }
  return shown;
}

}  // namespace

int main() {
  int failed = 0;
  // reads of 4, of 3 (one USB-MIDI packet of system exclusive), of 1 and of uneven sizes
  const std::vector<std::vector<std::size_t>> patterns = {{4}, {3}, {1}, {2, 3}, {4, 3, 1}, {64}};
  for (const auto& reads : patterns) {
    if (!shows_volume(reads, {})) {
      std::cerr << "message not assembled with reads of " << reads[0] << std::endl;
      ++failed;
    }
    // a message that starts in the middle of a read, after the end of another one
    if (!shows_volume(reads, {0xF0, 0x7E, 0x7F, 0xF7, 0xB0})) {
      std::cerr << "message after another one not assembled with reads of " << reads[0] << std::endl;
      ++failed;
    }
  }
  return failed;
}
