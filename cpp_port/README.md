# C++ Pico SDK port

This directory contains the Raspberry Pi Pico SDK + TinyUSB implementation of the controller firmware for three supported Arturia emulations:

- MiniLab 3
- KeyLab Essential 61
- KeyLab Essential Mk3

## What is here

- [main.cpp](main.cpp) - main loop, button/encoder handling, MIDI I/O, and model-aware controller behavior.
- [src/model_config.h](src/model_config.h) - compile-time model profile selection for MiniLab 3, KeyLab Essential 61, and KeyLab Essential Mk3.
- [tests/keylab_button_mapping_test.cpp](tests/keylab_button_mapping_test.cpp) - regression tests for the KeyLab runtime mapping path.
- [src/pcf8574_lcd.cpp](src/pcf8574_lcd.cpp) - the PCF8574-based LCD driver.
- [src/sysex_display.cpp](src/sysex_display.cpp) - parsing and normalization of the Arturia display sysex messages.
- [usb_descriptors.cpp](usb_descriptors.cpp) - USB device + MIDI port descriptors.

## Model switch

The firmware is compiled with `-DEMULATED_MODEL=MINILAB3`, `-DEMULATED_MODEL=KEYLAB_ESSENTIAL_61`, or `-DEMULATED_MODEL=KEYLAB_ESSENTIAL_MK3`.

- `MINILAB3` uses the full four-port MIDI layout and the original Minilab runtime path.
- `KEYLAB_ESSENTIAL_61` uses the single MIDI interface that the real KeyLab exposes, and its runtime mapping follows the Python reference for button/encoder behavior.
- `KEYLAB_ESSENTIAL_MK3` uses the wiki-reported USB ID `0x028c`, the `KL Essential 61 mk3` identity, and the four-port layout (`ALV`, `DINTHRU`, `MCU/HUI`, `MIDI`).

Examples:

```sh
cmake -S cpp_port -B cpp_port/build -DPICO_BOARD=pico -DEMULATED_MODEL=MINILAB3
cmake --build cpp_port/build -j4
```

```sh
cmake -S cpp_port -B cpp_port/build -DPICO_BOARD=pico -DEMULATED_MODEL=KEYLAB_ESSENTIAL_61
cmake --build cpp_port/build -j4
```

```sh
cmake -S cpp_port -B cpp_port/build -DPICO_BOARD=pico -DEMULATED_MODEL=KEYLAB_ESSENTIAL_MK3
cmake --build cpp_port/build -j4
```

## Build prerequisites

On Debian/Ubuntu, install the standard Pico SDK toolchain:

```sh
sudo apt-get update
sudo apt-get install -y cmake ninja-build gcc-arm-none-eabi picotool
```

Then export the local SDK path and build:

```sh
export PICO_SDK_PATH="$PWD/pico-sdk"
# For MiniLab 3 protocol
cmake -S cpp_port -B cpp_port/build -G Ninja -DPICO_BOARD=pico -DEMULATED_MODEL=MINILAB3
cmake --build cpp_port/build -j4
# For KeyLab Essential 61 protocol
cmake -S cpp_port -B cpp_port/build-keylab -G Ninja -DPICO_BOARD=pico -DEMULATED_MODEL=KEYLAB_ESSENTIAL_61
cmake --build cpp_port/build-keylab -j4

# For KeyLab Essential Mk3 protocol
cmake -S cpp_port -B cpp_port/build-keylab-mk3 -G Ninja -DPICO_BOARD=pico -DEMULATED_MODEL=KEYLAB_ESSENTIAL_MK3
cmake --build cpp_port/build-keylab-mk3 -j4
```

The UF2 image is written to `cpp_port/build-*/mock_arturia_controller_cpp.uf2`.

## Real-device descriptor capture

The attached Minilab 3 reports:

- `idVendor = 0x1c75`
- `idProduct = 0x020b`
- `bcdDevice = 18.0` (USB descriptor version 0x1200)

The raw capture is stored in [real_minilab3_descriptor.txt](../real_minilab3_descriptor.txt) for comparison with the emulator.

## Interactive hardware probe

Use the real-device probe script to confirm the display path on the original controller:

```sh
python3 cpp_port/tests/real_minilab3_probe.py
```

The script prints the real USB descriptor summary, opens the attached Minilab 3 MIDI port, and asks you to confirm the text you see on the display for the standard and alternative Arturia sysex packets.

## Host-side parser test

A small native test target is available for the sysex display parser:

```sh
cmake -S cpp_port -B cpp_port/build-tests -DENABLE_TESTS=ON
cmake --build cpp_port/build-tests -j4
ctest --test-dir cpp_port/build-tests --output-on-failure
```

This validates the Arturia display-text parsing logic without requiring the Pico toolchain.

## MIDI port layout

The C++ build advertises the MiniLab 3 MIDI port family used by the controller:

- `Minilab3 ALV` for display / sysex traffic
- `Minilab3 DIN THRU`
- `Minilab3 MCU/HUI`
- `Minilab3 MIDI` for outgoing CC / note events

The current firmware uses the ALV port for inbound display traffic and the MIDI port for outbound controller events.

For the KeyLab Essential 61 path, the real controller's Arturia driver must own the display path. In REAPER, do not configure the KeyLab Essential 61 as an output device; otherwise the display update path will be broken and the controller will not show the expected display text.

## CI

The GitHub Actions workflow in [.github/workflows/cpp-port.yml](../.github/workflows/cpp-port.yml) builds both model profiles to catch regressions in the C++ port.
