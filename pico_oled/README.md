# Example application showing how to use the pico_oled library

This example reads a GPIO pin (PHONE_DETECT_PIN) to detect the phone/LED state and
updates the SSD1306 OLED accordingly.

Wiring
- VCC -> 3.3V
- GND -> GND
- SDA -> GPIO4
- SCL -> GPIO5
- PHONE_DETECT_PIN -> the pin your project uses to detect phone connection (pulled HIGH when connected)

Build
1. Add pico_oled as a library or add the source files to your target:

   target_include_directories(your_target PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pico_oled)
   target_sources(your_target PRIVATE
       pico_oled/ssd1306.c
       pico_oled/oled.c
   )

2. Make sure your target links pico_stdlib and hardware_i2c:

   target_link_libraries(your_target PRIVATE pico_stdlib hardware_i2c)

3. Example main is provided as examples/example_main.c — copy or include it in your project.

Notes
- Display defaults: 128x64, I2C addr 0x3C. To change pins or address edit example_main.c and oled_init_i2c call.
