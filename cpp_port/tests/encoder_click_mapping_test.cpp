#include <cstdint>
#include <iostream>

#include "src/model_config.h"

int main() {
  using mock_arturia_controller::EncoderClickControlChange;

  if (EncoderClickControlChange(false) != 0x73) {
    std::cerr << "Expected normal encoder click CC to be 0x73" << std::endl;
    return 1;
  }

  if (EncoderClickControlChange(true) != 0x71) {
    std::cerr << "Expected shift+encoder click CC to be 0x71" << std::endl;
    return 2;
  }

  std::cout << "encoder click CC mapping ok" << std::endl;
  return 0;
}
