#include <cassert>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::KeyLabButtonControlChange;
  using mock_arturia_controller::KeyLabEncoderClickControlChange;

  assert(KeyLabButtonControlChange(0, true) == 0x74);
  assert(KeyLabButtonControlChange(0, false) == 0x75);
  assert(KeyLabButtonControlChange(1, true) == 0x75);
  assert(KeyLabButtonControlChange(1, false) == 0x1C);
  assert(KeyLabButtonControlChange(2, true) == 0x1C);
  assert(KeyLabButtonControlChange(2, false) == 0x1D);
  assert(KeyLabButtonControlChange(3, true) == 0x1D);
  assert(KeyLabButtonControlChange(3, false) == 0x74);

  assert(KeyLabEncoderClickControlChange(false) == 0x73);
  assert(KeyLabEncoderClickControlChange(true) == 0x71);

  return 0;
}
