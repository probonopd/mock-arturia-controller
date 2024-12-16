import board
import rotaryio
import busio
import usb_midi
import adafruit_midi
import digitalio
import time
import sys
import select

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
from adafruit_midi.midi_message import MIDIMessage, MIDIUnknownEvent

# QUESTION: How does the controller know which names the knobs and faders have? Is this information sent from the DAW to the controller?
# Or does the controller just get the CC number and has to look up the name in a table, depending on the selected instrument?
# If the latter is the case, then this would mean that the controller firmware needs to be updated every time a new instrument is released.
# This does not seem to be the case. So how does it work?

"""
RPi Pico      <-> Peripherals  
Pin 1  (GP0)  <-> Display SDA
Pin 2  (GP1)  <-> Display SCL
Pin 3  (GND)  <-> Display GND
Pin 4  (GP2)  <-> Button 0
Pin 5  (GP3)  <-> Button 1
Pin 6  (GP4)  <-> Button 2
Pin 7  (GP5)  <-> Button 3
Pin 8  (GND)  <-> Button GND
Pin 9  (GP6)  <-> Rotary Encoder CLK
Pin 10 (GP7)  <-> Rotary Encoder DT
Pin 11 (GP8)  <-> Rotary Encoder SW
Pin 12 (GP9)  <-> Rotary Encoder +
Pin 13 (GND)  <-> Rotary Encoder GND
Pin 40 (VBUS) <-> Display VCC
# """

# Built-in LED
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
led.value = False

# Enable pull-up resistors for the buttons
buttons = [digitalio.DigitalInOut(pin) for pin in (board.GP2, board.GP3, board.GP4, board.GP5, board.GP8)]
for button in buttons:
    button.switch_to_input(pull=digitalio.Pull.UP)

# Set up rotary encoder
encoder = rotaryio.IncrementalEncoder(board.GP6, board.GP7)
last_position = None
# Switch on + pin of rotary encoder
switch = digitalio.DigitalInOut(board.GP9)
switch.switch_to_input(pull=digitalio.Pull.UP)

# Initialize and lock the I2C bus
i2c = busio.I2C(board.GP1, board.GP0)
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
lcd.backlight = True

# Print the available ports
print("Available MIDI ports:", usb_midi.ports)
# NOTE: If in_buf_size is too small, then MIDIUnknownEvent is received instead of the actual message;
# in this case,  need to increase in_buf_size further
midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1], in_channel=0, out_channel=0, in_buf_size=512, debug=True)
print("MIDI input port:", usb_midi.ports[0])
print("MIDI input channel:", midi.in_channel)
print("MIDI output port:", usb_midi.ports[1])
print("MIDI output channel:", midi.out_channel)

"""
Sysex message format used by Arturia KeyLab Essential 61 with AnalogLab to write to the display:
F0               # sysex header
00 20 6B 7F 42   # Arturia header
04 00 60         # set text
01 S1 00         # S1 = Instrument (e.g. 'ARP 2600')
02 S2 00         # S2 = Name (e.g. 'Bloody Swing')
03 S3 00         # S3 = Type (e.g. 'Noise')
04 S4 00         # S4 = Whether to display a heart (if 46 20, then display a heart; if nonexistent, then do not display a heart) - OPTIONAL
F7               # sysex footer

Example with heart:
F0 00 20 6B 7F 42 04 00 60 01 41 52 50 20 32 36 30 30 00 02 2A 42 6C 6F 6F 64 79 20 53 77 69 6E 67 00 03 4E 6F 69 73 65 00 04 46 20 00 F7
Example without heart:
F0 00 20 6B 7F 42 04 00 60 01 41 52 50 20 32 36 30 30 00 02 2A 42 6C 6F 6F 64 79 20 53 77 69 6E 67 00 03 4E 6F 69 73 65 00 04 00 F7
"""
buttons_pressed = [False, False, False, False, False]

debounce_time = 0.05  # 50 ms debounce time

#####################################################
# This block is just for testing purposes, shall be removed later
index = 0
# 20...31, 52...63, 85...87, 89...90
# potential_cc = [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 85, 86, 87, 89, 90]
# 110...117
potential_cc = [110, 111, 112, 113, 114, 115, 116, 117]
potential_values = [63, 64]
# Make a list of all combinations of potential CCs and values
combinations = [(cc, value) for cc in potential_cc for value in potential_values]
# 28 63 shows menu
# 28 64 shows menu
# 29 63 shows next menu
# 29 64 shows next menu
# 30 63 shows prev menu
# 30 64 nothing
# 85 64 shows "Unassigned" for a while
# 112 63 shwos menu
# 112 64 nothing
# 113 63 shows menu
# 113 64 shows liked, and goes out of menu
# Somewhere there the magic happens: we go into the submenu of e.g., Instruments and can select the instrument there
# 116 63 nothing
#####################################################



while True:
    # Check for Serial commands without blocking
    # This can be useful for testing purposes during development
    # BUT THIS MAKES THE CODE VERY SLOW BECAUSE IT WAITS FOR INPUT
    """if select.select([sys.stdin], [], [], 0.1)[0]:
        data = input()
        # Check if we have received a message from the Serial port containing the CC and value
        # which are separated by a space and each could be binary or hexadecimal
        # Example: "20 63" or "0x20 0x63"
        try:
            cc, value = data.split()
            # Convert the strings to integers
            cc = int(cc, 0)
            value = int(value, 0)
            # Send the CC message
            midi.send(ControlChange(cc, value))
        except:
            pass
        if data == "category" or data == "C":
            midi.send(ControlChange(116, 64))
            led.value = True
        if data == "preset" or data == "P":
            midi.send(ControlChange(117, 64))
            led.value = False
        if data == "next" or data == "n":
            midi.send(ControlChange(29, 1))
        if data == "previous" or data == "p":
            midi.send(ControlChange(28, 1))"""

    # Handle rotary encoder
    position = encoder.position
    if last_position is None or position != last_position:
        print(position)
        print(buttons[4].value)
        # If the new position is greater than the last position, then the encoder was turned clockwise
        if last_position is not None and position > last_position:
            print("Clockwise")
            if led.value == False:
                # midi.send(ControlChange(114, 64))
                midi.send(ControlChange(114, 65))
            else:
                # midi.send(ControlChange(112, 64))
                midi.send(ControlChange(112, 65))
        # If the new position is less than the last position, then the encoder was turned counterclockwise
        elif last_position is not None and position < last_position:
            print("Counterclockwise")
            if led.value == False:
                # midi.send(ControlChange(114, 64))
                midi.send(ControlChange(114, 63))
            else:
                # midi.send(ControlChange(112, 64))
                midi.send(ControlChange(112, 63))

    last_position = position

    # Check for button presses, debounce them, and trigger the corresponding actions
    for i, button in enumerate(buttons):
        if not button.value and not buttons_pressed[i]:
            buttons_pressed[i] = True
            time.sleep(debounce_time)
            print(f"Button {i} pressed")
            
            if i == 0:

                
                """# Send one CC per click
                cc, value = combinations[index]
                print(f"************* Sending CC {cc} with value {value}")
                midi.send(ControlChange(cc, value))
                # LCD display
                lcd.clear()
                lcd.move_to(0, 0)
                lcd.putstr(f"CC {cc}")
                lcd.move_to(0, 1)
                lcd.putstr(f"Value {value}")
                index = index + 1
                if index >= len(combinations):
                    index = 0"""

                # "Preset" button
                # QUESTION: According to https://www.youtube.com/watch?v=ipnTPsDN3t4 3:33, the "Preset" button 
                # may not always go into Preset mode, as it is also used to "select a song from the playlist"
                midi.send(ControlChange(117, 127))
                led.value = False

            elif i == 1:
                # "<-" button
                # Previous preset
                midi.send(ControlChange(28, 127))
            elif i == 2:
                # "->"" button
                # Next preset
                midi.send(ControlChange(29, 127))
            elif i == 3:
                # "Category" button
                if led.value == False:
                    # We are not in the menu
                    midi.send(ControlChange(116, 127))
                    led.value = True
                else:
                    # We are in the menu
                    # QUESTION: What should actually happen when we are already in the menu and press the "Category" button?
                    pass
            elif i == 4:
                # Encoder OK/Enter
                if led.value == False:
                    # We are not in the menu
                    midi.send(ControlChange(115, 127))
                else:
                    # We are in the menu
                    midi.send(ControlChange(113, 127))
                
                midi.send(ControlChange(115, 1))
        elif button.value and buttons_pressed[i]:
            time.sleep(debounce_time)
            if button.value:
                buttons_pressed[i] = False
                print(f"Button {i} released")
                if i == 0:
                    midi.send(ControlChange(117, 0))
                elif i == 1:
                    midi.send(ControlChange(28, 0))
                elif i == 2:
                    midi.send(ControlChange(29, 0))
                elif i == 3:
                    midi.send(ControlChange(116, 0))
                elif i == 4:
                    pass # midi.send(ControlChange(113, 0))
        
    """
    Arturia CC messages
    # TODO: Find out which messages an Arturia controller sends when the buttons are pressed: left, right, up, down, select/enter, back, etc.

    CC 28 value 0:   Nothing
    CC 28 value 1...127: Next preset with selecting it (alphabetical)

    CC 29 value 0:   Nothing
    CC 29 value 1...127: Previous preset with selecting it (alphabetical)

    CC 100 to CC 107: Results in no messages being sent from AnalogLab to the controller

    CC 109 value 127: Nothing?
    
    CC 110 value 127: ?

    CC 111 value 127: ?

    CC 112 value 0:   Nothing
    CC 112 value 1:   Prev preset without selecting it
    CC 112 value 127: Next preset without selecting it

    CC 113 value 0...63:   OK/Enter
    CC 113 value 64...127: Like/unlike

    CC 114 value 0:   Nothing
    CC 114 value 1:   Prev preset without selecting it
    CC 114 value 127: Next preset without selecting it

    CC 115 value 0...63:   OK/Enter
    CC 115 value 64...127: Like/unlike toggle

    CC 116 value 0: Does nothing
    CC 116 value 1...63: Goes out of the menu
    CC 116 value 64...127: Goes into the Instruments menu, from there can go to Types, My Favorites, Sound Banks using CC 28 and CC 29
    
    CC 117 value 0: Does nothing
    CC 116 value 1...63: ?
    CC 116 value 64...127: Goes out of the menu

    CC 118 value 0...127 causes a lot of messages to be send from AnalogLab to the controller
    
    https://forum.arturia.com/t/sysex-protocol-documentation/5746/3
    ENC_CLICK = 118, ENC_SHIFT_CLICK = 119
    """

    # Check for incoming MIDI messages
    message = midi.receive()
    
    try:
        bytes = list(message.__bytes__())
        print("\r\nReceived:", ' '.join([f"{b:02X}" for b in bytes]))
        # lcd.clear()
        # lcd.putstr(''.join([f"{b:02X}" for b in bytes]))
        # print("--> https://www.google.com/search?q=%22" + '+'.join([f"{b:02X}" for b in bytes]) + "%22")
    except:
        pass

    if message is not None:
        print("Message:", message)

        # If MIDIUnknownEvent, then print a message explaining how to debug
        if isinstance(message, MIDIUnknownEvent):
            print("MIDIUnknownEvent received")
            print("See the contents of the message by setting debug=True in the adafruit_midi.MIDI object")
            print("Most likely in_buf_size needs to be further increased")

        # "Universal Device Request" message
        if bytes == [0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7]:
            print("Request for device ID")
            for i in range(0, 20):
                print("########################################################")
                # Apparently AnalogLab does not send this, but we might want to support it
                # e.g., for MiniDexed to find out which device it is connected to
        
        # If sysex, then check if it starts with the expected header
        if isinstance(message, SystemExclusive):
            if bytes[:6] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42]:
                print("Arturia sysex recognized")
            if bytes[:9] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60]:
                print("Set text sysex recognized")
                #try:
                # Find the indices of the bytes
                first_0x01 = bytes.index(0x01)
                first_0x00_after_0x01 = bytes.index(0x00, first_0x01)
                first_0x02_after_0x00 = bytes.index(0x02, first_0x00_after_0x01)
                first_0x00_after_0x02 = bytes.index(0x00, first_0x02_after_0x00)
                first_0x03_after_0x00 = bytes.index(0x03, first_0x00_after_0x02)
                first_0x00_after_0x03 = bytes.index(0x00, first_0x03_after_0x00)
                try:
                    first_0x04_after_0x00 = bytes.index(0x04, first_0x00_after_0x03)
                    first_0x00_after_0x04 = bytes.index(0x00, first_0x04_after_0x00)
                except:
                    first_0x04_after_0x00 = None
                    first_0x00_after_0x04 = None
                # Extract the strings
                S1 = bytes[first_0x01 + 1:first_0x00_after_0x01]
                S2 = bytes[first_0x02_after_0x00 + 1:first_0x00_after_0x02]
                S3 = bytes[first_0x03_after_0x00 + 1:first_0x00_after_0x03]
                if first_0x04_after_0x00 is not None:
                    S4 = bytes[first_0x04_after_0x00 + 1:first_0x00_after_0x04]
                else:
                    S4 = None
                # Convert the bytes to a string
                S1_string = ''.join([chr(b) for b in S1])
                S2_string = ''.join([chr(b) for b in S2])
                S3_string = ''.join([chr(b) for b in S3])
                if S4 is not None:
                    S4_string = ''.join([chr(b) for b in S4])
                else:
                    S4_string = None
                print("Instrument:", S1_string)
                print("Name:", S2_string)
                print("Type:", S3_string)
                # If the bytes are 46 20, then it is a heart
                if S4 == [0x46, 0x20]:
                    print("Heart")
                    # Replace the "*" ASCII character with a heart symbol
                    heart = bytearray([0x00,0x0a,0x1f,0x1f,0x0e,0x04,0x00,0x00])
                    lcd.custom_char(0, heart)
                    S2_string = S2_string.replace('*', chr(0))
                else:
                    print("No heart")
                lcd.clear()
                lcd.move_to(0, 0)
                lcd.putstr(S1_string)
                lcd.move_to(0, 1)
                lcd.putstr(S2_string)
                #except:
                #    print("Error processing sysex message")
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

                if bb == 89:
                    # LCD "bye"
                    lcd.clear()
                    lcd.move_to(0, 0)
                    lcd.putstr("bye")
                    continue

                print(f"Write value; parameter number: {pp}, button id: {bb}, value: {vv}")
                # Just for testing, send a sysex message back with value 0x01; FIXME: AnalogLab does not seem to adjust the on-screen controls accordingly
                # Maybe different messages are needed to be sent back to AnalogLab?s
                # midi.send(SystemExclusive([0xF0, 0x00, 0x20], [0x6B, 0x7F, 0x42, 0x02, 0x00, pp, bb, 0x01]))

# When "Mackie Control" is selected in REAPER under "Control/OSC/Web", REAPER sends the following message when exiting:
# [f0 00 00 66 14 08 00 f7]
# This is from the "Mackie Control Universal" (MCU) protocol
# It is currently unknown whether Arturia devices can use the screen via this protocol
