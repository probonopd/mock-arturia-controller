#include <cstddef>
#include <cstdint>
#include <cstring>

#include "bsp/board_api.h"
#include "tusb.h"

#include "src/midi_descriptor_helpers.h"
#include "src/model_config.h"

namespace {

#define _PID_MAP(itf, n)  ((CFG_TUD_##itf) << (n))
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_PORT_ALV,
  STRID_PORT_DIN_THRU,
  STRID_PORT_MCU_HUI,
  STRID_PORT_MIDI,
  STRID_SERIAL,
};

}  // namespace

//--------------------------------------------------------------------+
// Device descriptors
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = mock_arturia_controller::kModelConfig.vid,
  .idProduct          = mock_arturia_controller::kModelConfig.pid,
  .bcdDevice          = 0x1200,
  .iManufacturer      = STRID_MANUFACTURER,
  .iProduct           = STRID_PRODUCT,
  .iSerialNumber      = STRID_SERIAL,
  .bNumConfigurations = 0x01,
};

uint8_t const* tud_descriptor_device_cb(void) {
  return reinterpret_cast<uint8_t const*>(&desc_device);
}

//--------------------------------------------------------------------+
// Configuration descriptors
//--------------------------------------------------------------------+

enum {
#if defined(EMULATED_MODEL_KEYLAB_ESSENTIAL_61)
  ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING,
  ITF_NUM_TOTAL,
#else
  ITF_NUM_ALV = 0,
  ITF_NUM_ALV_STREAMING,
  ITF_NUM_DIN_THRU,
  ITF_NUM_DIN_THRU_STREAMING,
  ITF_NUM_MCU_HUI,
  ITF_NUM_MCU_HUI_STREAMING,
  ITF_NUM_MIDI,
  ITF_NUM_MIDI_STREAMING,
  ITF_NUM_TOTAL,
#endif
};

#if defined(EMULATED_MODEL_KEYLAB_ESSENTIAL_61)
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)
#else
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + (4 * TUD_MIDI_DESC_LEN))
#endif

#define EPNUM_ALV_OUT      0x01
#define EPNUM_ALV_IN       0x81
#define EPNUM_DIN_THRU_OUT 0x02
#define EPNUM_DIN_THRU_IN  0x82
#define EPNUM_MCU_HUI_OUT  0x03
#define EPNUM_MCU_HUI_IN   0x83
#define EPNUM_MIDI_OUT     0x04
#define EPNUM_MIDI_IN      0x84

#define TUD_MIDI_DESC_HEAD_ALT(_itfnum, _stridx, _numcables) \
  /* Audio Control (AC) Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, AUDIO_FUNC_PROTOCOL_CODE_UNDEF, _stridx,\
  /* AC Header */\
  9, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_HEADER, U16_TO_U8S_LE(0x0100), U16_TO_U8S_LE(0x0009), 1, (uint8_t)((_itfnum) + 1),\
  /* MIDI Streaming (MS) Interface */\
  9, TUSB_DESC_INTERFACE, (uint8_t)((_itfnum) + 1), 0, 2, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_MIDI_STREAMING, AUDIO_FUNC_PROTOCOL_CODE_UNDEF, _stridx,\
  /* MS Header */\
  7, TUSB_DESC_CS_INTERFACE, MIDI_CS_INTERFACE_HEADER, U16_TO_U8S_LE(0x0100), U16_TO_U8S_LE(7 + (_numcables) * TUD_MIDI_DESC_JACK_LEN + 2 * TUD_MIDI_DESC_EP_LEN(_numcables))

#define TUD_MIDI_DESC_JACK_DESC_ALT(_cablenum, _stridx) \
  6, TUSB_DESC_CS_INTERFACE, MIDI_CS_INTERFACE_IN_JACK, MIDI_JACK_EMBEDDED, TUD_MIDI_JACKID_IN_EMB(_cablenum), _stridx,\
  6, TUSB_DESC_CS_INTERFACE, MIDI_CS_INTERFACE_IN_JACK, MIDI_JACK_EXTERNAL, TUD_MIDI_JACKID_IN_EXT(_cablenum), _stridx,\
  9, TUSB_DESC_CS_INTERFACE, MIDI_CS_INTERFACE_OUT_JACK, MIDI_JACK_EMBEDDED, TUD_MIDI_JACKID_OUT_EMB(_cablenum), 1, TUD_MIDI_JACKID_IN_EXT(_cablenum), 1, _stridx,\
  9, TUSB_DESC_CS_INTERFACE, MIDI_CS_INTERFACE_OUT_JACK, MIDI_JACK_EXTERNAL, TUD_MIDI_JACKID_OUT_EXT(_cablenum), 1, TUD_MIDI_JACKID_IN_EMB(_cablenum), 1, _stridx

#define TUD_MIDI_DESCRIPTOR_ALT(_itfnum, _stridx, _cable, _epout, _epin, _epsize) \
  TUD_MIDI_DESC_HEAD_ALT(_itfnum, _stridx, 1),\
  TUD_MIDI_DESC_JACK_DESC_ALT(_cable, _stridx),\
  TUD_MIDI_DESC_EP(_epout, _epsize, 1),\
  TUD_MIDI_JACKID_IN_EMB(_cable),\
  TUD_MIDI_DESC_EP(_epin, _epsize, 1),\
  TUD_MIDI_JACKID_OUT_EMB(_cable)

#if defined(EMULATED_MODEL_KEYLAB_ESSENTIAL_61)
uint8_t const desc_fs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MIDI, STRID_PORT_MIDI, mock_arturia_controller::MidiCableNumberForPort(0), EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 64),
};

#if TUD_OPT_HIGH_SPEED
uint8_t const desc_hs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MIDI, STRID_PORT_MIDI, mock_arturia_controller::MidiCableNumberForPort(0), EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 512),
};
#endif
#else
uint8_t const desc_fs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_ALV, STRID_PORT_ALV, mock_arturia_controller::MidiCableNumberForPort(0), EPNUM_ALV_OUT, (0x80 | EPNUM_ALV_IN), 64),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_DIN_THRU, STRID_PORT_DIN_THRU, mock_arturia_controller::MidiCableNumberForPort(1), EPNUM_DIN_THRU_OUT, (0x80 | EPNUM_DIN_THRU_IN), 64),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MCU_HUI, STRID_PORT_MCU_HUI, mock_arturia_controller::MidiCableNumberForPort(2), EPNUM_MCU_HUI_OUT, (0x80 | EPNUM_MCU_HUI_IN), 64),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MIDI, STRID_PORT_MIDI, mock_arturia_controller::MidiCableNumberForPort(3), EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 64),
};

#if TUD_OPT_HIGH_SPEED
uint8_t const desc_hs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_ALV, STRID_PORT_ALV, mock_arturia_controller::MidiCableNumberForPort(0), EPNUM_ALV_OUT, (0x80 | EPNUM_ALV_IN), 512),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_DIN_THRU, STRID_PORT_DIN_THRU, mock_arturia_controller::MidiCableNumberForPort(1), EPNUM_DIN_THRU_OUT, (0x80 | EPNUM_DIN_THRU_IN), 512),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MCU_HUI, STRID_PORT_MCU_HUI, mock_arturia_controller::MidiCableNumberForPort(2), EPNUM_MCU_HUI_OUT, (0x80 | EPNUM_MCU_HUI_IN), 512),
  TUD_MIDI_DESCRIPTOR_ALT(ITF_NUM_MIDI, STRID_PORT_MIDI, mock_arturia_controller::MidiCableNumberForPort(3), EPNUM_MIDI_OUT, (0x80 | EPNUM_MIDI_IN), 512),
};
#endif
#endif

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;

#if TUD_OPT_HIGH_SPEED
  return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
  return desc_fs_configuration;
#endif
}

//--------------------------------------------------------------------+
// String descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr[] = {
  (const char[]){0x09, 0x04},
  mock_arturia_controller::kModelConfig.manufacturer.data(),
  mock_arturia_controller::kModelConfig.product.data(),
  mock_arturia_controller::kModelConfig.midi_port_names[0].data(),
  mock_arturia_controller::kModelConfig.midi_port_names[1].data(),
  mock_arturia_controller::kModelConfig.midi_port_names[2].data(),
  mock_arturia_controller::kModelConfig.midi_port_names[3].data(),
  nullptr,
};

static uint16_t _desc_str[32 + 1];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t chr_count = 0;

  switch (index) {
    case STRID_LANGID:
      std::memcpy(&_desc_str[1], string_desc_arr[0], 2);
      chr_count = 1;
      break;

    case STRID_SERIAL:
      chr_count = board_usb_get_serial(_desc_str + 1, 32);
      break;

    default:
      if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
        return nullptr;
      }
      const char* str = string_desc_arr[index];
      if (str == nullptr) {
        return nullptr;
      }
      chr_count = std::strlen(str);
      const size_t max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
      if (chr_count > max_count) {
        chr_count = max_count;
      }
      for (size_t i = 0; i < chr_count; ++i) {
        _desc_str[1 + i] = static_cast<uint16_t>(static_cast<unsigned char>(str[i]));
      }
      break;
  }

  _desc_str[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
