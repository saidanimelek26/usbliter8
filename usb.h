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

uint16_t usb_get_vid(void);
uint16_t usb_get_pid(void);
bool is_device_supported(uint16_t vid, uint16_t pid);
bool usb_bus_wait_for_device_timeout(uint32_t timeout_ms);
bool usb_is_device_pwned(void);
void usb_get_device_info_string(uint16_t vid, uint16_t pid, char *buf, size_t buf_size);
const char* usb_get_device_name(uint16_t vid, uint16_t pid);

#endif // USB_H  // <-- ONLY ONE #endif HERE!
