#include <cassert>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::MidiOutputPort;

  assert(MidiOutputPort() == 0);
  return 0;
}
