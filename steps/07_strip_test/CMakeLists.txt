add_executable(07_strip_test
    main.c
    ${CMAKE_SOURCE_DIR}/receiver/ws2811.c
)
pico_generate_pio_header(07_strip_test ${CMAKE_SOURCE_DIR}/receiver/ws2811.pio)
target_include_directories(07_strip_test PRIVATE ${CMAKE_SOURCE_DIR}/receiver)
target_link_libraries(07_strip_test pico_stdlib pico_leds_common hardware_pio hardware_dma)
pico_enable_stdio_usb(07_strip_test 1)
pico_enable_stdio_uart(07_strip_test 0)
pico_add_extra_outputs(07_strip_test)
