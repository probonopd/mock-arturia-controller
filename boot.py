import supervisor
import usb_midi
import usb_hid

usb_hid.disable()

# Vendor: usb 0x1c75 "Arturia"
# Device: usb 0x028a "Arturia KeyLab Essential 61"
supervisor.set_usb_identification(manufacturer="Arturia", product="Arturia KeyLab Essential 61", vid=0x1c75, pid=0x028a)

# The following seems to be necessary so that Arturia AnalogLab sends MIDI messages to the device
# This determines the name of the MIDI port visible in Arturia AnalogLab
usb_midi.set_names(streaming_interface_name="Arturia KeyLab Essential 61", audio_control_interface_name="Arturia KeyLab Essential 61")
# With "Arturia KeyLab 61" it gets no messages from AnalogLab (standalone)
# With "Arturia KeyLab Essential 61" it gets messages but not for the display
# With "Arturia MiniLab 3" it gets no messages
# Does this need to match the USB product name?

usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")

# NOTE: Making changes here requires power cycling the device
