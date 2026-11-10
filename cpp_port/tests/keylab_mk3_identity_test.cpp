#include <cassert>
#include <string_view>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::IsKeyLabModel;
  using mock_arturia_controller::KeyLabButtonControlChange;
  using mock_arturia_controller::KeyLabEncoderControlChange;
  using mock_arturia_controller::KeyLabEncoderClickControlChange;
  using mock_arturia_controller::kModelConfig;

  assert(IsKeyLabModel(kModelConfig.model));
  assert(KeyLabButtonControlChange(0) == 30);
  assert(KeyLabButtonControlChange(1) == 31);
  assert(KeyLabButtonControlChange(2) == 86);
  assert(KeyLabButtonControlChange(3) == 87);
  assert(KeyLabEncoderControlChange(false) == 114);
  assert(KeyLabEncoderClickControlChange(false) == 115);
  assert(kModelConfig.vid == 0x1c75);
  assert(kModelConfig.pid == 0x028c);
  assert(kModelConfig.manufacturer == std::string_view("Arturia"));
  assert(kModelConfig.product == std::string_view("KL Essential 61 mk3"));
  assert(kModelConfig.midi_interface_name == std::string_view("KL Essential 61 mk3 MIDI"));
  assert(kModelConfig.midi_port_count == 4);
  assert(kModelConfig.midi_port_names[0] == std::string_view("KL Essential 61 mk3 ALV"));
  assert(kModelConfig.midi_port_names[1] == std::string_view("KL Essential 61 mk3 DINTHRU"));
  assert(kModelConfig.midi_port_names[2] == std::string_view("KL Essential 61 mk3 MCU/HUI"));
  assert(kModelConfig.midi_port_names[3] == std::string_view("KL Essential 61 mk3 MIDI"));

  return 0;
}
