#include <cassert>
#include <string_view>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::kModelConfig;

  assert(kModelConfig.vid == 0x1c75);
  assert(kModelConfig.pid == 0x028a);
  assert(kModelConfig.manufacturer == std::string_view("Arturia"));
  assert(kModelConfig.product == std::string_view("Arturia KeyLab Essential 61"));
  assert(kModelConfig.midi_port_count == 1);
  assert(kModelConfig.midi_interface_name == std::string_view("Arturia KeyLab Essential 61"));

  return 0;
}
