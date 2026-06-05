cmake_minimum_required(VERSION 3.13)

project(mock_arturia_controller_cpp C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(ENABLE_TESTS "Build host-side tests for the sysex parser" OFF)

set(EMULATED_MODEL "MINILAB3" CACHE STRING "Which Arturia controller model to emulate")
set_property(CACHE EMULATED_MODEL PROPERTY STRINGS MINILAB3 KEYLAB_ESSENTIAL_61)

if(ENABLE_TESTS)
  add_executable(sysex_display_tests
    tests/sysex_display_test.cpp
    src/sysex_display.cpp
  )
  add_executable(usb_descriptor_layout_tests
    tests/usb_descriptor_layout_test.cpp
  )
  add_executable(encoder_click_mapping_tests
    tests/encoder_click_mapping_test.cpp
  )
  add_executable(rotary_encoder_tests
    tests/rotary_encoder_test.cpp
  )
  add_executable(keylab_usb_identity_tests
    tests/keylab_usb_identity_test.cpp
  )
  add_executable(keylab_button_mapping_tests
    tests/keylab_button_mapping_test.cpp
  )
  add_executable(keylab_output_port_tests
    tests/midi_output_port_test.cpp
  )

  target_include_directories(sysex_display_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(usb_descriptor_layout_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(encoder_click_mapping_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(rotary_encoder_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(keylab_usb_identity_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(keylab_button_mapping_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )
  target_include_directories(keylab_output_port_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
  )

  target_compile_definitions(sysex_display_tests PRIVATE EMULATED_MODEL_MINILAB3=1)
  target_compile_definitions(usb_descriptor_layout_tests PRIVATE EMULATED_MODEL_MINILAB3=1)
  target_compile_definitions(encoder_click_mapping_tests PRIVATE EMULATED_MODEL_MINILAB3=1)
  target_compile_definitions(rotary_encoder_tests PRIVATE EMULATED_MODEL_MINILAB3=1)

  target_compile_options(sysex_display_tests PRIVATE -Wall -Wextra)
  target_compile_options(usb_descriptor_layout_tests PRIVATE -Wall -Wextra)
  target_compile_options(encoder_click_mapping_tests PRIVATE -Wall -Wextra)
  target_compile_options(rotary_encoder_tests PRIVATE -Wall -Wextra)
  target_compile_options(keylab_usb_identity_tests PRIVATE -Wall -Wextra)
  target_compile_options(keylab_button_mapping_tests PRIVATE -Wall -Wextra)
  target_compile_options(keylab_output_port_tests PRIVATE -Wall -Wextra)
  target_compile_definitions(keylab_usb_identity_tests PRIVATE EMULATED_MODEL_KEYLAB_ESSENTIAL_61=1)
  target_compile_definitions(keylab_button_mapping_tests PRIVATE EMULATED_MODEL_KEYLAB_ESSENTIAL_61=1)
  target_compile_definitions(keylab_output_port_tests PRIVATE EMULATED_MODEL_KEYLAB_ESSENTIAL_61=1)

  enable_testing()
  add_test(NAME sysex_display_tests COMMAND sysex_display_tests)
  add_test(NAME usb_descriptor_layout_tests COMMAND usb_descriptor_layout_tests)
  add_test(NAME encoder_click_mapping_tests COMMAND encoder_click_mapping_tests)
  add_test(NAME rotary_encoder_tests COMMAND rotary_encoder_tests)
  add_test(NAME keylab_usb_identity_tests COMMAND keylab_usb_identity_tests)
  add_test(NAME keylab_button_mapping_tests COMMAND keylab_button_mapping_tests)
  add_test(NAME keylab_output_port_tests COMMAND keylab_output_port_tests)
  return()
endif()

set(BOARD pico_sdk)
include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

pico_sdk_init()

add_executable(mock_arturia_controller_cpp
  main.cpp
  usb_descriptors.cpp
  src/pcf8574_lcd.cpp
  src/sysex_display.cpp
)

target_include_directories(mock_arturia_controller_cpp PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_options(mock_arturia_controller_cpp PRIVATE -Wall -Wextra)
target_compile_definitions(mock_arturia_controller_cpp PRIVATE
  EMULATED_MODEL_${EMULATED_MODEL}=1
)

target_link_libraries(mock_arturia_controller_cpp
  pico_stdlib
  pico_time
  hardware_gpio
  hardware_i2c
  tinyusb_device
  tinyusb_board
)

pico_add_extra_outputs(mock_arturia_controller_cpp)

