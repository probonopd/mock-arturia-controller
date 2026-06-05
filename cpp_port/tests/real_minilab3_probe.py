#!/usr/bin/env python3
"""Interactive probe for the attached real Minilab 3.

This script is meant to be run on the host computer that has the original
Minilab 3 attached. It:
  1. prints the real USB descriptor summary from lsusb,
  2. asks the user to confirm the display behavior after sending a few
     known Arturia display sysex messages,
  3. records the user-confirmed observations for later comparison with the
     emulator.
"""

from __future__ import annotations

import subprocess
import sys
from typing import Iterable

import mido


def run_lsusb() -> str:
    result = subprocess.run(
        ['lsusb', '-d', '1c75:020b', '-v'],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.stdout or result.stderr


def choose_output_port() -> str:
    ports = list(mido.get_output_names())
    if not ports:
        raise RuntimeError('No MIDI output ports are available. Connect the real Minilab 3 first.')

    preferred = next((name for name in ports if 'ALV' in name.upper()), None)
    if preferred is not None:
        print(f'Using the display/ALV port automatically: {preferred}')
        return preferred

    print('\nAvailable MIDI output ports:')
    for index, name in enumerate(ports, start=1):
        print(f'  {index}. {name}')

    while True:
        try:
            choice = input('\nSelect the Arturia output port [1]: ').strip() or '1'
            selected = ports[int(choice) - 1]
            return selected
        except (ValueError, IndexError):
            print('Please enter a valid port number.')


def send_sysex(port_name: str, packet: Iterable[int]) -> None:
    with mido.open_output(port_name) as out:
        out.send(mido.Message('sysex', data=list(packet)))


def ask_user(prompt: str) -> bool:
    while True:
        answer = input(prompt).strip().lower()
        if answer in {'y', 'yes'}:
            return True
        if answer in {'n', 'no'}:
            return False
        print('Please answer yes or no.')


def main() -> int:
    print('== Real Minilab 3 USB descriptor probe ==')
    descriptor_dump = run_lsusb()
    if not descriptor_dump:
        print('Could not read the USB descriptors. Is the Minilab 3 connected and visible to lsusb?')
        return 1

    for line in descriptor_dump.splitlines():
        if any(token in line for token in ('idVendor', 'idProduct', 'bcdDevice', 'iManufacturer', 'iProduct', 'iSerial', 'bNumConfigurations', 'Endpoint Descriptor', 'MIDIStreaming')):
            print(line)

    try:
        port_name = choose_output_port()
    except RuntimeError as exc:
        print(f'ERROR: {exc}')
        return 1

    standard_packet = [
        0x00, 0x20, 0x6B, 0x7F, 0x42, 0x04, 0x00, 0x60,
        0x01, 0x41, 0x52, 0x50, 0x20, 0x32, 0x36, 0x30, 0x30, 0x00,
        0x02, 0x2A, 0x42, 0x6C, 0x6F, 0x6F, 0x64, 0x79, 0x20, 0x53,
        0x77, 0x69, 0x6E, 0x67, 0x00, 0x03, 0x4E, 0x6F, 0x69, 0x73,
        0x65, 0x00, 0x04, 0x46, 0x20, 0x00,
    ]

    print('\nThe next sysex message is the verified display path to test on the attached Minilab 3.')
    print('  1. Standard Arturia display sysex with a heart marker (real Minilab 3 display text: “*Bloody Swing” / “Noise”).')
    print('Please look at the real Minilab 3 display and answer whether the text matches the expectation.')

    print('\nSending standard display sysex…')
    send_sysex(port_name, standard_packet)
    if ask_user('Did you see the display change to “*Bloody Swing” and “Noise” with the heart icon? [y/N] '):
        print('Standard display sysex: OK')
    else:
        print('Standard display sysex: did not match expectation')

    print('\nProbe complete. Compare the real-device observations with the emulator behavior in the C++ port.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
