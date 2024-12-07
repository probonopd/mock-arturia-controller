# mock-arturia-controller

Mock Arturia controller for testing MiniDexed, specifically https://github.com/probonopd/MiniDexed/pull/743 using a Raspberry Pi Pico with a 16x2 character display attached via i2c.

## Installation

* Install CircuitPython `.uf2` on Raspberry Pi Pico
* Install the needed libraries in the `lib` directory as shown below

```
lib/adafruit_mcp230xx
lib/adafruit_hid
lib/adafruit_midi
lib/adafruit_character_lcd
```

## Development in VSCode

* Install CircuitPython v2 extension
* Press Ctrl-Shift-P, type `circuitpython:` to see the CircuitPython related commands
* This allows to just save `code.py`, and it will be reloaded on the device and the output will be shown in the Serial Monitor

## Note

* Turns out https://github.com/dhylands/python_lcd is needed since the library from Adafruit does not work with generic PCF8574 based i2c 16x2 character displays

## TODO

* Simulate Arturia USB ID and device name
* Actually display something on the display
* Support rotary encoder and button to send data back to host
* Possibly support graphical OLED mode, too
