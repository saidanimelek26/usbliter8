#include <pico/stdio.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "led.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "bus.h"
#include "exploit.h"
#include "usb.h"
#include "log.h"

#include "pico_oled/oled.h"

#if WITH_AUTO_MODE

#define WITH_AUTO_REBOOT            (0)
#define FAILURE_REBOOT_DELAY_SEC    (3)
#define SUCCESS_REBOOT_DELAY_SEC    (5)

#define CONNECTION_FAIL_SLEEP_MS    (500)

__attribute__((noreturn))
void fatal_failure(void) {
    led_set_state(LED_STATE_ERROR);
    oled_show_error_msg("Fatal error");  // CHANGED: use oled_show_error_msg

#if WITH_AUTO_REBOOT
    printf("\nfatal failure, rebooting in %d seconds\n", FAILURE_REBOOT_DELAY_SEC);

    sleep_ms(FAILURE_REBOOT_DELAY_SEC * 1000);

    watchdog_reboot(0, SRAM_END, 1);
    while (1) {}

#else
    printf("\nfatal failure, spinning forever\n");

    while (1) {
        sleep_ms(100);
    }
#endif
}

__attribute__((noreturn))
void do_auto(void) {
    while (true) {
        oled_show_exploiting();
        int ret = exploit_run();

        /*
         * This case means not even device descriptor
         * could be queried
         *
         * On those boards with redundant resistor
         * it could mean that device was not even
         * connected yet
         *
         * So we sleep a bit, reset the bus and try again
         *
         * UPDATE: same behavior for already PWNED devices
         */
        if (ret == -2) {
            oled_show_error_msg("No device");  // CHANGED: use oled_show_error_msg
            sleep_ms(CONNECTION_FAIL_SLEEP_MS);
            usb_bus_reset_open_ep0();
            continue;
        }

        /* Device already pwned */
        if (ret == -3) {
            oled_show_already_pwned();
            sleep_ms(2000);
            oled_show_reboot_needed();
            fatal_failure();
        }

        /* Unsupported device */
        if (ret == -4) {
            oled_show_unsupported();
            fatal_failure();
        }

        /* no-go failure, bailing */
        if (ret != 0) {
            oled_show_error_msg("Exploit failed");  // CHANGED: use oled_show_error_msg
            fatal_failure();
        }

        /* it all went well then */
        oled_show_pwnd_success();
        break;
    }

#if WITH_AUTO_REBOOT
    printf("\nsuccess, rebooting in %d seconds\n", SUCCESS_REBOOT_DELAY_SEC);

    sleep_ms(SUCCESS_REBOOT_DELAY_SEC * 1000);

    watchdog_reboot(0, SRAM_END, 1);
    while (1) {}
#else
    printf("\nsuccess, spinning forever\n");
    oled_show_done();

    while (1) {
        sleep_ms(100);
    }
#endif
}

#else

static void help(void) {
    printf("'e'\texploit\n");
    printf("'r'\tbus reset\n");
    printf("'p'\treboot\n");
    printf("'h'\thelp\n");
}

void do_shell(void) {
    char buf[2] = { 0 };

    printf("\nDEBUG build, starting command prompt\n");

    printf("\n");
    help();
    printf("\n");

    while (1) {
        printf("> ");

        char c = stdio_getchar();

        printf("%c\n", c);

        switch (c) {
            case 'r': {
                printf("Resetting the bus...\n");
                usb_bus_reset_open_ep0();
                oled_show_reset();
                break;
            }

            case 'e': {
                usb_bus_reset_open_ep0();
                oled_show_exploiting();
                int ret = exploit_run();

                if (ret == -2) {
                    printf("failed to discover a device\n");
                    oled_show_error_msg("No device");  // CHANGED: use oled_show_error_msg
                } else if (ret == -3) {
                    printf("device already pwned\n");
                    oled_show_already_pwned();
                } else if (ret == -4) {
                    printf("device not supported\n");
                    oled_show_unsupported();
                } else if (ret == 0) {
                    printf("exploit successful!\n");
                    oled_show_pwnd_success();
                } else {
                    printf("exploit failed with code: %d\n", ret);
                    oled_show_error_msg("Failed");  // CHANGED: use oled_show_error_msg
                }
                break;
            }

            case 'p': {
                printf("Rebooting the Pico...\n");
                oled_show_reboot_needed();
                sleep_ms(100);
                watchdog_reboot(0, SRAM_END, 1);
                while (1) {}
            }

            case 'h': {
                help();
                break;
            }

            default: {
                printf("Unknown command '%s'\n", buf);
                break;
            }
        }
    }
}

#endif

int main(void) {
    // PIO USB needs sys_clk to be a multiple of 12 MHz
#if PICO_RP2350
    set_sys_clock_khz(156000, true);
#elif PICO_RP2040
    set_sys_clock_khz(120000, true);
#else
#error What is this MCU even?
#endif

    led_init();
    led_set_state(LED_STATE_BOOTING);

    stdio_init_all();

    // this delay being long enough, seems to have
    // a HUGE impact on the exploit reliability
    sleep_ms(2000);

    led_set_state(LED_STATE_IDLE);

    // Initialize OLED (SDA=GPIO4, SCL=GPIO5)
    oled_init_i2c(i2c0, 4, 5);
    oled_show_booting();

    printf("\n============ %s v%s ============\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
    printf("built for %s, PIO USB @ GP%d/%d (D+/D-)\n\n", BOARD_NAME, PIO_USB_DP_PIN_DEFAULT, PIO_USB_DP_PIN_DEFAULT + 1);

#if PICO_RP2040
    printf("==========================================================\n");
    printf("WARNING: RP2040 reliability is not the best in many cases,\n");
    printf("you shall better switch to RP2350\n");
    printf("==========================================================\n");
    printf("\n");
#endif

    // Wait for DFU device
    oled_show_waiting_dfu();
    
    usb_start();
    usb_bus_init();
    
    oled_show_waiting_for_device();
    usb_bus_wait_for_device();

    // Device detected - get info
    uint16_t vid = usb_get_vid();
    uint16_t pid = usb_get_pid();
    
    oled_show_device_found(vid, pid);
    sleep_ms(1000);
    
    // Check if device is supported
    if (!is_device_supported(vid, pid)) {
        oled_show_unsupported_device(vid, pid);
        printf("Unsupported device: VID=0x%04X, PID=0x%04X\n", vid, pid);
        fatal_failure();
    }
    
    oled_show_device_detected();
    sleep_ms(500);

    usb_bus_reset_open_ep0();

#if WITH_AUTO_MODE
    do_auto();
#else
    do_shell();
#endif
}
