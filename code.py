import board
import busio
import usb_midi
import adafruit_midi

from circuitpython_i2c_lcd import I2cLcd # https://github.com/dhylands/python_lcd

# Apparently all of these imports are necessary for the MIDI sysex message to be recognized
# otherwise the message is not recognized as a known MIDI message
from adafruit_midi.control_change import ControlChange
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff
from adafruit_midi.pitch_bend import PitchBend
from adafruit_midi.program_change import ProgramChange
from adafruit_midi.start import Start
from adafruit_midi.stop import Stop
from adafruit_midi.system_exclusive import SystemExclusive
from adafruit_midi.timing_clock import TimingClock
from adafruit_midi.midi_message import MIDIMessage

i2c = busio.I2C(board.GP1, board.GP0)

# Lock the I2C bus
while i2c.try_lock():
    pass

# Scan for I2C devices
devices = i2c.scan()

print("I2C devices found:", [hex(device) for device in devices])

# Try to find LCD at address 0x27 or 0x3f
lcd = None
for address in (0x27, 0x3f):
    try:
        lcd = I2cLcd(i2c, address, 2, 16)
        print("LCD found at address", hex(address))
        break
    except ValueError:
        print("No LCD found at address", hex(address))
# Throw error if no LCD found at either address
if lcd is None:
    raise ValueError("No LCD found!")

lcd.clear()
lcd.putstr("It works!\nSecond line")

# Write a message to the LCD, two lines
lcd.message = "Hello\nCircuitPython!"

# Print the available ports
print("Available MIDI ports:", usb_midi.ports)
midi = adafruit_midi.MIDI(midiIn=usb_midi.ports[0], midi_out=usb_midi.ports[1], in_channel=0, out_channel=0, debug=False)
print("MIDI input port:", usb_midi.ports[0])
print("MIDI input channel:", midi.in_channel)
print("MIDI output port:", usb_midi.ports[1])
print("MIDI output channel:", midi.out_channel)

"""
Sysex message format:
F0               # sysex header
00 20 6B 7F 42   # Arturia header
04 02 60         # set text
01 S1 00         # S1 = line 1 of the text
02 S2            # S2 = line 2 of the text
F7               # sysex footer

Example: This message sets the first line to "Hello" and the second line to "World"
F0 00 20 6B 7F 42 04 02 60 01 48 65 6C 6C 6F 00 02 57 6F 72 6C 64 F7
NOTE: For some devices, the sysex message might instead start with
F0 00 20 6B 7F 42 04 00
F0 00 20 6B 7F 42 04 00 60 01 48 65 6C 6C 6F 00 02 57 6F 72 6C 64 F7
"""

# Define the expected header
arturia_header = [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42]
expected_header = [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04]

while True:
    # Check for incoming MIDI messages
    message = midi.receive()

    if message is not None:

        try:
            bytes = list(message.__bytes__())
            print("Received:", ' '.join([f"{b:02X}" for b in bytes]))
            # lcd.clear()
            # lcd.putstr(''.join([f"{b:02X}" for b in bytes]))
            # print("--> https://www.google.com/search?q=%22" + '+'.join([f"{b:02X}" for b in bytes]) + "%22")
        except:
            pass
        
        # If sysex, then check if it starts with the expected header
        if isinstance(message, SystemExclusive):
            if bytes[:6] == arturia_header:
                print("Arturia sysex recognized")
            #  The software driver sends the standard device inquiry using the “wildcard” device ID of 7F.
            if bytes[:6] == [0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7]:
                print("Device inquiry recognized")
                lcd.clear()
                lcd.putstr("Received device inquiry")
                # midi.send(SystemExclusive([0xF0, 0x7E, 0x7F, 0x06, 0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x74, 0xF7]))
            if bytes[:7] == expected_header:
                print("Set text sysex recognized")
                # Parse the message S1 and S2. S1 is between 0x01 and 0x00, S2 is between 0x02 and 0xF7
                # Find the index of the first 0x01 after the header
                index = bytes.index(0x01)
                # Find the index of the first 0x00 after the 0x01
                index2 = bytes.index(0x00, index)
                # S1 is whatever is between index and index2
                S1 = bytes[index+1:index2]
                # Find the index of the first 0x02 after index2
                index3 = bytes.index(0x02, index2)
                index4 = bytes.index(0xF7, index3)
                S2 = bytes[index3+1:index4]
                # Convert the bytes to a string
                S1_string = ''.join([chr(b) for b in S1])
                S2_string = ''.join([chr(b) for b in S2])
                print("S1 string:", S1_string)
                print("S2 string:", S2_string)
                lcd.clear()
                lcd.putstr(S1_string + "\n" + S2_string)
            # 01 - Read value
            # F0 00 20 6B 7F 42 01 00 pp bb
            # pp = parameter number
            # bb = button id
            if bytes[:8] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x01, 0x00]:
                pp = bytes[8]
                bb = bytes[9]
                print(f"Read value; parameter number: {pp}, button id: {bb}")
            # 02 - Write value
            # F0 00 20 6B 7F 42 02 00 pp bb vv F7
            # pp = parameter number
            # bb = button id
            # vv = value
            if bytes[:8] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x02, 0x00]:
                pp = bytes[8]
                bb = bytes[9]
                vv = bytes[10]
                print(f"Write value; parameter number: {pp}, button id: {bb}, value: {vv}")
                # Just for testing, send a sysex message back with value 0x01; FIXME: AnalogLab does not seem to adjust the on-screen controls accordingly
                # Maybe different messages are needed to be sent back to AnalogLab?s
                midi.send(SystemExclusive([0xF0, 0x00, 0x20], [0x6B, 0x7F, 0x42, 0x02, 0x00, pp, bb, 0x01]))


# F0 7E 00 06 02 00 20 6B 02 00 05 74 F7
