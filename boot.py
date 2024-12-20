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
                      (0x1c75, 0x0285, "Arturia", "KeyLab 61", "KeyLab 61"), # Works but seems to have DIFFERENT MIDI CC mappings than the KeyLab mkII 61
                      (0x1c75, 0x220b, "Arturia", "Minilab3", "Minilab3 MIDI"), # NOTE: lower-case "l" in "Minilab"! "Minilab3 MIDI" is confirmed from https://youtu.be/Zcwdv4ZYipw?feature=shared&t=529 and the USB descriptor name form https://linux-hardware.org/?device_vendor=Arturia&device_type=sound
                      ] 

# NOTE: "Arturia KeyLab Essential 61 MID" does NOT work with AnalogLab standalone!

# NOTE: Minilab 3 has 3(!) MIDI cables: "Minilab3 MIDI", "Minilab3 DIN THRU", "Minilab3 MCU"
# as can be seen in https://youtu.be/Zcwdv4ZYipw?feature=shared&t=529

which_protocol = 3
product=emulated_protocols[which_protocol][3] # We use this in code.py

if __name__ == "__main__":

    buttons = [digitalio.DigitalInOut(pin) for pin in (board.GP2, board.GP3, board.GP4, board.GP5, board.GP8)]
    for button in buttons:
        button.switch_to_input(pull=digitalio.Pull.UP)
    # If buton 2 is pressed, then expose the USB mass storage device to the host computer
    if buttons[2].value == True:
        # Prevent USB mass storage from being mounted on RPi Pico
        storage.disable_usb_drive()
    # If button 3 is pressed, then go into bootloader mode, allowing for CircuitPython to be reinstalled
    # or other firmware to be uploaded without access to the BOOTSEL button on the RPi Pico
    if buttons[3].value == False:
        import microcontroller
        microcontroller.on_next_reset(microcontroller.RunMode.BOOTLOADER)
        microcontroller.reset()

    usb_hid.disable()
    supervisor.set_usb_identification(manufacturer=emulated_protocols[which_protocol][2], product=emulated_protocols[which_protocol][3], vid=emulated_protocols[which_protocol][0], pid=emulated_protocols[which_protocol][1])
    usb_midi.set_names(streaming_interface_name=emulated_protocols[which_protocol][4], audio_control_interface_name=emulated_protocols[which_protocol][4])
    usb_midi.enable()
    print("enabled USB MIDI, disabled USB HID")
    print("manufacturer: ", emulated_protocols[which_protocol][2])
    print("product: ", emulated_protocols[which_protocol][3])
    print("vid: ", emulated_protocols[which_protocol][0])
    print("pid: ", emulated_protocols[which_protocol][1])
    print("streaming_interface_name: ", emulated_protocols[which_protocol][4])
    print("audio_control_interface_name: ", emulated_protocols[which_protocol][4])
