# mock-arturia-controller

Mock Arturia KeyLab Essential 61 for testing MiniDexed, specifically https://github.com/probonopd/MiniDexed/pull/743 using a Raspberry Pi Pico with a 16x2 character display attached via i2c.

## Theory of Operation

The AnalogLab standalone application sends special MIDI System Exclusive (sysex) messages to the controller if the name of the MIDI device matches certain names, such as "Arturia KeyLab Essential 61".
  
## Installation

* Install CircuitPython `.uf2` on Raspberry Pi Pico
* Install the needed libraries in the `lib` directory as shown below

```
lib/adafruit_hid
lib/adafruit_midi
circuitpython_i2c_lcd.py # https://github.com/dhylands/python_lcd
lcd_api.py # https://github.com/dhylands/python_lcd
```

## Development in VSCode

* Install CircuitPython v2 extension
* Press Ctrl-Shift-P, type `circuitpython:` to see the CircuitPython related commands
* This allows to just save `code.py`, and it will be reloaded on the device and the output will be shown in the Serial Monitor

## Note

* Turns out https://github.com/dhylands/python_lcd is needed since the library from Adafruit does not work with generic PCF8574 based i2c 16x2 character displays

## TODO

- [x] Get it to display patch names on the display sent by Arturia AnalogLab (using undocumented Arturia sysex)
- [ ] Get it to work in a DAW like REAPER, too (it seems to use the Mackie Control Universal protocol there)
- [ ] Support rotary encoder and button to send data back to host
- [ ] Possibly support graphical OLED mode, too

## References

* https://forum.arturia.com/index.php?topic=90496.0 has _some_ documentation about the protocol
* https://www.youtube.com/watch?v=QtMN1y-Kf_w shows the Arturia Keylab mkII in action
* https://github.com/rjuang/flstudio-arturia-keylab-mk2 may have necessary information regarding the Arturia Keylab mkII protocol

