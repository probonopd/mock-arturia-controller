# NOTE: For this to take effect
# 1. You need to power cycle the device
# 2. You need to restart AnalogLab
# 3. You need to select a MIDI port with a well-known name in AnalogLab
# 4. You need to select a MIDI controller in AnalogLab that belongs to the device selected as the MIDI port

import supervisor
import usb_midi
import usb_hid
import storage
import digitalio
import board

# According to https://www.youtube.com/watch?v=ipnTPsDN3t4, the MIDI port is called "Keylab mkII 61 MIDI"
# USB vendor ID, USB product ID, USB manufacturer string, USB product string, MIDI interface name (that shows up in AnalogLab)
emulated_protocols = [(0x1c75, 0x028a, "Arturia", "Arturia KeyLab Essential 61", "Arturia KeyLab Essential 61"), # Works
                      (0x1c75, 0x028b, "Arturia", "KeyLab mkII 61", "KeyLab mkII 61 MIDI"), # Works
                      (0x1c75, 0x0285, "Arturia", "KeyLab 61", "KeyLab 61")] # Works but seems to have DIFFERENT MIDI CC mappings than the KeyLab mkII 61

# NOTE: "Arturia KeyLab Essential 61 MID" does NOT work with AnalogLab standalone!

which_protocol = 0

buttons = [digitalio.DigitalInOut(pin) for pin in (board.GP2, board.GP3, board.GP4, board.GP5, board.GP8)]
for button in buttons:
    button.switch_to_input(pull=digitalio.Pull.UP)
# If buton 2 ("<-") is pressed at boot time, then expose the USB mass storage device to the host computer,
# allowing for the Python code to be modified
if buttons[2].value == True:
    # Prevent USB mass storage from being mounted on RPi Pico
    storage.disable_usb_drive()
# If button 3 ("->") is pressed at boot time, then go into bootloader mode, 
# allowing for CircuitPython to be reinstalled or other firmware to be uploaded
# without access to the BOOTSEL button on the RPi Pico
if buttons[3].value == False:
    import microcontroller
    microcontroller.on_next_reset(microcontroller.RunMode.BOOTLOADER)
    microcontroller.reset()

usb_hid.disable()
supervisor.set_usb_identification(manufacturer=emulated_protocols[which_protocol][2], product=emulated_protocols[which_protocol][3], vid=emulated_protocols[which_protocol][0], pid=emulated_protocols[which_protocol][1])
usb_midi.set_names(streaming_interface_name=emulated_protocols[which_protocol][4], audio_control_interface_name=emulated_protocols[which_protocol][4])
usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")
