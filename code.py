import board
import busio
import usb_midi
import adafruit_midi
import adafruit_character_lcd.character_lcd_i2c as character_lcd

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

# Set up the 16x2 LCD display
i2c = busio.I2C(scl=board.GP15, sda=board.GP14)

# Scan for I2C devices
# Function requires lock, so we do:
i2c.try_lock()
devices = i2c.scan()
i2c.unlock()
print("I2C devices found:", [hex(device) for device in devices])

# Try to find LCD at address 0x27 or 0x3f
lcd = None
for address in (0x27, 0x3f):
    try:
        # FIXME: Use mono display instead of RGB; how?
        lcd = character_lcd.Character_LCD_I2C(i2c, 16, 2, address=address)

        print("LCD found at address", hex(address))
        break
    except ValueError:
        print("No LCD found at address", hex(address))
# Throw error if no LCD found at either address
if lcd is None:
    raise ValueError("No LCD found!")

# Initialize the LCD
#lcd.clear()
#lcd.backlight = True
#lcd.cursor = False
#lcd.blink = True

# Write a message to the LCD, two lines
lcd.message = "Hello\nCircuitPython!"

# Print the available ports
print("Available MIDI ports:", usb_midi.ports)
midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], in_channel=0, debug=True)
print("MIDI input port:", usb_midi.ports[0])
print("MIDI input channel:", midi.in_channel)

# Define the expected header
expected_header = [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60, 0x01]

while True:
    # Check for incoming MIDI messages
    message = midi.receive()

    if message is not None:

        bytes = list(message.__bytes__())
        print("Received:", ' '.join([hex(b) for b in bytes]))
        
        # If sysex, then check if it starts with the expected header
        if isinstance(message, SystemExclusive):
            print("Sysex received:", ' '.join([hex(b) for b in bytes]))
            if bytes[:10] == expected_header:
                print("Sysex recognized!")
                # TODO: Parse the sysex message, get the data, and display it on the LCD
            else:
                print("Sysex not recognized!")
