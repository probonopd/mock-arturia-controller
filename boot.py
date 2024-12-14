# NOTE: For this to take effect
# 1. You need to power cycle the device
# 2. You need to restart AnalogLab
# 3. You need to select a MIDI port with a well-known name in AnalogLab
# 4. You need to select a MIDI controller in AnalogLab that belongs to the device selected as the MIDI port

import supervisor
import usb_midi
import usb_hid

# According to https://www.youtube.com/watch?v=ipnTPsDN3t4, the MIDI port is called "Keylab mkII 61 MIDI"
# USB vendor ID, USB product ID, USB manufacturer string, USB product string, MIDI interface name (that shows up in AnalogLab)
emulated_protocols = [(0x1c75, 0x028a, "Arturia", "Arturia KeyLab Essential 61", "Arturia KeyLab Essential 61"), # Works
                      (0x1c75, 0x028b, "Arturia", "KeyLab mkII 61", "KeyLab mkII 61 MIDI"), # Works
                      (0x1c75, 0x0285, "Arturia", "KeyLab 61", "KeyLab 61")] # Works but seems to have DIFFERENT MIDI CC mappings than the KeyLab mkII 61

which_protocol = 1

usb_hid.disable()
supervisor.set_usb_identification(manufacturer=emulated_protocols[which_protocol][2], product=emulated_protocols[which_protocol][3], vid=emulated_protocols[which_protocol][0], pid=emulated_protocols[which_protocol][1])
usb_midi.set_names(streaming_interface_name=emulated_protocols[which_protocol][4], audio_control_interface_name=emulated_protocols[which_protocol][4])
usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")
