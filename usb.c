#include <stdint.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/time.h"

#include "usb.h"
#include "bus.h"
#include "log.h"

enum {
    USB_CMD_BUS_INIT = 0,
    USB_CMD_WAIT_FOR_DEVICE,
    USB_CMD_RESET_OPEN_EP0,
    USB_CMD_EXECUTE_FUNC,
    USB_CMD_GET_VID_PID
};

static struct {
    usb_executee_t func;
    void *ctx;
} usb_exec_ctx;

static bus_t gBus = { 0 };

// Store device VID/PID
static uint16_t g_device_vid = 0;
static uint16_t g_device_pid = 0;

void usb_task(void) {
    while (1) {
        uint32_t cmd = multicore_fifo_pop_blocking();
        uint32_t ret = -1;

        switch (cmd) {
            case USB_CMD_BUS_INIT: {
                bus_init(&gBus, false);
                ret = 0;
                break;
            }

            case USB_CMD_WAIT_FOR_DEVICE: {
                bus_wait_for_connect(&gBus);
                // Device is connected, we'll get VID/PID later
                ret = 0;
                break;
            }

            case USB_CMD_RESET_OPEN_EP0: {
                bus_reset_ep0_reopen(&gBus);
                ret = 0;
                break;
            }

            case USB_CMD_EXECUTE_FUNC: {
                ret = usb_exec_ctx.func(&gBus, usb_exec_ctx.ctx);
                break;
            }

            case USB_CMD_GET_VID_PID: {
                // Return VID in high 16 bits, PID in low 16 bits
                ret = ((uint32_t)g_device_vid << 16) | g_device_pid;
                break;
            }
        }

        multicore_fifo_push_blocking(ret);
    }
}

void usb_start(void) {
    multicore_reset_core1();
    multicore_launch_core1(usb_task);
}

int _usb_task_execute_cmd(int cmd, uint64_t timeout) {
    multicore_fifo_push_blocking(cmd);

    uint32_t out = -1;

    if (timeout) {
        if (!multicore_fifo_pop_timeout_us(timeout, &out)) {
            INFO("TIMEOUT");
        }
    } else {
        out = multicore_fifo_pop_blocking();
    }

    return out;
}

#define DEFAULT_TIMEOUT_US  (100 * 1000)

int usb_bus_init(void) {
    return _usb_task_execute_cmd(USB_CMD_BUS_INIT, DEFAULT_TIMEOUT_US);
}

int usb_bus_wait_for_device(void) {
    INFO("waiting for device...");

    int ret = _usb_task_execute_cmd(USB_CMD_WAIT_FOR_DEVICE, 0);
    if (ret == 0) {
        INFO("connected, speed = %s", gBus.root->is_fullspeed ? "FS" : "LS");
    }

    return ret;
}

int usb_bus_reset_open_ep0(void) {
    int ret = _usb_task_execute_cmd(USB_CMD_RESET_OPEN_EP0, DEFAULT_TIMEOUT_US);
    if (ret == 0) {
        INFO("opened EP0");
    }

    return ret;
}

int usb_bus_execute(usb_executee_t func, void *ctx, uint64_t timeout) {
    usb_exec_ctx.func = func;
    usb_exec_ctx.ctx = ctx;

    return _usb_task_execute_cmd(USB_CMD_EXECUTE_FUNC, timeout);
}

// ============================================
// New functions for OLED display and device info
// ============================================

uint16_t usb_get_vid(void) {
    // Try to get VID/PID from the bus
    uint32_t result = _usb_task_execute_cmd(USB_CMD_GET_VID_PID, DEFAULT_TIMEOUT_US);
    if (result != (uint32_t)-1) {
        g_device_vid = (result >> 16) & 0xFFFF;
        g_device_pid = result & 0xFFFF;
    }
    return g_device_vid;
}

uint16_t usb_get_pid(void) {
    // Try to get VID/PID from the bus
    uint32_t result = _usb_task_execute_cmd(USB_CMD_GET_VID_PID, DEFAULT_TIMEOUT_US);
    if (result != (uint32_t)-1) {
        g_device_vid = (result >> 16) & 0xFFFF;
        g_device_pid = result & 0xFFFF;
    }
    return g_device_pid;
}

bool is_device_supported(uint16_t vid, uint16_t pid) {
    // Apple devices supported by usbliter8
    if (vid == 0x05AC) { // Apple Vendor ID
        // A12 SoC (iPhone XS, XS Max, XR)
        if (pid == 0x12A8 || pid == 0x12AA || pid == 0x12AB || pid == 0x12AC) {
            return true;
        }
        // S4/S5 (Apple Watch Series 4 & 5)
        if (pid == 0x12A0 || pid == 0x12A2) {
            return true;
        }
        // A13 SoC (iPhone 11, 11 Pro, 11 Pro Max)
        if (pid == 0x12B0 || pid == 0x12B2) {
            return true;
        }
        // A12X/Z (iPad Pro 2018) - theoretically supported but not implemented
        // if (pid == 0x12A4 || pid == 0x12A6) {
        //     return true;
        // }
    }
    return false;
}

bool usb_bus_wait_for_device_timeout(uint32_t timeout_ms) {
    uint32_t start = time_us_32();
    while (time_us_32() - start < timeout_ms * 1000) {
        // Try to get device
        int ret = _usb_task_execute_cmd(USB_CMD_WAIT_FOR_DEVICE, 100 * 1000); // 100ms timeout per attempt
        if (ret == 0) {
            // Get VID/PID
            uint32_t result = _usb_task_execute_cmd(USB_CMD_GET_VID_PID, DEFAULT_TIMEOUT_US);
            if (result != (uint32_t)-1) {
                g_device_vid = (result >> 16) & 0xFFFF;
                g_device_pid = result & 0xFFFF;
                INFO("Device found: VID=0x%04X, PID=0x%04X", g_device_vid, g_device_pid);
                return true;
            }
        }
        sleep_ms(10);
    }
    return false;
}

bool usb_is_device_pwned(void) {
    // Check if device is already pwned
    // This would require reading the device serial number
    // For now, return false
    return false;
}

void usb_get_device_info_string(uint16_t vid, uint16_t pid, char *buf, size_t buf_size) {
    const char *device_name = usb_get_device_name(vid, pid);
    snprintf(buf, buf_size, "%s (0x%04X:0x%04X)", device_name, vid, pid);
}

const char* usb_get_device_name(uint16_t vid, uint16_t pid) {
    if (vid == 0x05AC) { // Apple
        switch (pid) {
            case 0x12A8: return "iPhone XS";
            case 0x12AA: return "iPhone XS Max";
            case 0x12AB: return "iPhone XR";
            case 0x12AC: return "iPhone XS (var)";
            case 0x12A0: return "Apple Watch S4";
            case 0x12A2: return "Apple Watch S5";
            case 0x12B0: return "iPhone 11";
            case 0x12B2: return "iPhone 11 Pro";
            case 0x12A4: return "iPad Pro 2018 (11\")";
            case 0x12A6: return "iPad Pro 2018 (12.9\")";
            default: return "Apple Device";
        }
    }
    return "Unknown Device";
}
