#pragma once

#include "bus.h"
#include <stdbool.h>
#include <stdint.h>

void usb_start(void);
int  usb_bus_init(void);
int  usb_bus_wait_for_device(void);
int  usb_bus_reset_open_ep0(void);

typedef int (*usb_executee_t)(bus_t *b, void *ctx);

int usb_bus_execute(usb_executee_t func, void *ctx, uint64_t timeout);

// ============================================
// New functions for OLED display and device info
// ============================================

/**
 * Get the Vendor ID of the connected USB device
 * @return uint16_t Vendor ID (e.g., 0x05AC for Apple)
 */
uint16_t usb_get_vid(void);

/**
 * Get the Product ID of the connected USB device
 * @return uint16_t Product ID (e.g., 0x12A8 for iPhone XS)
 */
uint16_t usb_get_pid(void);

/**
 * Check if the device is supported by usbliter8
 * @param vid Vendor ID
 * @param pid Product ID
 * @return true if supported, false otherwise
 */
bool is_device_supported(uint16_t vid, uint16_t pid);

/**
 * Wait for a USB device with timeout
 * @param timeout_ms Timeout in milliseconds
 * @return true if device found, false if timeout
 */
bool usb_bus_wait_for_device_timeout(uint32_t timeout_ms);

/**
 * Check if the connected device is already pwned
 * @return true if device is already pwned, false otherwise
 */
bool usb_is_device_pwned(void);

/**
 * Get device info string (for OLED display)
 * @param vid Vendor ID
 * @param pid Product ID
 * @param buf Buffer to store the string
 * @param buf_size Size of the buffer
 */
void usb_get_device_info_string(uint16_t vid, uint16_t pid, char *buf, size_t buf_size);

/**
 * Get human-readable device name
 * @param vid Vendor ID
 * @param pid Product ID
 * @return String describing the device, or "Unknown Device"
 */
const char* usb_get_device_name(uint16_t vid, uint16_t pid);

#endif // USB_H
