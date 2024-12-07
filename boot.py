import supervisor
import usb_midi
import usb_hid

usb_hid.disable()

# Vendor: usb 0x1c75 "Arturia"
# Device: usb 0x028a "Arturia KeyLab Essential 61"
supervisor.set_usb_identification(manufacturer="Arturia", product="Arturia KeyLab Essential 61", vid=0x1c75, pid=0x028a)
usb_midi.set_names(manufacturer="Arturia", product="Arturia KeyLab Essential 61")

usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")
