#include <cassert>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::KeyLabButtonControlChange;
  using mock_arturia_controller::KeyLabEncoderClickControlChange;

  // A real KeyLab Essential sends a button's press (127) and its release (0)
  // on the same CC: Category 116, Preset 117, <- 28, -> 29 (firmware 1.1.10)
  assert(KeyLabButtonControlChange(0) == 0x74);
  assert(KeyLabButtonControlChange(1) == 0x75);
  assert(KeyLabButtonControlChange(2) == 0x1C);
  assert(KeyLabButtonControlChange(3) == 0x1D);

  assert(KeyLabEncoderClickControlChange(false) == 0x73);
  assert(KeyLabEncoderClickControlChange(true) == 0x71);

  return 0;
}
