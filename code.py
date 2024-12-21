import board
import rotaryio
import busio
import usb_midi
import adafruit_midi
import digitalio
import time

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

from boot import product

debugging_on = False

mode = "arturia" # Arturia mode, e.g., for AnalogLab
# Other modes are "daw" and "mcu" (Mackie Control Universal); these are selected by pressing the buttons on the controller
# at startup
# TODO: Store in a file which mode was selected last time and start in that mode

# QUESTION: How does the controller know which names the knobs and faders have? Is this information sent from the DAW to the controller?
# Or does the controller just get the CC number and has to look up the name in a table, depending on the selected instrument?
# If the latter is the case, then this would mean that the controller firmware needs to be updated every time a new instrument is released.
# This does not seem to be the case. So how does it work?

print(product)

"""
RPi Pico      <-> Peripherals  
Pin 1  (GP0)  <-> Display SDA
Pin 2  (GP1)  <-> Display SCL
Pin 3  (GND)  <-> Display GND
Pin 4  (GP2)  <-> Button 0: Category
Pin 5  (GP3)  <-> Button 1: Preset
Pin 6  (GP4)  <-> Button 2: <-
Pin 7  (GP5)  <-> Button 3: ->
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

# Scan for I2C devices and use the first one found for the display
devices = i2c.scan()
print("I2C devices found:", [hex(device) for device in devices])
try:
    lcd = I2cLcd(i2c, devices[0], 2, 16)
except:
    lcd = None

lcd.clear()
lcd.backlight = True

# Print the available ports
print("Available MIDI ports:", usb_midi.ports)
# NOTE: If in_buf_size is too small, then MIDIUnknownEvent is received instead of the actual message;
# in this case,  need to increase in_buf_size further
midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1], in_channel=0, out_channel=0, in_buf_size=512, debug=debugging_on)
print("MIDI input port:", usb_midi.ports[0])
print("MIDI input channel:", midi.in_channel)
print("MIDI output port:", usb_midi.ports[1])
print("MIDI output channel:", midi.out_channel)

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
#####################################################

print("Checking for button presses...")
for button in buttons:
    print("Button", button, "pressed:", button.value)
# If the encoder button is pressed, then enter MCU mode
# Note the button is active low, so we check for False
if buttons[4].value == False:
    mode = "mcu"
    print("MCU mode enabled")
    lcd.clear()
    lcd.putstr("MCU mode enabled")
# If button 0 is pressed, then enter DAW mode
if buttons[0].value == False:
    mode = "daw"
    print("DAW mode enabled")
    lcd.clear()
    lcd.putstr("DAW mode enabled")

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
        # If the new position is greater than the last position, then the encoder was turned clockwise
        if last_position is not None and position > last_position:
            print("Clockwise")
            if mode == "daw":
                midi.send(ControlChange(28, 64))
                midi.send(ControlChange(28, 65))
            else:
                if mode == "mcu":
                    midi.send(ControlChange(0x3C , 1))
                elif led.value == False:
                    midi.send(ControlChange(114, 64))
                    midi.send(ControlChange(114, 65))
                else:
                    midi.send(ControlChange(112, 64))
                    midi.send(ControlChange(112, 65))
        # If the new position is less than the last position, then the encoder was turned counterclockwise
        elif last_position is not None and position < last_position:
            print("Counterclockwise")
            if mode == "daw":
                midi.send(ControlChange(28, 64))
                midi.send(ControlChange(28, 63))
            elif mode == "mcu":
                midi.send(ControlChange(0x3C , 127))
            else:
                if led.value == False:
                    midi.send(ControlChange(114, 64))
                    midi.send(ControlChange(114, 63))
                else:
                    midi.send(ControlChange(112, 64))
                    midi.send(ControlChange(112, 63))

    last_position = position

    # Check for button presses, debounce them, and trigger the corresponding actions
    for i, button in enumerate(buttons):
        if not button.value and not buttons_pressed[i]:
            buttons_pressed[i] = True
            time.sleep(debounce_time)
            print(f"Button {i} pressed")
            if i == 0:
                # "Category" button
                if mode == "daw":
                    pass
                elif mode == "mcu":
                    # https://github.com/bitwig/bitwig-extensions/blob/da7d70e73cc055475d63ac6c7de17e69f89f4993/src/main/java/com/bitwig/extensions/controllers/arturia/keylab/essential/ArturiaKeylabEssentialControllerExtension.java#L355
                    midi.send(NoteOn(0x65, 127))
                    midi.send(NoteOff(0x65))
                    led.value = True
                else:
                    if led.value == False:
                        # We are not in the menu
                        midi.send(ControlChange(116, 127))
                        led.value = True
                    else:
                        # We are in the menu
                        # QUESTION: What should actually happen when we are already in the menu and press the "Category" button?
                        pass
            elif i == 1:
                # "Preset" button
                if mode == "daw":
                    pass
                elif mode == "mcu":
                    # https://github.com/bitwig/bitwig-extensions/blob/da7d70e73cc055475d63ac6c7de17e69f89f4993/src/main/java/com/bitwig/extensions/controllers/arturia/keylab/essential/ArturiaKeylabEssentialControllerExtension.java#L366
                    midi.send(NoteOn(0x64, 127))
                    midi.send(NoteOff(0x64))
                    led.value = False
                else:
                    # QUESTION: According to https://www.youtube.com/watch?v=ipnTPsDN3t4 3:33, the "Preset" button 
                    # may not always go into Preset mode, as it is also used to "select a song from the playlist"
                    midi.send(ControlChange(117, 127))
                    led.value = False
            elif i == 2:
                # "<-" button
                # Previous preset
                if mode == "daw":
                    pass
                elif mode == "mcu":
                    # https://github.com/bitwig/bitwig-extensions/blob/da7d70e73cc055475d63ac6c7de17e69f89f4993/src/main/java/com/bitwig/extensions/controllers/arturia/keylab/essential/ArturiaKeylabEssentialControllerExtension.java#L323
                    midi.send(NoteOn(0x62, 127))
                    midi.send(NoteOff(0x62))
                else:
                    midi.send(ControlChange(28, 127))
            elif i == 3:
                # "->"" button
                # Next preset
                if mode == "daw":
                    pass
                elif mode == "mcu":
                    # https://github.com/bitwig/bitwig-extensions/blob/da7d70e73cc055475d63ac6c7de17e69f89f4993/src/main/java/com/bitwig/extensions/controllers/arturia/keylab/essential/ArturiaKeylabEssentialControllerExtension.java#L339
                    midi.send(NoteOn(0x63, 127))
                else:
                    midi.send(ControlChange(29, 127))
            elif i == 4:
                # Encoder OK/Enter
                if mode == "daw":
                    # If buton 0 (used as Shift in DAW mode) is not pressed
                    if buttons[0].value:
                        midi.send(ControlChange(118, 127)) # Click
                    else:
                        midi.send(ControlChange(119, 127)) # Shft + Click
                elif mode == "mcu":
                    # "MIDI_NOTE_ON 0x54"
                    midi.send(NoteOn(0x54, 127))
                else:
                    if led.value == False:
                        # We are not in the menu
                        # NOTE: This also functions as "Like" when long-pressed; hence we also need to send value 0 as soon as the button is released
                        midi.send(ControlChange(115, 127))
                    else:
                        # We are in the menu
                        midi.send(ControlChange(113, 127))
        
        elif button.value and buttons_pressed[i]:
            time.sleep(debounce_time)
            if button.value:
                buttons_pressed[i] = False
                print(f"Button {i} released")
                if i == 0:
                    if mode == "daw":
                        pass
                    else:
                        midi.send(ControlChange(117, 0))
                elif i == 1:
                    if mode == "daw":
                        pass
                    else:
                        midi.send(ControlChange(28, 0))
                elif i == 2:
                    if mode == "daw":
                        pass
                    else:
                        midi.send(ControlChange(29, 0))
                elif i == 3:
                    if mode == "daw":
                        pass
                    else:
                        midi.send(ControlChange(116, 0))
                elif i == 4:
                    if mode == "daw":
                        midi.send(ControlChange(118, 0)) # To end the "Click" action
                        midi.send(ControlChange(119, 0)) # To end the "Shft + Click" action
                    elif mode == "mcu":
                        midi.send(NoteOff(0x54))
                    else:
                        if led.value == False:
                            midi.send(ControlChange(115, 0))
                        else:
                            midi.send(ControlChange(113, 0))

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

    if isinstance(message, MIDIUnknownEvent) and debugging_on == False:
        continue
    
    if message is not None:
        print("Message:", message)

        # If MIDIUnknownEvent, then print a message explaining how to debug
        if isinstance(message, MIDIUnknownEvent):
            print("MIDIUnknownEvent received")
            print("See the contents of the message by setting debug=True in the adafruit_midi.MIDI object")
            print("Possibly in_buf_size needs to be further increased")

        # If bytes 90 32 00, then print a message on the display
        if bytes == [0x90, 0x32, 0x00]:
            lcd.clear()
            lcd.putstr("30 92 00, why?")

        if debugging_on:
            # If not MIDIUnknownEvent, then print the bytes on the display
            if not isinstance(message, MIDIUnknownEvent):
                lcd.clear()
                lcd.putstr(' '.join([f"{b:02X}" for b in bytes]))

        # If the string "MiniDexed" is in the received bytes, then switch to DAW mode
        if not mode == "daw" and "MiniDexed" in ''.join([chr(b) for b in bytes]):
            mode = "daw"
            print("DAW mode enabled")
            lcd.clear()
            lcd.putstr("DAW mode enabled")

        # "Universal Device Request" message
        if bytes == [0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7]:
            print("Request for device ID")
            for i in range(0, 20):
                print("########################################################")
                # Apparently AnalogLab does not send this, but we might want to support it
                # e.g., for MiniDexed to find out which device it is connected to

            """
            
            # When "Mackie Control" is selected in REAPER under "Control/OSC/Web", REAPER sends the following message when exiting:
            # [f0 00 00 66 14 08 00 f7]
            # This is from the "Mackie Control Universal" (MCU) protocol

            sysex inquiry 0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7.
            The Keylab Essential 61 responds with 0xF0, 0x7E, 0x7F, 0x06, 0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x54, 0xAA, 0xBB, 0xCC, 0xDD, 0xF7 (AA BB CC DD is the firmware version)
            https://docs.rs/midi-control/latest/midi_control/vendor/arturia/index.html
            Then it sets the DAW mode into mackie with 0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x02, 0x00, 0x40, 0x51, 0x00, 0xF7"""

            # Respond with a matching device ID
            # NOTE: The first and last bytes are the sysex header and footer and must not be included as they are added automatically by the MIDI library
            # The last 4 bytes are the firmware version; how exactly to convert from XX.YY.ZZZZ to 0xXX 0xYY 0xZZ 0xZZ?
            if product == "Minilab3":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 49":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x52, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 61":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x54, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 88":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x58, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab mkII 49":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x62, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab mkII 61":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x64, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab mkII 88":
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x68, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 49 mk3": # FIXME: This string is an unconfirmed guess
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x72, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 61 mk3": # FIXME: This string is an unconfirmed guess
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x74, 0x01, 0x01, 0x01, 0x01]))
            elif product == "Arturia KeyLab Essential 88 mk3": # FIXME: This string is an unconfirmed guess
                midi.send(SystemExclusive([0x7E, 0x7F, 0x06], [0x02, 0x00, 0x20, 0x6B, 0x02, 0x00, 0x05, 0x78, 0x01, 0x01, 0x01, 0x01]))
            else:
                print("FIXME: Respond with the correct device ID for", product)
                lcd.clear()
                lcd.putstr("FIXME: device ID")
                lcd.move_to(0, 1)
                lcd.putstr("for " + product)
            # Set the DAW mode into Mackie???
            midi.send(SystemExclusive([0x00, 0x20, 0x6B], [0x7F, 0x42, 0x02, 0x00, 0x40, 0x51, 0x00]))

        # If sysex, then check if it starts with the expected header
        if isinstance(message, SystemExclusive):
            if bytes[:6] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42]:
                print("Arturia sysex recognized")

            """
            Sysex message format used by Arturia KeyLab Essential 61 with AnalogLab to write to the display:
            F0               # sysex header
            00 20 6B 7F 42   # Arturia header
            04 ?? 60         # set text (?? can be 00 for KeyLab Essential or 02 for Minilab3 and possibly other values)
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

            # Hence checking only the first 7 bytes for now and the 9th byte
            if bytes[:7] == [0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04] and bytes[8] == 0x60:
                print("Set text sysex recognized")
                #try:
                # Find the indices of the bytes
                first_0x01 = bytes.index(0x01)
                first_0x00_after_0x01 = bytes.index(0x00, first_0x01)
                try:
                    first_0x02_after_0x00 = bytes.index(0x02, first_0x00_after_0x01)
                    first_0x00_after_0x02 = bytes.index(0x00, first_0x02_after_0x00)
                except:
                    first_0x02_after_0x00 = None
                    first_0x00_after_0x02 = None
                try:
                    first_0x03_after_0x00 = bytes.index(0x03, first_0x00_after_0x02)
                    first_0x00_after_0x03 = bytes.index(0x00, first_0x03_after_0x00)
                except:
                    first_0x03_after_0x00 = None
                    first_0x00_after_0x03 = None
                try:
                    first_0x04_after_0x00 = bytes.index(0x04, first_0x00_after_0x03)
                    first_0x00_after_0x04 = bytes.index(0x00, first_0x04_after_0x00)
                except:
                    first_0x04_after_0x00 = None
                    first_0x00_after_0x04 = None
                # Extract the strings
                S1 = bytes[first_0x01 + 1:first_0x00_after_0x01]
                if first_0x02_after_0x00 is not None:
                    S2 = bytes[first_0x02_after_0x00 + 1:first_0x00_after_0x02]
                else:
                    S2 = None
                if first_0x03_after_0x00 is not None:
                    S3 = bytes[first_0x03_after_0x00 + 1:first_0x00_after_0x03]
                else:
                    S3 = None
                if first_0x04_after_0x00 is not None:
                    S4 = bytes[first_0x04_after_0x00 + 1:first_0x00_after_0x04]
                else:
                    S4 = None
                # Convert the bytes to a string
                S1_string = ''.join([chr(b) for b in S1])
                if S2 is not None:
                    S2_string = ''.join([chr(b) for b in S2])
                else:
                    S2_string = None
                if S3 is not None:
                    S3_string = ''.join([chr(b) for b in S3])
                else:
                    S3_string = None
                if S4 is not None:
                    S4_string = ''.join([chr(b) for b in S4])
                else:
                    S4_string = None
                #print("Instrument:", S1_string)
                #print("Name:", S2_string)
                #print("Type:", S3_string)
                # If the bytes are 46 20, then it is a heart
                if S4 == [0x46, 0x20]:
                    print("Heart")
                    # Replace the "*" ASCII character with a heart symbol
                    heart = bytearray([0x00,0x0a,0x1f,0x1f,0x0e,0x04,0x00,0x00])
                    lcd.custom_char(0, heart)
                    S2_string = S2_string.replace('*', chr(0))
                else:
                    print("No heart")
                # If we are emulating Minilab3, then we need to remove extraneous spaces to win ideally 2 characters in each line
                # We check if there are multiple spaces adjacent to each other.
                # If there are more than 2 spaces, then we remove 2 of them. If there is only more than 1 space, then we remove 1 of them.
                if product == "Minilab3":
                    if S1_string.count('   ') > 0:
                        # Find the offset of the first occurrence of 3 spaces
                        offset = S1_string.find('   ')
                        # Remove 2 spaces
                        S1_string = S1_string[:offset + 1] + S1_string[offset + 3:]
                    elif S1_string.count('  ') > 0:
                        # Find the offset of the first occurrence of 2 spaces
                        offset = S1_string.find('  ')
                        # Remove 1 space
                        S1_string = S1_string[:offset + 1] + S1_string[offset + 2:]
                    if S2_string is not None:
                        if S2_string.count('   ') > 0:
                            # Find the offset of the first occurrence of 3 spaces
                            offset = S2_string.find('   ')
                            # Remove 2 spaces
                            S2_string = S2_string[:offset + 1] + S2_string[offset + 3:]
                        elif S2_string.count('  ') > 0:
                            # Find the offset of the first occurrence of 2 spaces
                            offset = S2_string.find('  ')
                            # Remove 1 space
                            S2_string = S2_string[:offset + 1] + S2_string[offset + 2:]
                lcd.clear()
                lcd.move_to(0, 0)
                lcd.putstr(S1_string)
                lcd.move_to(0, 1)
                if S2_string is not None:
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
                    # AnalogLab is closing
                    lcd.clear()
                    lcd.putstr("Bye AnalogLab")
                    continue

                print(f"Write value; parameter number: {pp}, button id: {bb}, value: {vv}")
                # Just for testing, send a sysex message back with value 0x01; FIXME: AnalogLab does not seem to adjust the on-screen controls accordingly
                # Maybe different messages are needed to be sent back to AnalogLab?s
                # midi.send(SystemExclusive([0xF0, 0x00, 0x20], [0x6B, 0x7F, 0x42, 0x02, 0x00, pp, bb, 0x01]))

            if bytes == [0xF0, 0x00, 0x00, 0x66, 0x14, 0x08, 0x00, 0xF7]:
                print("Bye Mackie Control Universal mode")
                lcd.clear()
                lcd.putstr("Bye MCU mode")
