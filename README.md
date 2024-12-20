# mock-arturia-controller

Emulate ("mock") parts of Arturia KeyLab for compatibility testing of the menu-driven operation ("AnalogLab mode"), specifically for https://github.com/probonopd/MiniDexed/pull/743.

Specifically, the objective is to emulate ("mock") this area of an Arturia controller

![image](https://github.com/user-attachments/assets/49d67184-bd86-43f2-b285-898e790acd32)

using inexpensive hardware (Raspberry Pi Pico, i2c display, rotary encoder, 4 buttons).

**If you have access to an Arturia MIDI controller, you can greatly help this effort by checking/completing the information in the [wiki](../../wiki/).**

## Hardware

* Raspberry Pi Pico
* 26x2 character LCD with i2c "backpack"
* KY-040-02 rotary encoder
* 4 standard button switches
* 3D printable [housing](https://github.com/probonopd/mock-arturia-controller/releases/tag/housing)

| RPi Pico      | Peripherals        |
|---------------|--------------------|
| Pin 1  (GP0)  | Display SDA        |
| Pin 2  (GP1)  | Display SCL        |
| Pin 3  (GND)  | Display GND        |
| Pin 4  (GP2)  | Button 0: Category |
| Pin 5  (GP3)  | Button 1: Preset   |
| Pin 6  (GP4)  | Button 2: <-       |
| Pin 7  (GP5)  | Button 3: ->       |
| Pin 8  (GND)  | Button GND         |
| Pin 9  (GP6)  | Rotary Encoder CLK |
| Pin 10 (GP7)  | Rotary Encoder DT  |
| Pin 11 (GP8)  | Rotary Encoder SW  |
| Pin 12 (GP9)  | Rotary Encoder +   |
| Pin 13 (GND)  | Rotary Encoder GND |
| Pin 40 (VBUS) | Display VCC        |

## Theory of Operation

The AnalogLab standalone application sends special MIDI System Exclusive (sysex) messages to the controller if the name of the MIDI device matches certain names, such as "Arturia KeyLab Essential 61".

```
Sysex message format used by Arturia KeyLab Essential 61 with AnalogLab to write to the display:
F0               # sysex header
00 20 6B 7F 42   # Arturia header
04 00 60         # set text
01 S1 00         # S1 = Instrument (e.g. 'ARP 2600')
02 S2 00         # S2 = Name (e.g. 'Bloody Swing')
02 S3 00         # S3 = Type (e.g. 'Noise')
02 S4 00         # S4 = Whether to display a heart (if 46 20, then display a heart; if nonexistent, then do not display a heart)
F7               # sysex footer

Example with heart:
F0 00 20 6B 7F 42 04 00 60 01 41 52 50 20 32 36 30 30 00 02 2A 42 6C 6F 6F 64 79 20 53 77 69 6E 67 00 03 4E 6F 69 73 65 00 04 46 20 00 F7
Example without heart:
F0 00 20 6B 7F 42 04 00 60 01 41 52 50 20 32 36 30 30 00 02 2A 42 6C 6F 6F 64 79 20 53 77 69 6E 67 00 03 4E 6F 69 73 65 00 04 00 F7
```

When buttons are pressed and when the rotary encoder is moved, then certain MIDI messages need to be sent as documented in the [wiki](../../wiki/).
  
## Installation

* Install CircuitPython `.uf2` on Raspberry Pi Pico
* Install `boot.py` and `code.py` in the root of the Raspberry Pi Pico
* Install the needed libraries in the `lib/` directory as shown below

```
lib/adafruit_hid
lib/adafruit_midi
circuitpython_i2c_lcd.py # https://github.com/dhylands/python_lcd
lcd_api.py # https://github.com/dhylands/python_lcd
```

# Configuration

In `boot.py`, you can select which device gets emulated. Known to work are:
* KeyLab Essential 61 emulation in Arturia mode with AnalogLab standalone and in Cubase
* Minilab3 emulation in DAW mode with MiniDexed

# Usage

* Power on to use in Arturia mode (e.g., with AnalogLab standalone)
* Power on while holding down button 0 ("Category") to use in DAW mode (e.g., with MiniDexed - currently works when is set to Minilab3)
* Power on while holding down the rotary encoder button to use in Mackie Control Universal (MCU) mode (e.g., with REAPER - does not work properly yet)

## Development in VSCode

Power on the device while button 2 (`<--`) is held down. This will cause the USB mass storage device to be mounted where the Python code can be edited.

* Install CircuitPython v2 extension
* Press Ctrl-Shift-P, type `circuitpython:` to see the CircuitPython related commands
* This allows to just save `code.py`, and it will be reloaded on the device and the output will be shown in the Serial Monitor

Power on the device while button 3 (`-->`) is held down. This will cause the USB mass storage device to be mounted where a new CircuitPython version (or entirely different firmware) can be uploaded without needing access to the BOOTSEL pin on the Raspberry Pi Pico.

## Note

* Turns out https://github.com/dhylands/python_lcd is needed since the library from Adafruit does not work with generic PCF8574 based i2c 16x2 character displays

## TODO

- [x] Get it to display patch names on the display sent by Arturia AnalogLab (using undocumented Arturia sysex)
- [x] Make it possible to browse presets using buttons
- [x] Make it possible to select presets using a button
- [x] Make it possible to browse categories
- [x] Make it possible to browse subcategories
- [x] Make it possible to browse presets using rotary encoder
- [ ] Fix "Mackie Control Universal" protocol (to get AnalogLab to work in REAPER; this is also what MiniDexed uses) (To enable this mode, power on the device while the rotary encoder button is held down.)
- [ ] Possibly support graphical OLED, too

## References

* https://forum.arturia.com/index.php?topic=90496.0 has _some_ documentation about the protocol
* https://www.youtube.com/watch?v=QtMN1y-Kf_w shows the Arturia Keylab mkII in action
* https://github.com/rjuang/flstudio-arturia-keylab-mk2 may have necessary information regarding the Arturia Keylab mkII protocol

