// RP2040 erratum E5: after a bus reset the B0/B1 device controller waits for 800 us of idle J before it counts as
// connected; behind a hub (every port of a Raspberry Pi 4) the bus seldom stays idle that long, the controller stays in the
// reset and the host's SET_ADDRESS gets no answer. The workaround forces J through GPIO15 for 1 ms after each reset.
//
// The SDK's version of it (pico_fix/rp2040_usb_device_enumeration) failed on the Pi in three ways, so TinyUSB's call is
// redirected here (-Wl,--wrap=rp2040_usb_device_enumeration_fix):
// - It checks the line state right after switching the controller to the forced pad; the switch is not always visible
//   yet, and the hard_assert panicked the chip during enumeration ("Hard assert" on the display, "device not accepting
//   address, error -71" on the host).
// - A bus reset while the workaround runs starts it again: it then saves the already forced GPIO15 setup as the one to
//   restore, and two alarms race to finish.
// - Its last step spins in the timer interrupt until the controller reports a connection, without a limit.
// Here the line state is awaited for a bounded time, a reset during the workaround first undoes the forcing, and the last
// step polls from its alarm. Each step that cannot complete panics with a short reason (the panic screen shows 9
// characters), as the SDK's asserts did.

#include <cstdint>

#include "hardware/gpio.h"
#include "hardware/structs/iobank0.h"
#include "hardware/structs/padsbank0.h"
#include "hardware/structs/usb.h"
#include "pico/platform.h"
#include "pico/time.h"

namespace {

constexpr uint kDp = 15;
constexpr uint kDm = 16;
constexpr std::uint8_t kLineStateSe0 = 0;
constexpr std::uint8_t kLineStateJ = 1;
// The forced J shows within a few USB clock cycles; this is ample
constexpr std::uint32_t kForcedJTimeoutUs = 100;
// The controller connects after 800 us of J; polled every millisecond after the first
constexpr int kConnectPollLimit = 20;

enum class FixState : std::uint8_t {
  kIdle,
  kWaitingForResetEnd,
  kForcingJ,
};

// Changed only from the USB and the timer interrupt; both run on core 0 at the same priority, so neither interrupts the
// other and no lock is needed.
FixState state = FixState::kIdle;
alarm_id_t pending_alarm = 0;
std::uint32_t saved_gpio_ctrl = 0;
std::uint32_t saved_pad_ctrl = 0;
int connect_polls = 0;

std::uint8_t line_state() {
  return static_cast<std::uint8_t>((usb_hw->sie_status & USB_SIE_STATUS_LINE_STATE_BITS) >> USB_SIE_STATUS_LINE_STATE_LSB);
}

void schedule(alarm_callback_t callback) {
  pending_alarm = add_alarm_in_ms(1, callback, nullptr, true);
  if (pending_alarm < 0) {
    panic("E5 alarm");
  }
}

void stop_forcing_j() {
  usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;
  hw_clear_bits(&usb_hw->phy_direct_override, USB_USBPHY_DIRECT_OVERRIDE_DP_PULLUP_EN_OVERRIDE_EN_BITS);
  io_bank0_hw->io[kDp].ctrl = saved_gpio_ctrl;
  pads_bank0_hw->io[kDp] = saved_pad_ctrl;
}

std::int64_t finish(alarm_id_t, void*) {
  if (!(usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS)) {
    if (++connect_polls > kConnectPollLimit) {
      panic("E5 conn");
    }
    return 1000;
  }
  stop_forcing_j();
  pending_alarm = 0;
  state = FixState::kIdle;
  return 0;
}

void force_j() {
  if (gpio_get_function(kDm) == static_cast<gpio_function_t>(8)) {
    panic("E5 GPIO16");
  }

  saved_gpio_ctrl = io_bank0_hw->io[kDp].ctrl;
  saved_pad_ctrl = pads_bank0_hw->io[kDp];

  // Bus keep and output disabled, so the USB debug muxing on GPIO15 cannot drive the pin
  gpio_set_pulls(kDp, true, true);
  gpio_set_oeover(kDp, GPIO_OVERRIDE_LOW);
  hw_write_masked(&io_bank0_hw->io[kDp].ctrl, 8 << IO_BANK0_GPIO15_CTRL_FUNCSEL_LSB, IO_BANK0_GPIO15_CTRL_FUNCSEL_BITS);
  // J is DP high, DM low; DM is already gated low while GPIO16 is not on function 8
  gpio_set_inover(kDp, GPIO_OVERRIDE_HIGH);

  // The pull-up must stay on while the controller is switched away from the PHY, or the host sees a disconnect
  usb_hw_t* usb_hw_set = static_cast<usb_hw_t*>(hw_set_alias_untyped(usb_hw));
  usb_hw_set->phy_direct = USB_USBPHY_DIRECT_DP_PULLUP_EN_BITS;
  usb_hw_set->phy_direct_override = USB_USBPHY_DIRECT_OVERRIDE_DP_PULLUP_EN_OVERRIDE_EN_BITS;
  usb_hw->muxing = USB_USB_MUXING_TO_DIGITAL_PAD_BITS | USB_USB_MUXING_SOFTCON_BITS;

  const absolute_time_t deadline = make_timeout_time_us(kForcedJTimeoutUs);
  while (line_state() != kLineStateJ) {
    if (time_reached(deadline)) {
      panic("E5 no J");
    }
  }

  state = FixState::kForcingJ;
  connect_polls = 0;
  schedule(finish);
}

std::int64_t wait_for_reset_end(alarm_id_t, void*) {
  // The host's reset (SE0) lasts 10 to 20 ms
  if (line_state() == kLineStateSe0) {
    return 1000;
  }
  force_j();
  return 0;
}

}  // namespace

extern "C" void __wrap_rp2040_usb_device_enumeration_fix(void) {
  if (rp2040_chip_version() != 1) {
    return;
  }

  // A reset while the workaround runs: undo the forcing first, so that the GPIO15 setup saved next is the original one
  if (state != FixState::kIdle) {
    cancel_alarm(pending_alarm);
    pending_alarm = 0;
    if (state == FixState::kForcingJ) {
      stop_forcing_j();
    }
  }

  state = FixState::kWaitingForResetEnd;
  schedule(wait_for_reset_end);
}
