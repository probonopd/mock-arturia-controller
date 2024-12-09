import supervisor
import usb_midi
import usb_hid

usb_hid.disable()

# Vendor: usb 0x1c75 "Arturia"
# Device: usb 0x028a "KeyLab Essential 61"
supervisor.set_usb_identification(manufacturer="Arturia", product="Arturia KeyLab Essential 61", vid=0x1c75, pid=0x028a)

# Arturia KeyLab mkII 61
# USB ID 1c75:028b
# supervisor.set_usb_identification(manufacturer="Arturia", product="KeyLab mkII 61", vid=0x1c75, pid=0x028b)

# The following seems to be necessary so that Arturia AnalogLab sends MIDI messages to the device
# This determines the name of the MIDI port visible in Arturia AnalogLab
# usb_midi.set_names(streaming_interface_name="Arturia Keylab mkII 61", audio_control_interface_name="Arturia Keylab mkII 61")
# With "Arturia KeyLab 61" it gets no messages from AnalogLab (standalone) - TO BE RETESTED WITH PROPER USB IDs
# With "Arturia Keylab mkII (MIDI)" it gets no messages from AnalogLab (standalone) - TO BE RETESTED WITH PROPER USB IDs
# With "Arturia Keylab mkII DAW (MIDIIN2/MIDIOUT2)" does not even show up in AnalogLab (standalone) - ??? 
# With "Arturia KeyLab Essential 61" it gets messages but not for the display???
usb_midi.set_names(streaming_interface_name="Arturia KeyLab Essential 61", audio_control_interface_name="Arturia KeyLab Essential 61") # Known to work
# With "Arturia MiniLab 3" it gets no messages - TO BE RETESTED WITH PROPER USB IDs
# Does this need to match the USB product name?

usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")

# NOTE: Making changes here requires power cycling the device
