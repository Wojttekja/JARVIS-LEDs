add_executable(receiver
    main.c
    effects.c
    ws2811.c
)
pico_generate_pio_header(receiver ${CMAKE_CURRENT_LIST_DIR}/ws2811.pio)
target_link_libraries(receiver
    pico_stdlib pico_leds_common pico_rand hardware_pio hardware_dma hardware_watchdog
)
pico_enable_stdio_usb(receiver 1)
pico_enable_stdio_uart(receiver 0)
pico_add_extra_outputs(receiver)
