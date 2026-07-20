__attribute__((noreturn))
void do_auto(void) {
    while (true) {
        // show waiting until a DFU-capable device appears
        oled_show_connecting();

        // block until any USB device connects
        usb_bus_wait_for_device();

        // try to identify device
        uint16_t cpid = 0;
        bool pwnd = false;
        int idret = device_identify(&cpid, &pwnd);

        if (idret == -2) {
            // couldn't talk to device (not in DFU / not responding) -> keep waiting
            oled_show_connecting();
            sleep_ms(CONNECTION_FAIL_SLEEP_MS);
            usb_bus_reset_open_ep0();
            continue;
        }

        if (idret != 0) {
            // device present but not Apple DFU (or other transient error)
            oled_show_status(false); // shows "Phone: Disconnected"
            sleep_ms(CONNECTION_FAIL_SLEEP_MS);
            usb_bus_reset_open_ep0();
            continue;
        }

        // identified as Apple DFU
        if (pwnd) {
            oled_show_already_pwned(); // shows "Already / PWNED"
            // DEFAULT: stay here (don't try exploit again). Change behavior if you want.
            while (1) sleep_ms(1000);
        }

        // choose exploit based on cpid (exploit_run already handles this internally,
        // but we can use direct usb_bus_execute flows if desired)
        usb_executee_t func = NULL;
        switch (cpid) {
            case 0x8020:
            case 0x8006:
                func = t8020_t8006_exploit_run;
                break;
            case 0x8030:
                func = t8030_exploit_run;
                break;
            default:
                // unsupported CPID -> treat as disconnected per requirement
                oled_show_status(false);
                sleep_ms(CONNECTION_FAIL_SLEEP_MS);
                usb_bus_reset_open_ep0();
                continue;
        }

        // run exploit
        oled_show_message("Exploit:", "Running");
        int ret = usb_bus_execute(func, &cpid, EXPLOIT_TIMEOUT);

        if (ret != 0) {
            oled_show_message("Exploit:", "Error");
            // retry after a short delay (default). If you want to stop, change this.
            sleep_ms(CONNECTION_FAIL_SLEEP_MS);
            usb_bus_reset_open_ep0();
            continue;
        }

        // verify result
        usb_bus_reset_open_ep0();
        if (device_identify(&cpid, &pwnd) != 0 || !pwnd) {
            oled_show_message("Exploit:", "Error");
            sleep_ms(CONNECTION_FAIL_SLEEP_MS);
            continue;
        }

        // success
        oled_show_message("Exploit:", "Done");
        led_set_state(LED_STATE_SUCCESS);
        while (1) sleep_ms(1000);
    }
}
