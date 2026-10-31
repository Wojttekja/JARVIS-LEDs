add_executable(remote
    main.c
    input.c
    storage.c
)
target_link_libraries(remote pico_stdlib pico_leds_common)
pico_enable_stdio_usb(remote 1)
pico_enable_stdio_uart(remote 0)
pico_add_extra_outputs(remote)
