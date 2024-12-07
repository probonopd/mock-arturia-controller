import usb_hid
import usb_midi
usb_hid.disable()
usb_midi.set_names(manufacturer="CircuitPython", product="CircuitPython MIDI")
usb_midi.enable()
print("enabled USB MIDI, disabled USB HID")
