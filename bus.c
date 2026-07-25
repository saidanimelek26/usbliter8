#include "bus.h"

#include <pico/time.h>
#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "pio_usb.h"
#include "pio_usb_ll.h"
#include "usb_definitions.h"
#include "log.h"

void bus_init(bus_t *b, bool skip_alarm_pool) {
    pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
    cfg.skip_alarm_pool = skip_alarm_pool;
    b->dev  = pio_usb_host_init(&cfg);
    b->root = PIO_USB_ROOT_PORT(0);
    b->pp   = PIO_USB_PIO_PORT(0);
    b->maxpacket0 = 8;
}

bool bus_wait_for_connect(bus_t *b) {
    int timeout = 5000; // 5 second timeout
    while (!b->root->connected && timeout > 0) {
        sleep_ms(10);
        timeout--;
        tight_loop_contents();
    }
    return b->root->connected;
}

void bus_close_all(bus_t *b) {
    if (b->dev) {
        pio_usb_host_close_device(0, b->dev->address);
    }
}

void bus_reset(bus_t *b, uint32_t hold_ms) {
    pio_usb_host_port_reset_start(0);
    sleep_ms(hold_ms);

    pio_usb_host_port_reset_end(0);
    sleep_ms(50);  // recovery + slack

    if (b->dev) {
        b->dev->connected    = true;
        b->dev->is_fullspeed = b->root->is_fullspeed;
        b->dev->is_root      = true;
        b->dev->root         = b->root;
        b->dev->address      = 0;
    }
}

bool bus_open_ep0(bus_t *b, uint8_t maxpacket) {
    endpoint_descriptor_t ep0 = {
        .length   = 7,
        .type     = DESC_TYPE_ENDPOINT,
        .epaddr   = 0x00,
        .attr     = EP_ATTR_CONTROL,
        .max_size = {maxpacket, 0},
        .interval = 0,
    };

    if (!pio_usb_host_endpoint_open(0, b->dev->address, (const uint8_t *)&ep0, false)) {
        return false;
    }

    b->maxpacket0 = maxpacket;
    return true;
}

void bus_reset_ep0_reopen(bus_t *b) {
    bus_close_all(b);
    bus_reset(b, 20);
    
    // Try to open EP0 with proper max packet size
    // For full-speed devices, max packet size can be 8, 16, 32, or 64
    uint8_t max_packet_sizes[] = {64, 8, 16, 32};
    bool opened = false;
    
    for (int i = 0; i < 4; i++) {
        if (bus_open_ep0(b, max_packet_sizes[i])) {
            INFO("EP0 opened with max packet size %d", max_packet_sizes[i]);
            opened = true;
            break;
        }
        sleep_ms(50);
    }
    
    if (!opened) {
        INFO("Failed to open EP0 with any max packet size");
    }
}

int bus_control_xfer(bus_t *b, const uint8_t setup[8], uint8_t *data, uint16_t data_len, bool data_in, uint32_t timeout_ms) {
    bool has_data = (data != NULL) && (data_len > 0);

    // Clear the control pipe state
    memset(&b->dev->control_pipe, 0, sizeof(b->dev->control_pipe));
    
    b->dev->control_pipe.stage          = STAGE_SETUP;
    b->dev->control_pipe.operation      = data_in ? CONTROL_IN : CONTROL_OUT;
    b->dev->control_pipe.rx_buffer      = data_in ? data : NULL;
    b->dev->control_pipe.request_length = data_len;
    b->dev->control_pipe.out_data_packet.tx_address = (!data_in && has_data) ? data : NULL;
    b->dev->control_pipe.out_data_packet.tx_length  = (!data_in && has_data) ? data_len : 0;

    if (!pio_usb_host_send_setup(0, b->dev->address, setup)) {
        return -1;
    }

    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (b->dev->control_pipe.operation != CONTROL_COMPLETE && b->dev->control_pipe.operation != CONTROL_ERROR) {
        if (time_reached(deadline)) {
            return -2;
        }
        tight_loop_contents();
    }

    if (b->dev->control_pipe.operation == CONTROL_ERROR) {
        return -1;
    }

    // Return the actual number of bytes transferred
    if (data_in && data) {
        return b->dev->control_pipe.rx_length;
    }
    
    return 0; // Success for OUT transfers
}

bool bus_enumerate_device(bus_t *b) {
    // Wait for device to be connected
    if (!bus_wait_for_connect(b)) {
        INFO("Device not connected");
        return false;
    }
    
    INFO("Device connected, waiting to stabilize...");
    sleep_ms(200);
    
    // Reset the device
    INFO("Resetting device...");
    bus_reset(b, 20);
    
    // Give device time after reset
    sleep_ms(100);
    
    // Try to open EP0 with different max packet sizes
    uint8_t max_packet_sizes[] = {64, 8, 16, 32};
    bool opened = false;
    
    for (int i = 0; i < 4; i++) {
        if (bus_open_ep0(b, max_packet_sizes[i])) {
            INFO("EP0 opened with max packet size %d", max_packet_sizes[i]);
            opened = true;
            break;
        }
        sleep_ms(50);
    }
    
    if (!opened) {
        INFO("Failed to open EP0");
        return false;
    }
    
    INFO("Device enumerated successfully");
    return true;
}

void bus_dump_state(bus_t *b) {
    if (!b) {
        printf("Bus is NULL\n");
        return;
    }
    
    printf("=== USB Bus State ===\n");
    printf("Root connected: %s\n", b->root ? (b->root->connected ? "YES" : "NO") : "NULL");
    printf("Root fullspeed: %s\n", b->root ? (b->root->is_fullspeed ? "YES" : "NO") : "NULL");
    printf("Device: %s\n", b->dev ? "EXISTS" : "NULL");
    if (b->dev) {
        printf("  Address: %d\n", b->dev->address);
        printf("  Connected: %s\n", b->dev->connected ? "YES" : "NO");
        printf("  Fullspeed: %s\n", b->dev->is_fullspeed ? "YES" : "NO");
        printf("  Maxpacket0: %d\n", b->maxpacket0);
        printf("  Control pipe stage: %d\n", b->dev->control_pipe.stage);
        printf("  Control pipe operation: %d\n", b->dev->control_pipe.operation);
    }
    printf("=====================\n");
}
