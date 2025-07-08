#pragma once
#include <cstdint>
#include <string>
#include <linux/input-event-codes.h>


namespace InputDevice {
// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Check input device event type capabilities.
 * @param fd File descriptor of opened event file from `/dev/input/event*`
 * @param eventId Event type id from `linux/input-event-codes.h`. Example: `EV_KEY`
 * @return `true` if device supports provided event type. Errors also return `false`.
 */
bool supportsEventType(int fd, uint32_t eventId) noexcept;


/**
 * @brief Check if input device supports a specific key event.
 * @param fd File descriptor of opened event file from `/dev/input/event*`
 * @param keyId Key id from `linux/input-event-codes.h`. Example: `KEY_A`
 * @return `true` if device supports provided key event. Errors also return `false`.
 */
bool supportsKeyEvent(int fd, uint32_t keyId) noexcept;


/**
 * @brief Check if input device supports an LED.
 * @param fd File descriptor of opened event file from `/dev/input/event*`
 * @param ledId LED id from `linux/input-event-codes.h`. Example: `LED_NUML`
 * @return `true` if device supports the provided LED. Errors also return `false`.
 */
bool supportsLED(int fd, uint32_t ledId) noexcept;


/**
 * @brief Get LED status.
 * @param fd File descriptor of opened event file from `/dev/input/event*`
 * @param ledId LED id from `linux/input-event-codes.h`. Example: `LED_NUML`
 * @return `true` if LED is turned on.
 * @return `false` if LED is turned off or when an error occured.
 */
bool getLED(int fd, uint32_t ledId) noexcept;


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Get name of input device.
 * @param fd File descriptor of opened event file from `/dev/input/event*`
 */
std::string getName(int fd);


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool toggleNumlock(int fd) noexcept;


// ------------------------------------------------------------------------------------------ //
}